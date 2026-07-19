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
- Bundled application icons are compiled into the Qt resources as well as copied beside the desktop executable, so Raspberry Pi OS launchers can fall back to embedded artwork if the runtime icon directory is missing or the Signal K catalog is still settling.
- Raspberry Pi OS builds use explicit work-area geometry for `maximized` and a frameless topmost kiosk request for `fullscreen`. On Linux ARM, FairWindSK also clamps the window to the connected DRM display mode when the desktop reports a virtual width that is wider than the actual panel. On Bookworm/labwc desktops, the panel can still be policy-controlled by the desktop session; for dedicated MFD/kiosk installations, set `autohide=true` in `~/.config/wf-panel-pi.ini` or disable the panel autostart if it still overlays the application.
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
- Local `file://` launcher entries remain readable for compatibility, but single-window mode blocks them from launching external native applications on Windows.

## Android

Android builds use the native Qt WebView backend while the surrounding application shell stays widget-based and inside one landscape MFD window. The recommended first target is a 64-bit ARM (`arm64-v8a`) APK.

### Step-by-step Android build guide

1. Install Qt Creator and an Android Qt kit. In the Qt Maintenance Tool, select Android arm64-v8a plus Qt WebView, Virtual Keyboard, Positioning, WebSockets, and SVG for the same Qt release.
2. In Qt Creator, open **Preferences > Devices > Android**. Install or select its recommended JDK, Android SDK, Android NDK, SDK Platform-Tools, and SDK Build-Tools. Accept the SDK licenses and resolve every error shown on that page. Do not mix an NDK from a different Qt release.
3. Clone the repository:

```bash
git clone https://github.com/OpenFairWind/FairWindSK.git
cd FairWindSK
```

4. Open `CMakeLists.txt` in Qt Creator and select a kit such as **Android Qt 6.x.x Clang arm64-v8a**.
5. Build with **Build > Build Project**. The mobile configuration uses Qt WebView, Quick, and Quick Widgets and excludes Qt WebEngine, Print Support, QtZeroConf, and QHotkey.
6. Connect an authorized USB-debugging device or start an emulator, select it under **Projects > Run**, and use **Build > Run**. Qt Creator builds, debug-signs, installs, and launches the APK.
7. For a shareable artifact, use Qt Creator's **Build APK** packaging action, then inspect the build directory for the generated APK. A debug APK is only for testing; configure a protected release keystore in Qt Creator before distributing a release build.

### Command-line APK build

Use the exact SDK, NDK, JDK, and Qt kit paths shown by Qt Creator. These example environment-variable names avoid modifying system-owned variables:

```bash
export FAIRWINDSK_QT_ANDROID=/path/to/Qt/6.8.3/android_arm64_v8a
export FAIRWINDSK_ANDROID_SDK=/path/to/Android/Sdk
export FAIRWINDSK_ANDROID_NDK=/path/to/Android/Sdk/ndk/26.1.10909125
export FAIRWINDSK_JAVA_HOME=/path/to/jdk-17
export PATH="$FAIRWINDSK_JAVA_HOME/bin:$PATH"
```

Verify the selected tools:

```bash
"$FAIRWINDSK_JAVA_HOME/bin/java" -version
"$FAIRWINDSK_ANDROID_SDK/platform-tools/adb" version
"$FAIRWINDSK_QT_ANDROID/bin/qt-cmake" --version
```

Configure, compile, and package a Release APK in a dedicated build directory:

```bash
"$FAIRWINDSK_QT_ANDROID/bin/qt-cmake" \
    -S . \
    -B build-android-arm64 \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_SDK_ROOT="$FAIRWINDSK_ANDROID_SDK" \
    -DANDROID_NDK="$FAIRWINDSK_ANDROID_NDK" \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build-android-arm64 --parallel
cmake --build build-android-arm64 --target apk
find build-android-arm64 -type f -name '*.apk' -print
```

