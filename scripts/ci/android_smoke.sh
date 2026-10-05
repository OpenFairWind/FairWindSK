#!/usr/bin/env bash
set -euo pipefail

# Prefer the installable signed APK without coupling CI to a Qt patch release's directory layout.
apk_path="$(find "$1" -type f -name '*signed.apk' -print -quit)"
if [[ -z "${apk_path}" ]]; then
    apk_path="$(find "$1" -type f -name '*.apk' -print -quit)"
fi
test -n "${apk_path}"
adb install -r "${apk_path}"

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
