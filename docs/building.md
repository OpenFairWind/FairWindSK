# Building FairWindSK

FairWindSK uses a single Qt/CMake codebase across desktop and mobile builds. Desktop targets keep the existing Qt WebEngine Widgets stack, while Android and iOS build through an alternate Qt WebView based implementation that preserves the surrounding widget UI.

## Support matrix

| Platform | Status | Notes |
| --- | --- | --- |
| macOS | Supported | Full desktop feature set. |
| Linux | Supported | Full desktop feature set. |
| Raspberry Pi OS | Supported | Same desktop feature set, plus kiosk/autostart helpers in `extras/`. |
| Windows | Supported | Full desktop feature set, with `windeployqt` recommended after building. |
| Android | Supported | Uses the alternate Qt WebView based mobile implementation instead of Qt WebEngine Widgets. |
| iOS | Supported | Uses the alternate Qt WebView based mobile implementation instead of Qt WebEngine Widgets. |

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

## Common mobile requirements

- CMake 3.16 or newer
- A Qt 6 mobile kit with:
  - `Core`
  - `Gui`
  - `Widgets`
  - `Qml`
  - `Quick`
  - `QuickWidgets`
  - `WebView`
  - `Concurrent`
  - `Network`
  - `WebSockets`
  - `Xml`
  - `Svg`
  - `Positioning`
  - `VirtualKeyboard`

The mobile build does not pull in the desktop-only external dependencies (`QtZeroConf`, `QHotkey`, `PrintSupport`, or `QtWebEngineWidgets`).

## Generic desktop workflow

```bash
git clone https://github.com/OpenFairWind/FairWindSK.git
cd FairWindSK
cmake -S . -B build
cmake --build build --parallel
cmake --install build
```

Run from the build tree:

- macOS: `open build/FairWindSK.app`
- Linux / Raspberry Pi OS: `./build/FairWindSK`
- Windows: `build\\FairWindSK.exe`

Install into the default prefix:

- macOS: `/usr/local/FairWindSK.app` (or the active `CMAKE_INSTALL_PREFIX`)
- Linux / Raspberry Pi OS: `/usr/local/bin/FairWindSK`
- Windows: `<prefix>\\bin\\FairWindSK.exe`

Override the install prefix when needed:

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/desired/prefix
cmake --build build --parallel
cmake --install build
```

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
- `cmake --install build` installs `FairWindSK.app`, preserves the bundled icon directory inside the app bundle, and installs the desktop helper libraries inside `Contents/Frameworks`.
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
- `cmake --install build` installs the executable to `${CMAKE_INSTALL_BINDIR}`, the bundled icon directory to `${CMAKE_INSTALL_BINDIR}/icons`, and the desktop helper libraries to `${CMAKE_INSTALL_LIBDIR}`.
- On Linux desktop targets, `cmake --install build` also installs a `fairwindsk.desktop` launcher and a `fairwindsk.png` menu icon into the standard XDG applications/icon locations under the chosen prefix.
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
- The `fairwindsk-startup` helper now waits briefly for the configured Signal K web-app catalog to come online before launching FairWindSK, which improves plugin icon/artwork availability during Raspberry Pi autostart boots.
- `cmake --install build` uses the same Linux desktop install layout, so the binary lands in `${CMAKE_INSTALL_BINDIR}` and the icon/runtime helper assets are installed beside it.
- When the install runs on Raspberry Pi OS, the generated `fairwindsk-startup.desktop` entry is also installed to `/etc/xdg/autostart` (respecting `DESTDIR` when packaging) so FairWindSK autostarts automatically after installation.
- When an OpenPlotter installation is detected during install, FairWindSK keeps the normal XDG launcher entry and also performs a best-effort copy into any detected OpenPlotter menu directories.

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
cmake --install build
build\FairWindSK.exe
```

Deploy the Qt runtime after a successful build:

```powershell
windeployqt build\FairWindSK.exe
```

Notes:

- The CMake build handles `QtZeroConf` and `QHotkey` as desktop-only dependencies and copies their DLLs next to the executable after the build.
- `cmake --install build` installs `FairWindSK.exe`, the bundled icon directory under `bin\\icons`, and the desktop helper DLLs into the same `bin` directory.
- Local `file://` applications remain a desktop-only integration and are supported on Windows.

## Android

Android builds use the mobile web implementation based on `Qt::WebView` hosted inside `QQuickWidget` containers, while the surrounding application shell stays widget-based.

### Step-by-step Android build guide

1. Install the Android toolchain prerequisites:
   - Qt 6 with the Android kit you plan to use
   - Android Studio or the Android command-line SDK tools
   - A matching Android NDK supported by your Qt release
   - CMake and Ninja
   - Java JDK, if it is not already provided by Android Studio
2. Open Android Studio once and install:
   - an Android SDK platform
   - Android SDK Platform-Tools
   - Android SDK Build-Tools
   - the NDK version recommended by your Qt kit
