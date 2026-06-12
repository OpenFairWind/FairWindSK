Autostart on Raspberry Pi

`fairwindsk-startup` is a small Raspberry Pi OS helper that:

- finds `fairwindsk.ini` in the same per-user configuration locations used by the application
- resolves the JSON configuration path from that INI file, with legacy fallbacks for older installs
- enables `QT_IM_MODULE=qtvirtualkeyboard` only when `main.virtualKeyboard` is enabled in `fairwindsk.json`
- waits for the configured Signal K web-app catalog to become reachable before launching FairWindSK, so plugin tiles have a better chance of loading their real artwork during kiosk/autostart boots
- launches `FairWindSK` from `PATH`
- restarts the application only when it exits with code `1`

It also prints simple startup diagnostics to standard output so kiosk and autostart failures are easier to debug on-device.

The companion `fairwindsk-startup.desktop` file is now a fuller desktop-entry definition with a stable name/comment, `Terminal=false`, disabled startup notifications, and explicit autostart enablement so it behaves more consistently across Raspberry Pi OS desktop sessions.

For the Raspberry Pi OS applications menu, `fairwindsk.desktop` provides a normal launcher entry for FairWindSK itself. Unlike the autostart entry, it launches `FairWindSK` directly and is intended to be copied into the desktop environment's application-menu directory.

The CMake install flow now generates installed variants of both desktop entries with absolute `Exec=` paths pointing at the installed binaries. On Linux desktop installs:

- `fairwindsk.desktop` is installed into the standard XDG applications directory together with a `fairwindsk` launcher icon
- Raspberry Pi OS detection triggers automatic installation of `fairwindsk-startup.desktop` into `/etc/xdg/autostart` (respecting `DESTDIR` for package builds)
- OpenPlotter detection triggers a best-effort copy of the launcher entry into any known OpenPlotter menu directories that exist on the target system

Because OpenPlotter packaging conventions vary across images, the OpenPlotter integration is intentionally best-effort rather than hard-coded to a single menu path.
