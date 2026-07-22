# Building FairWindSK

FairWindSK has one C++17/Qt 6 source tree and six supported operating-system flavors. This page defines the common build contract and routes each platform to its authoritative guide. Platform guides own detailed setup and commands; they should not be duplicated here.

## Supported flavors

| Flavor | Host/target | Embedded web backend | Authoritative instructions |
| --- | --- | --- | --- |
| macOS | macOS desktop | Qt WebEngine Widgets | [macOS](macos.md) |
| Linux | Linux desktop | Qt WebEngine Widgets | [Linux](linux.md) |
| Raspberry Pi OS | Linux ARM desktop/MFD | Qt WebEngine Widgets | [Raspberry Pi OS](raspberrypi.md) |
| Windows | Windows desktop | Qt WebEngine Widgets | [Windows](windows.md) |
| Android | Android 13/API 33 or newer | Qt WebView in QQuickWidget | [Android](android.md) |
| iOS/iPadOS | Apple mobile devices/simulator | Qt WebView in QQuickWidget | [iOS/iPadOS](ios.md) |
| Linux container | Docker Linux desktop check | Qt WebEngine Widgets | [Container](container.md) |

All flavors preserve the single-window, touch-first marine Multi-Functional Display shell. A successful build on one flavor does not establish compatibility on the others.

## Shared source and build rules

- Use an out-of-source build directory dedicated to one platform, architecture, Qt kit, generator, and build type.
- Use CMake 3.16 or newer and a compiler with C++17 support.
- Desktop kits require Qt Core, Gui, Widgets, Qml, Concurrent, Network, WebSockets, Xml, Svg, SvgWidgets, Positioning, WebEngine Widgets, Virtual Keyboard, Print Support, and LinguistTools.
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

Follow the dedicated [Linux guide](linux.md) for distribution packages, native
configuration, compilation, CTest, build-tree execution, staging, installation,
desktop integration, troubleshooting, and validation. Use the separate
[container guide](container.md) only for reproducible Linux compile and headless
startup checks.

## Raspberry Pi OS

Raspberry Pi OS follows the Linux desktop Qt WebEngine path but has additional
ARM rendering, constrained-build, touch-display, autostart, and OpenPlotter
requirements. Follow the dedicated [Raspberry Pi OS guide](raspberrypi.md) for
64-bit system preparation, package installation, native compilation, CTest,
Signal K configuration, kiosk operation, system installation, troubleshooting,
and the physical-device validation checklist.

## Windows

Windows uses the desktop Qt WebEngine Widgets path and requires the MSVC 2022
64-bit Qt kit; MinGW is intentionally rejected at configure time. Follow the
dedicated [Windows build guide](windows.md) for prerequisites, Qt Creator and
command-line workflows, tests, deployment with `windeployqt`, troubleshooting,
and the platform validation checklist.

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
