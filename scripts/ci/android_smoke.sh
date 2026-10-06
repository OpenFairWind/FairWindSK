#!/usr/bin/env bash
set -euo pipefail

# Prefer the installable signed APK without coupling CI to a Qt patch release's directory layout.
apk_path="$(find "$1" -type f -name '*signed.apk' -print -quit)"
if [[ -z "${apk_path}" ]]; then
    apk_path="$(find "$1" -type f -name '*.apk' -print -quit)"
fi
test -n "${apk_path}"

# Android may report boot completion before Package Manager is stable on software-emulated runners.
ready_checks=0
for attempt in $(seq 1 36); do
    if adb shell service check package 2>/dev/null | grep -q 'found' && adb shell cmd package list packages >/dev/null 2>&1; then
        ready_checks=$((ready_checks + 1))
        if [[ "${ready_checks}" -ge 3 ]]; then
            break
        fi
    else
        ready_checks=0
    fi
    sleep 5
done
test "${ready_checks}" -ge 3

# Avoid incremental-install transport races and retry while late system services settle.
for attempt in $(seq 1 5); do
    if adb install --no-streaming -r "${apk_path}"; then
        installed=1
        break
    fi
    sleep 10
done
test "${installed:-0}" -eq 1

# Launch the explicit activity without coupling success to Activity Manager's slow-start wait timeout.
adb shell am start -n org.openfairwind.fairwindsk/org.openfairwind.fairwindsk.FairWindSKActivity

# Allow Qt and the system WebView to initialize, then require the application process to remain alive.
for attempt in $(seq 1 12); do
    if adb shell pidof org.openfairwind.fairwindsk; then
        exit 0
    fi
    sleep 5
done

exit 1
