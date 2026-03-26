# Building FairWindSK

FairWindSK currently targets desktop Qt kits. The project builds and runs on macOS, Linux, Raspberry Pi OS, and Windows from the same CMake codebase.

Android and iOS are not currently supported build targets. The application depends on Qt WebEngine Widgets and desktop-style widget flows, and the project now fails fast at CMake configure time on those mobile targets instead of advertising a broken path.

## Support matrix

| Platform | Status | Notes |
| --- | --- | --- |
| macOS | Supported | Full desktop feature set. |
| Linux | Supported | Full desktop feature set. |
| Raspberry Pi OS | Supported | Same desktop feature set, plus kiosk/autostart helpers in `extras/`. |
| Windows | Supported | Full desktop feature set, with `windeployqt` recommended after building. |
| Android | Not supported | Blocked by the current Qt WebEngine Widgets based UI architecture. |
| iOS | Not supported | Blocked by the current Qt WebEngine Widgets based UI architecture. |

## Common desktop requirements

- CMake 3.16 or newer
- A C++17 compiler supported by the selected Qt 6 desktop kit
- Git
- Qt 6 desktop modules:
  - `Core`
  - `Gui`
  - `Widgets`
  - `Concurrent`
  - `Network`
  - `WebSockets`
  - `Xml`
  - `Svg`
  - `Positioning`
  - `WebEngineWidgets`
  - `VirtualKeyboard`
  - `PrintSupport`
- Internet access during the first clean desktop build, because CMake downloads:
  - `nlohmann/json` header fallback
  - `QtZeroConf`
  - `QHotkey`

If internet access is restricted, install `nlohmann_json` from your platform package manager first so CMake can use the system header instead of downloading it.

The desktop dependency revisions are now pinned in CMake for reproducible builds.

## Generic desktop workflow

```bash
git clone https://github.com/OpenFairWind/FairWindSK.git
cd FairWindSK
cmake -S . -B build
cmake --build build --parallel
```

Run from the build tree:

- macOS: `open build/FairWindSK.app`
- Linux / Raspberry Pi OS: `./build/FairWindSK`
- Windows: `build\\FairWindSK.exe`

If Qt is installed outside the default search path, pass `-DCMAKE_PREFIX_PATH=...` during configure.

## macOS

Recommended toolchain:

- Xcode command line tools
- Qt 6 desktop kit
- Ninja or the default CMake generator

Example:

```bash
brew install qt cmake ninja nlohmann-json
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build --parallel
open build/FairWindSK.app
```

Notes:

- The build uses the CMake build-tree rpath so the app can run from the build folder with the downloaded desktop dependencies.
- For redistribution outside the build tree, use `macdeployqt` on the generated app bundle.

## Linux

Example package set for Debian or Ubuntu:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build git \
  nlohmann-json3-dev \
  qt6-base-dev qt6-base-dev-tools qt6-webengine-dev qt6-webengine-dev-tools \
  qt6-websockets-dev qt6-positioning-dev libqt6svg6-dev qt6-virtualkeyboard-dev \
  qt6-base-private-dev libqt6printsupport6 libavahi-compat-libdnssd-dev \
  libnss-mdns avahi-utils
```

Build and run:

```bash
cmake -S . -B build -G Ninja
cmake --build build --parallel
./build/FairWindSK
```

Notes:

- The desktop dependency libraries are added to the build-tree rpath, so running from `build/` works without setting `LD_LIBRARY_PATH`.
- For packaging, use your normal Linux deployment workflow and include the Qt runtime plus the desktop dependencies built under `build/external/lib`.

## Raspberry Pi OS

Raspberry Pi OS follows the Linux desktop path. On Bookworm-based images a broader Qt installation is usually needed:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build git \
  nlohmann-json3-dev \
  qml6-module-qt-labs-folderlistmodel qml6-module-qtquick-window \
  qml6-module-qtquick-layouts qml6-module-qtqml-workerscript \
  libnss-mdns avahi-utils libavahi-compat-libdnssd-dev libxkbcommon-dev \
  qt6-base-dev qt6-base-dev-tools qt6-webengine-dev qt6-webengine-dev-tools \
  qt6-websockets-dev qt6-positioning-dev libqt6svg6-dev \
  qt6-virtualkeyboard-dev libqt6virtualkeyboard6 qt6-virtualkeyboard-plugin
```

Build and run:

```bash
cmake -S . -B build -G Ninja
cmake --build build --parallel
./build/FairWindSK
```

Notes:

- If your distribution package misses `Qt6VirtualKeyboardConfigVersionImpl.cmake`, the existing workaround still applies:

```bash
sudo touch /usr/lib/aarch64-linux-gnu/cmake/Qt6VirtualKeyboard/Qt6VirtualKeyboardConfigVersionImpl.cmake
```

- For kiosk deployments, see `extras/fairwindsk-startup.desktop` and `extras/fairwindsk-startup`.

## Windows

Recommended toolchain:

- Qt 6 desktop kit for MSVC
- Visual Studio Build Tools or Visual Studio Community
- CMake and Ninja

Example from a developer prompt with the compiler environment already loaded:

```powershell
git clone https://github.com/OpenFairWind/FairWindSK.git
cd FairWindSK
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64"
cmake --build build --parallel
build\FairWindSK.exe
```

Deploy the Qt runtime after a successful build:

```powershell
windeployqt build\FairWindSK.exe
```

Notes:

- The CMake build handles `QtZeroConf` and `QHotkey` as desktop-only dependencies and copies their DLLs next to the executable after the build.
- Local `file://` applications remain a desktop-only integration and are supported on Windows.

## Android

Android is not currently a supported target for this project.

Reason:

- The codebase depends on `QtWebEngineWidgets` and several desktop-widget based flows that are not maintained here as an Android-compatible UI stack.

Current behavior:

- CMake stops during configure with an explicit error instead of pretending the build is supported.

## iOS

iOS is not currently a supported target for this project.

Reason:

- The codebase depends on `QtWebEngineWidgets` and desktop-style widget flows that are not maintained here as an iOS-compatible UI stack.

Current behavior:

- CMake stops during configure with an explicit error instead of pretending the build is supported.

## Runtime and deployment notes

- Desktop builds copy the bundled icon directory to the target output folder after the build.
- The first clean desktop build requires internet access for third-party dependency download steps.
- Desktop external dependencies are pinned in CMake so the same revisions are used across machines.
- For redistribution:
  - macOS: use `macdeployqt`
  - Windows: use `windeployqt`
  - Linux / Raspberry Pi OS: package the Qt runtime plus `build/external/lib`
