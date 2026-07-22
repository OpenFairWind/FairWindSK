# Building FairWindSK on macOS

This is the authoritative step-by-step guide for creating a native macOS build environment, compiling, testing, running, installing, and packaging FairWindSK. The macOS flavor uses Qt WebEngine Widgets; it is distinct from the Qt WebView-based iOS/iPadOS flavor described in [ios.md](ios.md).

## 1. Requirements

- A currently supported macOS release
- An administrator account for installing Xcode command-line tools and Homebrew
- Git
- CMake 3.16 or newer
- Ninja (recommended) or another CMake generator
- A C++17 compiler from Xcode
- Qt 6 for macOS with Core, Gui, Widgets, Qml, Concurrent, Network, WebSockets, Xml, Svg, SvgWidgets, Positioning, WebEngine Widgets, Virtual Keyboard, Print Support, and LinguistTools
- Internet access for the first clean build of pinned desktop helper dependencies

Use one architecture consistently. A native Apple Silicon toolchain produces `arm64`; an Intel Mac produces `x86_64`. Universal packaging requires building both architectures with compatible Qt libraries and is outside the normal single-architecture workflow.

## 2. Install and select Xcode tools

Install the command-line tools:

```bash
xcode-select --install
```

If full Xcode is installed, start it once, accept its license, then select it:

```bash
sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer
sudo xcodebuild -license accept
```

Verify the compiler:

```bash
xcode-select --print-path
clang++ --version
```

## 3. Install Homebrew

