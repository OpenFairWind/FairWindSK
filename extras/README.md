Autostart on Raspberry Pi

`fairwindsk-startup` is a small Raspberry Pi OS helper that:

- finds `fairwindsk.ini` in the same per-user configuration locations used by the application
- resolves the JSON configuration path from that INI file, with legacy fallbacks for older installs
- enables `QT_IM_MODULE=qtvirtualkeyboard` only when `main.virtualKeyboard` is enabled in `fairwindsk.json`
- launches `FairWindSK` from `PATH`
- restarts the application only when it exits with code `1`

It also prints simple startup diagnostics to standard output so kiosk and autostart failures are easier to debug on-device.
