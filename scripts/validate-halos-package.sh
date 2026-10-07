#!/bin/sh
set -eu
package=$1
test "$(dpkg-deb --field "$package" Package)" = fairwindsk
test "$(dpkg-deb --field "$package" Architecture)" = arm64
contents=$(dpkg-deb --contents "$package")
for path in usr/bin/FairWindSK usr/bin/fairwindsk-startup usr/bin/fairwindsk-halos-setup usr/share/applications/fairwindsk.desktop etc/xdg/autostart/fairwindsk-startup.desktop; do
    # Feed the finite package listing through standard input so grep cannot
    # close a pipe early and make the producer fail with SIGPIPE under set -e.
    grep -q "$path$" <<EOF
$contents
EOF
done