Install Homebrew from [brew.sh](https://brew.sh/) if it is not already available. Follow the installer instructions to add `brew` to the shell path, then verify it:

```bash
brew --version
brew doctor
```

`brew doctor` may report advisory warnings that are unrelated to FairWindSK. Resolve errors affecting compilers, CMake, or the Qt prefix before continuing.

## 4. Install the build dependencies

The simplest native desktop environment uses Homebrew Qt:

```bash
brew update
brew install git cmake ninja qt nlohmann-json
```

Homebrew keeps Qt keg-only on some macOS releases. Define a FairWindSK-specific path rather than modifying system paths:

```bash
export FAIRWINDSK_QT_MACOS="$(brew --prefix qt)"
```

Confirm that the required Qt packages are present:

```bash
"$FAIRWINDSK_QT_MACOS/bin/qtpaths" --qt-version
test -d "$FAIRWINDSK_QT_MACOS/lib/cmake/Qt6WebEngineWidgets"
test -d "$FAIRWINDSK_QT_MACOS/lib/cmake/Qt6VirtualKeyboard"
test -d "$FAIRWINDSK_QT_MACOS/lib/cmake/Qt6Positioning"
test -d "$FAIRWINDSK_QT_MACOS/lib/cmake/Qt6WebSockets"
```

Alternatively, install a macOS desktop kit with the Qt Online Installer. Select the same modules listed above and set `FAIRWINDSK_QT_MACOS` to that kit, for example `$HOME/Qt/6.8.3/macos`. Do not point a macOS build at an iOS Qt kit.

## 5. Clone FairWindSK

Clone the current integrated source line:

```bash
git clone --branch main https://github.com/OpenFairWind/FairWindSK.git
cd FairWindSK
```

For a published version, replace `--branch main` with its release tag.

## 6. Configure a native macOS build

Keep every platform and Qt kit in its own build directory:

```bash
cmake -S . -B build-macos -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$FAIRWINDSK_QT_MACOS"
```

The first clean desktop configure/build may download the pinned QtZeroConf and QHotkey sources. `nlohmann-json` is taken from Homebrew when available; otherwise CMake uses its pinned fallback. Mobile-only dependencies are not used by this build.

For a debuggable build, use a separate folder:

```bash
cmake -S . -B build-macos-debug -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH="$FAIRWINDSK_QT_MACOS"
```

Never reuse an iOS, Android, Linux, or differently configured macOS build directory.

## 7. Compile

```bash
cmake --build build-macos --parallel
```

The result is normally `build-macos/FairWindSK.app`. Verify it:

```bash
test -d build-macos/FairWindSK.app
file build-macos/FairWindSK.app/Contents/MacOS/FairWindSK
```

If a clean Ninja build reports an unavailable generated QtZeroConf or QHotkey library, build those pinned targets first and resume:

```bash
cmake --build build-macos --target QtZeroConf qhotkey --parallel
cmake --build build-macos --parallel
```

Do not treat skipped or unavailable desktop dependency downloads as a successful full desktop build. Restore network access or provide the required dependencies, then perform the complete build.

## 8. Run the tests

```bash
ctest --test-dir build-macos --output-on-failure
```

These are host-side core and regression tests. They do not replace interactive validation of Qt WebEngine, touch targets, the single-window marine-MFD layout, Signal K connectivity, or every comfort preset.

## 9. Run from the build tree

```bash
open build-macos/FairWindSK.app
```

For terminal diagnostics:

```bash
build-macos/FairWindSK.app/Contents/MacOS/FairWindSK
```

On first launch, macOS may ask for local-network or location access. Grant only the permissions needed by the intended vessel setup. Configure the Signal K endpoint in **Settings > Connection**.

## 10. Install to a staging prefix

Avoid writing directly to a system location while validating packaging:

```bash
cmake -S . -B build-macos -DCMAKE_INSTALL_PREFIX="$PWD/stage-macos"
cmake --install build-macos
find stage-macos -maxdepth 3 -name 'FairWindSK.app' -print
```

The install step preserves the application bundle and its embedded resources. It does not make the bundle redistributable by itself: use a clean staging directory for each packaging check, then run `macdeployqt` so Qt and the linked desktop helper libraries are copied into the distribution bundle.

## 11. Bundle the Qt runtime

Copy the built app before modifying it for distribution:

```bash
cp -R build-macos/FairWindSK.app FairWindSK-distribution.app
"$FAIRWINDSK_QT_MACOS/bin/macdeployqt" FairWindSK-distribution.app -verbose=2
```

Inspect linkage after deployment:

```bash
otool -L FairWindSK-distribution.app/Contents/MacOS/FairWindSK
codesign --verify --deep --strict FairWindSK-distribution.app
```

`macdeployqt` bundles Qt libraries and plugins, but public distribution also requires an Apple Developer ID signature and notarization. Keep certificates, credentials, and notarization profiles outside the repository. Follow current Apple requirements when producing a release artifact.

## 12. Qt Creator workflow

1. Install the macOS Qt kit and Qt Creator with the Qt Online Installer.
2. Open the repository's top-level `CMakeLists.txt`.
3. Select a **Desktop Qt 6 macOS** kit, not an iOS kit.
4. Choose a dedicated build directory such as `build-macos-qtcreator`.
5. Select Debug or Release and configure the project.
6. Build `FairWindSK`, run the registered tests, then run the app.
7. Confirm the application remains one window and embedded apps do not escape into external windows.

## 13. Troubleshooting

### CMake cannot find Qt

Check the prefix and configure again in a clean build directory:

```bash
brew --prefix qt
test -f "$FAIRWINDSK_QT_MACOS/lib/cmake/Qt6/Qt6Config.cmake"
```

### Qt WebEngine is missing

Verify `Qt6WebEngineWidgets` exists in the selected kit. A mobile Qt kit or an incomplete custom Qt installation cannot build the macOS desktop flavor.

### A downloaded dependency fails

Confirm Git and HTTPS access to GitHub, remove only the affected disposable build directory, and configure again. Corporate proxies must be configured for both Git and CMake dependency downloads.

### The app is blocked by Gatekeeper

A locally built unsigned app can be launched from the build tree during development. A redistributed app must be correctly signed and notarized; do not advise users to disable Gatekeeper globally.

### Embedded content is blank

Run the executable from Terminal and inspect Qt WebEngine messages. Confirm network reachability, TLS trust, Signal K authentication, and that the web application is allowed to render inside an embedded view.

## 14. macOS validation checklist

Before calling a macOS change complete, verify:

1. Configure, full dependency build, application build, and CTest all succeed.
2. The app starts both from the build tree and the staged bundle.
3. Signal K discovery/manual connection, REST data, websocket updates, authentication, reconnect, and server-restart recovery work.
4. Web applications, their icons, launcher pages, and the Apps home action work without external windows.
5. Settings, MyData, POB, alarms, anchor, and available autopilot controls remain responsive.
6. Mouse, keyboard, touchscreen (when present), and the desktop foreground shortcut behave coherently.
7. Touch targets and text remain readable at helm distance in `default`, `dawn`, `day`, `sunset`, `dusk`, and `night` presets.
8. English, Italian, and other shipped translations fit without clipping.
9. The packaged app contains the FairWindSK icon and all required Qt plugins and helper libraries.
10. Relevant behavior remains coherent with Linux, Windows, Raspberry Pi OS, Android, and iOS/iPadOS; platform-specific differences are documented.

## Related guides

- [Cross-platform build overview](building.md)
- [iOS and iPadOS](ios.md)
- [Android](android.md)
- [Linux container build](container.md)