3. In Qt Maintenance Tool, install the Android Qt components that match your desktop host and target ABI.
4. In Qt Creator, go to `Preferences` -> `Devices` -> `Android` and verify that the SDK, NDK, JDK, and platform paths are detected correctly.
5. Clone the repository:

```bash
git clone https://github.com/OpenFairWind/FairWindSK.git
cd FairWindSK
```

6. Open `FairWindSK/CMakeLists.txt` in Qt Creator as a CMake project.
7. When Qt Creator asks for kits, select an Android kit such as `Android Qt 6.x.x Clang arm64-v8a`.
8. Let Qt Creator configure the project. The mobile configuration should:
   - use `Qt::WebView`, `Qt::Quick`, and `Qt::QuickWidgets`
   - skip desktop-only dependencies such as `QtZeroConf`, `QHotkey`, `PrintSupport`, and `QtWebEngineWidgets`
9. Build the application from Qt Creator with `Build` -> `Build Project`.
10. Connect an Android device with developer mode enabled, or start an Android emulator.
11. Deploy with `Build` -> `Run`, or use `Projects` -> `Run` to select the target device first.
12. On first launch, confirm that embedded web content opens inside the app and that the main widget shell scales correctly on the device.

### Optional command-line Android configure example

If you prefer configuring outside Qt Creator, use the Android kit paths supplied by your Qt installation and Android SDK:

```bash
cmake -S . -B build-android -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-24 \
  -DCMAKE_PREFIX_PATH="/path/to/Qt/6.x.x/android_arm64_v8a"
cmake --build build-android --parallel
```

Qt Creator is still the recommended path for packaging and deployment because it manages the Android manifest, APK signing flow, and device deployment more smoothly.

Notes:

- Desktop-only integrations such as `QHotkey`, Zeroconf browser discovery, desktop-native `file://` launcher apps, and the shared `QWebEngineProfile` cookie path remain disabled on Android.
- Embedded previews and web apps still load inside the application, but advanced desktop WebEngine-specific hooks are intentionally not compiled into the Android target.

## iOS / iPadOS

iOS and iPadOS builds use the same alternate mobile web implementation as Android: `Qt::WebView` inside `QQuickWidget` hosts, with the rest of the widget shell preserved.

### Step-by-step iOS / iPadOS build guide

1. Use a macOS machine with:
   - Xcode installed
   - Xcode command line tools installed
   - Qt 6 with the iOS kit
   - CMake and Ninja
2. Open Xcode at least once and accept the license if needed.
3. In Qt Maintenance Tool, install the iOS Qt components that match your desktop Qt release.
4. If you plan to deploy to physical hardware, make sure your Apple developer signing identities and provisioning profiles are available in Xcode.
5. Clone the repository:

```bash
git clone https://github.com/OpenFairWind/FairWindSK.git
cd FairWindSK
```

6. Open `FairWindSK/CMakeLists.txt` in Qt Creator as a CMake project.
7. Select an iOS kit such as the Qt 6 iPhoneOS kit during kit selection.
8. Let Qt Creator configure the project for the chosen Apple mobile target.
9. Build the application from Qt Creator with `Build` -> `Build Project`.
10. For simulator testing:
   - choose an iOS Simulator run target
   - start the app with `Build` -> `Run`
11. For device deployment:
   - connect the iPhone or iPad
   - select the physical device in the run target chooser
   - verify signing settings if Qt Creator prompts for them
   - run the project from Qt Creator
12. Validate that the Top Bar, Bottom Bar, Application Area, Bottom Bar horizontal drawer, Application Area vertical drawer, and embedded web content behave correctly on both compact phone layouts and larger iPad layouts.

### Optional command-line iOS configure example

Command-line iOS builds are possible, but Qt Creator remains the recommended workflow because Apple signing, simulator/device selection, and bundle deployment are easier there. A typical configure pattern is:

```bash
cmake -S . -B build-ios -G Ninja \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_PREFIX_PATH="/path/to/Qt/6.x.x/ios"
cmake --build build-ios --parallel
```

For simulator-only builds, use the simulator SDK and matching architecture values instead of `iphoneos`.

Notes:

- Desktop-only integrations such as `QHotkey`, Zeroconf browser discovery, desktop-native `file://` launcher apps, and the shared `QWebEngineProfile` cookie path remain disabled on iOS and iPadOS.
- Embedded previews and web apps still load inside the application, but advanced desktop WebEngine-specific hooks are intentionally not compiled into the Apple mobile targets.

## Runtime and deployment notes

- Desktop builds copy the bundled icon directory to the target output folder after the build.
- Desktop installs now ship the application target, the bundled icon directory, and the desktop-only helper libraries through `cmake --install build`.
- Linux desktop installs also ship the generated desktop launcher and menu icon; Raspberry Pi OS installs additionally enable system autostart through the shipped startup helper.
- The first clean desktop build requires internet access for third-party dependency download steps.
- Desktop external dependencies are pinned in CMake so the same revisions are used across machines.
- For redistribution:
  - macOS: use `macdeployqt`
  - Windows: use `windeployqt`
  - Linux / Raspberry Pi OS: package the Qt runtime plus `build/external/lib`
