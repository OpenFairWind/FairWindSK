# Building FairWindSK

FairWindSK has one C++17/Qt 6 source tree and six supported operating-system flavors. This page defines the common build contract and routes each platform to its authoritative guide. Platform guides own detailed setup and commands; they should not be duplicated here.

## Supported flavors

| Flavor | Host/target | Embedded web backend | Authoritative instructions |
| --- | --- | --- | --- |
| macOS | macOS desktop | Qt WebEngine Widgets | [macOS](macos.md) |
| Linux | Linux desktop | Qt WebEngine Widgets | [Linux below](#linux) |
| Raspberry Pi OS | Linux ARM desktop/MFD | Qt WebEngine Widgets | [Raspberry Pi OS below](#raspberry-pi-os) |
| Windows | Windows desktop | Qt WebEngine Widgets | [Windows below](#windows) |
| Android | Android 13/API 33 or newer | Qt WebView in QQuickWidget | [Android](android.md) |
| iOS/iPadOS | Apple mobile devices/simulator | Qt WebView in QQuickWidget | [iOS/iPadOS](ios.md) |
| Linux container | Docker Linux desktop check | Qt WebEngine Widgets | [Container](container.md) |

All flavors preserve the single-window, touch-first marine Multi-Functional Display shell. A successful build on one flavor does not establish compatibility on the others.

## Shared source and build rules

- Use an out-of-source build directory dedicated to one platform, architecture, Qt kit, generator, and build type.
- Use CMake 3.16 or newer and a compiler with C++17 support.
- Desktop kits require Qt Core, Gui, Widgets, Concurrent, Network, WebSockets, Xml, Svg, Positioning, WebEngine Widgets, Virtual Keyboard, and Print Support.
- Mobile kits require Qt Core, Gui, Widgets, Qml, Quick, Quick Widgets, WebView, Concurrent, Network, WebSockets, Xml, Svg, Positioning, and Virtual Keyboard.
- Desktop builds include pinned QtZeroConf and QHotkey helpers. They may also download the pinned `nlohmann/json` fallback when a system package is unavailable.
- Mobile builds exclude Qt WebEngine Widgets, Print Support, QtZeroConf, and QHotkey.
- Do not report a full desktop build as successful when required dependency downloads were skipped or unavailable. Supply the dependencies or restore network access and complete the build.
- Never commit signing keys, provisioning profiles, passwords, generated build trees, or packaged credentials.

## Common repository workflow

```bash
git clone --branch mobile https://github.com/OpenFairWind/FairWindSK.git
cd FairWindSK
```

Use a release tag instead of `mobile` when building a published release. Then follow the platform-specific guide. The generic desktop sequence is:

```bash
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --install build
```

Pass the selected Qt prefix with `-DCMAKE_PREFIX_PATH=/path/to/qt` when it is not discoverable automatically.

## macOS

Follow [Building FairWindSK on macOS](macos.md) for Xcode tools, Homebrew or Qt Online Installer setup, configuration, tests, staging, `macdeployqt`, signing, notarization notes, and the macOS validation checklist.

## Linux

Install the desktop dependencies on Debian/Ubuntu:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build git \
  nlohmann-json3-dev libavahi-compat-libdnssd-dev libnss-mdns avahi-utils \
  qt6-base-dev qt6-base-dev-tools qt6-webengine-dev qt6-webengine-dev-tools \
  qt6-websockets-dev qt6-positioning-dev libqt6svg6-dev \
  qt6-virtualkeyboard-dev libqt6printsupport6
```

Configure, build, test, and run:

```bash
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux --parallel
ctest --test-dir build-linux --output-on-failure
./build-linux/FairWindSK
```

Install to a staging prefix before packaging:

```bash
cmake -S . -B build-linux -DCMAKE_INSTALL_PREFIX="$PWD/stage-linux"
cmake --install build-linux
```

The install includes the executable, icon resources, desktop helper libraries, and XDG launcher metadata. Package the matching Qt runtime and the helper libraries under the build/install library directory. Use [container.md](container.md) for a reproducible Ubuntu compile and headless startup check.

## Raspberry Pi OS

Raspberry Pi OS follows the Linux desktop path. Bookworm installations commonly need:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build git nlohmann-json3-dev \
  libnss-mdns avahi-utils libavahi-compat-libdnssd-dev libxkbcommon-dev \
  qt6-base-dev qt6-base-dev-tools qt6-webengine-dev qt6-webengine-dev-tools \
  qt6-websockets-dev qt6-positioning-dev libqt6svg6-dev \
  qt6-virtualkeyboard-dev libqt6virtualkeyboard6 qt6-virtualkeyboard-plugin \
  qml6-module-qt-labs-folderlistmodel qml6-module-qtquick-window \
  qml6-module-qtquick-layouts qml6-module-qtqml-workerscript
```

If the distribution omits `Qt6VirtualKeyboardConfigVersionImpl.cmake` while providing the rest of the package:

```bash
sudo touch /usr/lib/aarch64-linux-gnu/cmake/Qt6VirtualKeyboard/Qt6VirtualKeyboardConfigVersionImpl.cmake
```

Build with the Linux commands above. Installation uses the Linux layout and additionally installs the Raspberry Pi startup integration. The helpers in `extras/` support kiosk/autostart operation and OpenPlotter integration. Validate on actual Raspberry Pi display hardware: a cross-build or container cannot confirm DRM geometry, panel policy, GPU/WebEngine operation, touch behavior, or helm readability.

## Windows

Windows builds use the standard Qt WebEngine Widgets stack, fully supporting the desktop feature set. We recommend using Qt Creator for the smoothest configuration and build experience, though command-line builds are fully supported.

### Step-by-step Windows build guide

1. Install the Windows toolchain prerequisites:
   - Qt 6 with a desktop kit (e.g., MSVC 2022 64-bit)
   - A C++17 compiler (Visual Studio Build Tools for MSVC)
   - CMake and Ninja
   - Git

2. Clone the repository:
```cmd
   git clone https://github.com/OpenFairWind/FairWindSK.git
   cd FairWindSK
```

3. Open Qt Creator and select `File` → `Open File or Project...`, then choose `FairWindSK/CMakeLists.txt`.
4. When Qt Creator asks for kits, select your installed Windows desktop kit (e.g., `Desktop Qt 6.x.x MSVC2022 64bit`).
5. Let Qt Creator configure the project automatically. If the build environment becomes out of sync, right-click the project root, select `Clear CMake Configuration`, then `Run CMake`.
6. Build with `Build` → `Build Project`.
7. Run the application by clicking the Run button.

### Optional command-line build

Open an "x64 Native Tools Command Prompt for VS 2022" and run:

```cmd
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64"
cmake --build build --parallel
cmake --install build
build\FairWindSK.exe
```

Deploy the Qt runtime for redistribution:

```cmd
windeployqt build\FairWindSK.exe
```

### Notes

- The CMake build handles `QtZeroConf` and `QHotkey` as desktop-only dependencies and copies their DLLs next to the executable after the build.
- `cmake --install build` installs `FairWindSK.exe`, the bundled icon directory under `bin\icons`, and the desktop helper DLLs into the same `bin` directory.
- Local `file://` launcher entries remain readable for compatibility, but single-window mode blocks them from launching external native applications on Windows.
- On high-DPI displays or with Windows display scaling enabled (e.g., 125% or 150%), UI elements adapt dynamically. If bottom bar widgets appear clipped, use Qt Layouts with `QSizePolicy` set to `Expanding` rather than hardcoded fixed heights.

## Android

Android 13/API 33 is the minimum runtime. The complete Linux, macOS, and Windows host setup, Qt/SDK/NDK/JDK compatibility rules, APK/AAB build, signing, installation, optional Home role, native launcher integration, debugging, and validation checklist are maintained in [android.md](android.md).

## iOS and iPadOS

Apple mobile builds require macOS, Xcode, matching macOS host tools and Qt iOS libraries, and Apple signing for physical devices. Simulator architecture depends on the selected Qt release. Follow [ios.md](ios.md) for Qt Creator, command-line simulator/device builds, signing, installation, diagnostics, and validation.

## Containers

The [container guide](container.md) builds and smoke-tests only the Linux desktop flavor. It is useful on CI and non-Linux hosts but does not validate macOS, Windows, Raspberry Pi hardware, Android, iOS/iPadOS, visual layout, touch ergonomics, or GPU behavior.

## Cross-platform completion checklist

For every change:

1. Configure and compile every affected flavor with its supported Qt kit.
2. Run desktop CTest where available and device/simulator checks for mobile code.
3. Validate Signal K discovery/manual connection, authentication, REST, websocket updates, reconnect, and server restart recovery.
4. Validate embedded apps, launcher icons/pages, Settings, MyData, POB, alarms, anchor, and available autopilot integration.
5. Preserve the single-window model and finger-friendly marine-MFD interaction.
6. Check `default`, `dawn`, `day`, `sunset`, `dusk`, and `night` comfort presets.
7. Check every shipped translation for readable, unclipped controls.
8. Document any platform that could not be executed; never imply runtime coverage from compilation alone.

## Related documentation

- [Getting started](getting_started.md)
- [Architecture](architecture.md)
- [Developer guide](developing_guide.md)
- [Configuration](configuring.md)
- [Multilingual development](multilingual.md)
