#!/usr/bin/env bash
set -euo pipefail

# Resolve the generated debug APK without coupling CI to a Qt patch release's directory layout.
apk_path="$(find "$1" -type f -name '*.apk' -print -quit)"
test -n "${apk_path}"
adb install -r "${apk_path}"

# Launch the explicit launcher activity and require Android's Activity Manager to report success.
adb shell am start -W -n org.openfairwind.fairwindsk/org.openfairwind.fairwindsk.FairWindSKActivity | tee /tmp/fairwindsk-am-start.txt
grep -q '^Status: ok' /tmp/fairwindsk-am-start.txt
sleep 10
adb shell pidof org.openfairwind.fairwindsk