The first clean build fetches the pinned Android OpenSSL packaging helper and the `nlohmann/json` fallback when it is not installed. The exact APK subdirectory varies between Qt releases, so use the reported `find` result rather than assuming a fixed path.

Enable Developer Options and USB debugging, authorize the computer, and install the reported APK:

```bash
"$FAIRWINDSK_ANDROID_SDK/platform-tools/adb" devices
"$FAIRWINDSK_ANDROID_SDK/platform-tools/adb" install -r /absolute/path/to/FairWindSK.apk
```

For Play Store delivery, configure release signing outside the repository and build an Android App Bundle:

```bash
cmake --build build-android-arm64 --target aab
```

Never commit a keystore or signing passwords. Retain the same protected release key for all future upgrades.

### Runtime logging and connectivity

Clear old logs, reproduce a problem, and collect the Android log:

```bash
"$FAIRWINDSK_ANDROID_SDK/platform-tools/adb" logcat -c
"$FAIRWINDSK_ANDROID_SDK/platform-tools/adb" logcat | grep -i -E 'fairwindsk|qt|chromium'
```

Use the Signal K server's LAN address in FairWindSK; `localhost` on Android refers to the Android device, not the development computer. Cleartext `http://` and `ws://` remain enabled for trusted vessel LANs, but TLS with a trusted certificate is preferred and an unauthenticated Signal K server must never be exposed to an untrusted network.

### Packaging and permission notes

- Package identifier: `org.openfairwind.fairwindsk`.
- The version name follows the CMake/Git version and the monotonically increasing Android version code follows the Git commit count.
- Adaptive and legacy launcher icons are generated from the FairWindSK logo.
- Application backup is disabled because configuration can reference private vessel endpoints and authentication state.
- The obsolete broad external-storage permission is not requested. Android file access must use Qt-supported app storage or a document-picker/scoped-storage workflow.
- Location hardware is optional, allowing installation on tablets without GPS; Android still controls runtime permission grants.
- The soft keyboard resizes the window instead of covering operational controls.
- Desktop Linux startup fallbacks are excluded explicitly from Android even on toolchains that define Linux compatibility macros.
- The Android TLS helper is pinned to a commit so clean builds are reproducible.

- Desktop-only integrations such as `QHotkey`, Zeroconf browser discovery, and the shared `QWebEngineProfile` cookie path remain disabled on Android. Native `file://` launcher apps are blocked on every target by the single-window model.
- Embedded previews and web apps still load inside the application, but advanced desktop WebEngine-specific hooks are intentionally not compiled into the Android target.

### Physical-device MFD checklist

An APK cross-build validates compilation and packaging, but it cannot validate WebView, keyboard, safe-area, GPS, Wi-Fi, or helm-distance behavior. Before distribution, test at least one physical tablet and a representative phone or emulator:

1. The FairWindSK logo is sharp in the launcher, recent-apps view, and Android settings.
2. FairWindSK remains one landscape window and hosted apps never open an external browser.
3. Top and Bottom Bars remain readable, stable, and finger-friendly at normal helm distance.
4. Drawers and the virtual keyboard do not obscure primary actions.
5. Signal K REST, websocket, web apps, and reconnect after Wi-Fi interruption work on the vessel LAN.
6. `default`, `dawn`, `day`, `sunset`, `dusk`, and `night` presets retain high contrast and hierarchy.
7. English and Italian shell text and hosted-web locale remain consistent, readable, and touch-friendly.
8. POB, Apps, MyData, Settings, alarms, and installed autopilot integration preserve the single-window MFD workflow.
9. Rotation attempts do not disturb landscape layout and Android back navigation does not accidentally exit an operational dialog.
10. The release-signed APK installs both cleanly and as an update over the previous release.

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

- Desktop-only integrations such as `QHotkey`, Zeroconf browser discovery, and the shared `QWebEngineProfile` cookie path remain disabled on iOS and iPadOS. Native `file://` launcher apps are blocked on every target by the single-window model.
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
