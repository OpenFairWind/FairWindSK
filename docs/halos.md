# FairWindSK on HaLOS

FairWindSK runs as a native Qt desktop application on the HaLOS host, while Signal K and other marine services may run in HaLOS-managed containers. Only Desktop images are supported because graphical autorun requires XFCE; headless images are not desktop targets.

## Supported images

| Image | Support |
|---|---|
| `Halos-Desktop-RPI` | Supported |
| `Halos-Desktop-Marine-RPI` | Recommended for Raspberry Pi 4/5 |
| `Halos-Desktop-HALPI2` | Supported |
| `Halos-Desktop-Marine-HALPI2` | Recommended for HALPI2 |
| `Halos-Desktop-Marine-HALPI2-AP` | Recommended when first setup needs a Wi-Fi access point |
| Headless `Halos-*` | Not supported as a desktop target |

The initial package targets Debian 13/Raspberry Pi OS Trixie, ARM64 (`arm64`/`aarch64`), XFCE, and Qt WebEngine. It does not support 32-bit ARM.

## Prepare and install

Boot HaLOS, complete network setup, change default credentials following HaLOS documentation, update, and reboot if system or kernel packages changed:

```bash
sudo apt update
sudo apt full-upgrade
dpkg --print-architecture
```

The result must be `arm64`, and XFCE login must work. Download the `.deb` and `.sha256` from the FairWindSK release, then use APT so dependencies are resolved:

```bash
sha256sum -c fairwindsk_<version>-1.halos13_arm64.deb.sha256
sudo apt install ./fairwindsk_<version>-1.halos13_arm64.deb
```

Binary users do not need Qt build dependencies. Verify before reboot:

```bash
command -v FairWindSK
command -v fairwindsk-startup
command -v fairwindsk-halos-setup
dpkg -s fairwindsk
test -f /usr/share/applications/fairwindsk.desktop
test -f /etc/xdg/autostart/fairwindsk-startup.desktop
fairwindsk-halos-setup --diagnose
```

A temporarily unavailable Signal K endpoint is not an installation defect.

## First launch, Signal K, and autorun

The first graphical-session launch runs `fairwindsk-halos-setup` as the user. Existing `~/.config/FairWindSK/fairwindsk.json` is preserved. If absent, the full packaged default is copied, a fullscreen overlay is applied, and `.halos-setup-v1` is written after success. No credentials are stored. A verified loopback Signal K endpoint may be selected; otherwise use discovery or **Settings > Connection**. Never disable TLS verification globally.

```text
HaLOS boot -> graphical login -> XFCE
  -> /etc/xdg/autostart/fairwindsk-startup.desktop
  -> fairwindsk-startup -> bounded Signal K wait -> FairWindSK
```

The package default wait is 120 seconds and is tunable in `/etc/default/fairwindsk`; invalid values fall back safely. FairWindSK launches after timeout and reconnects when Signal K later becomes available. A per-session lock prevents duplicate launcher instances.

## Helm commissioning

Confirm **Settings > Main > Window mode > Fullscreen**. Calibrate touchscreen, orientation, desktop scaling, and the optional virtual keyboard. Check `default`, `dawn`, `day`, `sunset`, `dusk`, and `night` presets. Then perform a real reboot with `sudo reboot`; XFCE must start normally, FairWindSK must autorun, and delayed Signal K must cause only the bounded wait.

To disable autorun for one user without modifying package files:

```bash
mkdir -p ~/.config/autostart
cp /etc/xdg/autostart/fairwindsk-startup.desktop ~/.config/autostart/
printf '\nHidden=true\n' >> ~/.config/autostart/fairwindsk-startup.desktop
```

Restore it with `rm -f ~/.config/autostart/fairwindsk-startup.desktop`. Manual launch is `fairwindsk-startup` (preferred) or `FairWindSK`.

## Upgrade, rollback, and removal

Upgrade with `sudo apt install ./fairwindsk_<new-version>-1.halos13_arm64.deb`; user configuration is preserved. No FairWindSK HaLOS APT repository is currently promised. Roll back by installing a retained older package after reviewing APT warnings.

Use `sudo apt remove fairwindsk` or `sudo apt purge fairwindsk`. Both retain user data in `~/.config/FairWindSK`; back it up and remove it manually only when a personal-data reset is intended.

## Diagnostics

Run `fairwindsk-halos-setup --diagnose` and `fairwindsk-startup` in a terminal. XFCE session output records startup phases, paths, readiness, executable, exit code, and restart requests without tokens.

- Signal K unavailable at boot is non-fatal; inspect HaLOS services separately and allow reconnection.
- Blank WebEngine pages: check DNS, clock, TLS/certificates, endpoint reachability, runtime packages, and GPU/software rendering.
- Platform-plugin failure: verify normal XFCE and `qt6-qpa-plugins`; avoid arbitrary overrides.
- Duplicate windows: inspect the runtime lock and remove any second user autostart entry.
- Exit code `1` requests a restart; other codes stop the launcher loop.

## Commissioning checklist

- ARM64 package, executable, desktop entry, and autostart verified.
- Real reboot, fullscreen, touch, virtual keyboard, and every comfort preset tested.
- Signal K connection, delayed start, restart/reconnect, and no-network start tested.
- Configuration survives upgrade; setup does not reset changes.
- Autorun disable/restore and remove/reinstall tested.
- Helm display remains readable, high-contrast, single-window, and finger-friendly.

Official acceptance requires these checks on representative HaLOS Desktop-Marine ARM64 hardware; CI cannot validate physical display, touch, GPU, or real container behavior.
