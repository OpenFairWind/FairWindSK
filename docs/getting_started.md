# Getting started with FairWindSK

FairWindSK is a Qt6-based shell that launches and supervises Signal K web applications, plus a handful of native bars (autopilot, anchor, person overboard, alarms, and status overlays). The application is written in C++17, uses Qt WebEngine for rendering, and connects directly to a running Signal K server to discover installed web apps.

## Prerequisites

- A running Signal K server reachable on the network. The client expects the standard API to be exposed at `<server>/signalk` and the web application catalog at `<server>/signalk/v1/apps/list`.
- Qt 6 with the modules listed in [building.md](./building.md). A C++17 compiler and CMake are required for builds from source.
- Optional on desktop targets: an environment that supports global hotkeys if you rely on the `SHIFT+TAB` shortcut to return to the FairWindSK desktop after launching a web app.

## Building from source

The project uses a conventional CMake workflow across all targets:

```bash
git clone https://github.com/OpenFairWind/FairWindSK.git
cd FairWindSK
cmake -S . -B build
cmake --build build --parallel
```

Platform-specific dependencies, Qt kit selection, Windows deployment, Raspberry Pi notes, and Android/iOS caveats are documented in [building.md](./building.md).

## First run and configuration bootstrap

1. Launch the binary. On first run the app reads `fairwindsk.ini` (created in the working directory by Qt) to determine the configuration file path and debug flag. If no path is stored, it defaults to `fairwindsk.json` beside the executable.
2. If the JSON configuration file does not exist, the application seeds its settings from the bundled `resources/json/configuration.json`, preserving defaults such as UI sizing, unit choices, and a starter app list.
3. Point the client at your Signal K server by editing the `connection.server` field in `fairwindsk.json` or by letting a platform integration layer provide the file.
4. FairWindSK connects to the server, negotiates a token if available, and persists the token in `fairwindsk.ini` for subsequent launches.
5. Discovered Signal K web applications are merged into the local configuration, preserving local overrides (ordering, activation state, and custom icons). Web apps appear on all targets; `file://` desktop-native app entries are desktop-only.

## Running the desktop

- Start the application normally from the build output folder or after installing it into your system path. On Raspberry Pi OS the project includes a sample autostart entry in `extras/fairwindsk-startup.desktop` and a helper script in `extras/fairwindsk-startup`.
- The splash screen shows connection progress while the Signal K client initializes and downloads the web app catalog.
- Once the main window appears, use the tiles to launch apps. On desktop targets, `SHIFT+TAB` brings you back to the FairWindSK desktop when a web app takes full focus.
- The bottom bars expose quick controls for alarms, person overboard, anchor, and autopilot features. Availability depends on the configured Signal K data paths and installed plugins.

## Troubleshooting

- If no applications appear, verify the `connection.server` URL and that the Signal K server exposes `/signalk/v1/apps/list` or the legacy `/skServer/webapps` endpoint.
- Enable debug logging by setting `debug=true` in `fairwindsk.ini` before launching. Logs include connection attempts and application discovery details.
- Check that your platform has Qt WebEngine acceleration enabled; `QWebEngineSettings::Accelerated2dCanvasEnabled` is turned on by default at runtime.
