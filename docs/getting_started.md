# Getting started with FairWindSK

FairWindSK is a Qt6 shell that launches and supervises Signal K web applications, plus a handful of native bars (autopilot, anchor, person overboard, alarms, and status overlays). The application is written in C++17, uses Qt WebEngine Widgets on desktop builds and a Qt WebView based alternative on Android/iOS, and connects directly to a running Signal K server to discover installed web apps. Android builds support Android 13/API 33 and newer and can optionally serve as the device Home app.

## Prerequisites

- A running Signal K server reachable on the network. The client expects the standard API to be exposed at `<server>/signalk` and the web application catalog at `<server>/signalk/v1/apps/list`.
- Qt 6 with the modules listed in [building.md](./building.md). A C++17 compiler and CMake are required for builds from source.
- Optional on desktop targets: an environment that supports global hotkeys if you rely on the `SHIFT+TAB` shortcut to return to the FairWindSK desktop after launching a web app.

## Building from source

Clone the source, then select the dedicated instructions for the target flavor:

```bash
git clone --branch mobile https://github.com/OpenFairWind/FairWindSK.git
cd FairWindSK
```

The [cross-platform build overview](building.md) links to the authoritative [macOS](macos.md), [Android](android.md), [iOS/iPadOS](ios.md), and [container](container.md) guides and contains the Linux, Raspberry Pi OS, and Windows workflows. A generic host CMake command is valid only after the correct platform Qt kit and dependencies are selected.

## First run and configuration bootstrap

1. Launch the binary. On first run the app reads `fairwindsk.ini` from the per-user FairWindSK configuration directory to determine the configuration file path and debug flag. If no path is stored, it defaults to `fairwindsk.json` in that same directory.
2. If the JSON configuration file does not exist, the application seeds its settings from the bundled `resources/json/configuration.json`, preserving defaults such as UI sizing, unit choices, and a starter app list.
3. Point the client at your Signal K server by editing the `connection.server` field in `fairwindsk.json` or by letting a platform integration layer provide the file.
4. FairWindSK connects to the server, negotiates a token if available, and persists the token in `fairwindsk.ini` for subsequent launches.
5. Discovered Signal K web applications are merged into the local configuration, preserving local overrides (ordering, activation state, and custom icons). Web apps appear on all targets; native `file://` app entries are blocked by single-window mode.
6. If the Signal K server restarts later, FairWindSK now attempts to recover automatically by rediscovering the server, reconnecting the websocket stream, restoring subscriptions, and refreshing server-backed resources.

On Android, open **Settings > Android**. Use the launcher-mode touch checkbox to ask Android to start FairWindSK as Home, or clear it and choose another Home app for regular-application operation. Tap an application icon to launch it immediately, or use its separate high-contrast checkbox to make it available under **Settings > Applications** for placement beside Signal K apps. Android entries are removed from launcher slots when deselected. When no corresponding hardware keys exist, launcher mode supplies soft Back, FairWindSK Home, and recently launched Android-app controls in the Bottom Bar. See [the Android guide](android.md).

## Running the desktop

- Start the application normally from the build output folder or after installing it into your system path. On Raspberry Pi OS the project includes a sample autostart entry in `extras/fairwindsk-startup.desktop` and a helper script in `extras/fairwindsk-startup`. The helper follows the same per-user Qt configuration directory used by the application, applies the virtual keyboard environment only when `main.virtualKeyboard` is enabled, and only relaunches FairWindSK when the application exits with code `1`. Direct launches now read the same startup setting too, but changing `main.virtualKeyboard` still requires an application restart.
- `cmake --install build` now installs the desktop app together with the bundled icon directory and desktop helper libraries, so the installed target keeps the same local-app icon lookup behavior as the build-tree run.
- On Raspberry Pi OS, `cmake --install build` now also installs the system autostart entry automatically. If OpenPlotter is present on the target, the install performs a best-effort addition of the FairWindSK launcher icon to the OpenPlotter menu.
- The main window appears directly and then performs deferred Signal K startup, app loading, and page prewarming inside the single-window shell.
- Use the tiles to launch apps. On desktop targets, `SHIFT+TAB` brings the FairWindSK window back to the foreground when a web app takes full focus.
- The bottom bars expose quick controls for alarms, person overboard, anchor, and autopilot features. Availability depends on the configured Signal K data paths and installed plugins.

## Troubleshooting

- If no applications appear, verify the `connection.server` URL and that the Signal K server exposes `/signalk/v1/apps/list` or the legacy `/skServer/webapps` endpoint.
- Enable debug logging by setting `debug=true` in `fairwindsk.ini` before launching. Logs include connection attempts and application discovery details.
- For diagnostics, the **Settings > System** tab lets you choose the FairWindSK log level, keep per-run message logs in a persistent directory, and configure the diagnostics email destination used after an unclean shutdown.
- On desktop, check that Qt WebEngine acceleration is available; `QWebEngineSettings::Accelerated2dCanvasEnabled` is turned on by default. Android and iOS/iPadOS use Qt WebView instead, so inspect the platform WebView logs and guide.
