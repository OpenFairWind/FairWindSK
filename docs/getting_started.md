# Getting started with FairWindSK

FairWindSK is a Qt6-based desktop that launches and supervises Signal K web applications, plus a handful of native bars (autopilot, anchor, person overboard, alarms, and status overlays). The application is written in C++17, uses Qt WebEngine for rendering, and connects directly to a running Signal K server to discover installed web apps.

## Prerequisites

- A running Signal K server reachable on the network. The client expects the standard API to be exposed at `<server>/signalk` and the web application catalog at `<server>/skServer/webapps`.
- Qt 6 with WebEngine, WebSockets, SVG, and VirtualKeyboard modules (see the package list in the root README for Linux and macOS examples). A C++17 compiler and CMake are required for builds from source.
- Optional: desktop environments that support global hotkeys if you rely on the SHIFT+TAB shortcut to return to the FairWindSK desktop after launching a web app.

## Building from source

The project uses a conventional CMake workflow. The steps below mirror the platform instructions in the root README and work across macOS, Linux, and Raspberry Pi OS.

1. Install Qt6 and the build toolchain (CMake, a C++17 compiler, and the Qt6 WebEngine, WebSockets, SVG, and VirtualKeyboard components).
2. Clone the repository and create a build directory:
   ```bash
   git clone https://github.com/OpenFairWind/FairWindSK.git
   cd FairWindSK
   cmake -S . -B build
   cmake --build build
   ```
3. On Raspberry Pi OS you may need to create `/usr/lib/aarch64-linux-gnu/cmake/Qt6VirtualKeyboard/Qt6VirtualKeyboardConfigVersionImpl.cmake` before configuring, matching the workaround in the main README.
4. The resulting executable is `build/FairWindSK` (or `FairWindSK.exe` on Windows builds).

## First run and configuration bootstrap

1. Launch the binary. On first run the app reads `fairwindsk.ini` (created in the working directory by Qt) to determine the configuration file path and debug flag. If no path is stored, it defaults to `fairwindsk.json` beside the executable.
2. If the JSON configuration file does not exist, the application seeds its settings from the bundled `resources/json/configuration.json`, preserving defaults such as UI sizing, unit choices, and a starter app list.
3. Point the client at your Signal K server by editing the `connection.server` field in `fairwindsk.json` or by letting a platform integration layer provide the file.
4. FairWindSK connects to the server, negotiates a token if available, and persists the token in `fairwindsk.ini` for subsequent launches.
5. Discovered Signal K web applications are merged into the local configuration, preserving local overrides (ordering, activation state, and custom icons). Web apps and locally defined URLs (including `file://` entries for native tools such as OpenCPN) appear on the desktop.

## Running the desktop

- Start the application normally from the build output folder or after installing it into your system path. On Raspberry Pi OS the project includes a sample autostart entry in `extras/fairwindsk-startup.desktop` and a helper script in `extras/fairwindsk-startup`.
- The splash screen shows connection progress while the Signal K client initializes and downloads the web app catalog.
- Once the main window appears, use the desktop tiles to launch apps. SHIFT+TAB brings you back to the FairWindSK desktop when a web app takes full focus.
- The bottom bars expose quick controls for alarms, person overboard, anchor, and autopilot features. Availability depends on the configured Signal K data paths and installed plugins.

## Troubleshooting

- If no applications appear, verify the `connection.server` URL and that the Signal K server exposes `/skServer/webapps`.
- Enable debug logging by setting `debug=true` in `fairwindsk.ini` before launching. Logs include connection attempts and application discovery details.
- Check that your platform has Qt WebEngine acceleration enabled; `QWebEngineSettings::Accelerated2dCanvasEnabled` is turned on by default at runtime.
