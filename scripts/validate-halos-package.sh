#!/bin/sh
set -eu
package=$1
test "$(dpkg-deb --field "$package" Package)" = fairwindsk
test "$(dpkg-deb --field "$package" Architecture)" = arm64
contents=$(dpkg-deb --contents "$package")
for path in usr/bin/FairWindSK usr/bin/fairwindsk-startup usr/bin/fairwindsk-halos-setup usr/share/applications/fairwindsk.desktop etc/xdg/autostart/fairwindsk-startup.desktop; do
    printf '%s\n' "$contents" | grep -q "$path$"
done
