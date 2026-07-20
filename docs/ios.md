# iOS and iPadOS build and simulator guide

FairWindSK supports iOS and iPadOS through Qt's mobile WebView implementation. The native Qt Widgets shell remains single-window, while embedded Signal K applications use `Qt::WebView` and `QQuickWidget` instead of the desktop-only Qt WebEngine stack.

This guide is the authoritative macOS environment setup, Qt Creator build, command-line build, iPad Simulator deployment, debugging, and troubleshooting workflow for FairWindSK on Apple mobile platforms.

## 1. Required components

iOS applications can only be built on macOS. Install mutually compatible versions of:

- a Mac supported by the selected Xcode release;
- Xcode and its command-line tools;
- an iOS Simulator runtime installed through Xcode;
- Git, CMake, and optionally Ninja;
- one Qt release installed both as macOS host tools and as an iOS kit;
- Qt iOS modules: WebView, Virtual Keyboard, Positioning, WebSockets, SVG, Quick, Quick Widgets, QML, and the base Qt libraries;
- Qt Creator, when using the graphical workflow.

The locally verified build reference environment is Qt 6.8.3, Xcode 26.6, and the iPhoneOS/iPhoneSimulator 26.5 SDKs. The unsigned arm64 device target and x86_64 simulator target both configured, compiled, linked, and passed Xcode bundle validation. Use an iOS Simulator runtime supported by the installed Xcode and macOS versions for execution.

The macOS and iOS Qt installations must have the same Qt version. A Homebrew Qt installation normally contains only macOS libraries and does not replace the Qt iOS kit.

## 2. Install and prepare Xcode

1. Install Xcode from the Mac App Store or Apple Developer downloads.
2. Start Xcode once, accept the license, and allow it to install required components.
3. Select the active developer directory:

   ```bash
   sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer
   ```

4. Confirm that the license and first-launch setup are complete:

   ```bash
   sudo xcodebuild -license accept
   xcodebuild -runFirstLaunch
   ```

5. Open **Xcode > Settings > Components** and install an iOS Simulator runtime compatible with that Xcode release.
6. Open **Window > Devices and Simulators > Simulators**. Create an iPad simulator if a suitable one is not already available. A current standard-size iPad is a useful general UI target; also test a smaller iPad and a large iPad Pro before release.

Qt 6.8's prebuilt iOS simulator libraries use x86_64. On an Apple Silicon Mac, install Rosetta and use a universal iOS platform component so Xcode can provide Rosetta simulator destinations:

```bash
softwareupdate --install-rosetta --agree-to-license
xcodebuild -downloadPlatform iOS -architectureVariant universal
```

In Xcode 26 or newer, remove the arm64-only iOS platform component from **Xcode > Settings > Components** before downloading the universal variant if Xcode reports a conflict. In Xcode's run-destination chooser, enable Rosetta destinations through **Product > Destination > Destination Architectures** when they are hidden.

Verify Xcode and the SDKs from Terminal:

```bash
xcodebuild -version
xcode-select --print-path
xcrun --sdk iphoneos --show-sdk-path
xcrun --sdk iphonesimulator --show-sdk-path
xcrun simctl list runtimes
xcrun simctl list devices available
```

If `simctl` reports that CoreSimulator is older than Xcode, update macOS/Xcode to a compatible pair, restart the Mac, and reinstall the simulator runtime from Xcode's Components settings.

## 3. Install Qt for macOS and iOS

1. Download and run the Qt Online Installer.
2. Sign in and select one Qt 6 release.
3. For exactly the same Qt version, select:
   - the macOS desktop kit;
   - the iOS kit;
   - Qt Creator;
   - Qt WebView;
   - Qt Virtual Keyboard;
   - Qt Positioning;
   - Qt WebSockets;
   - Qt SVG;
   - Qt Quick, Quick Widgets, QML, and the base development tools.
4. Complete installation and restart Qt Creator if it was already open.

Typical paths are:

```text
$HOME/Qt/6.8.3/macos
$HOME/Qt/6.8.3/ios
```

When adding modules later, use the Qt Maintenance Tool and select the modules under both the matching macOS and iOS Qt versions when offered. Mixing host tools from one Qt release with iOS libraries from another is unsupported.

## 4. Install command-line utilities and clone FairWindSK

Install the remaining host tools. Homebrew example:

```bash
brew install git cmake ninja
```

Clone the repository and enter it:

```bash
git clone --branch android https://github.com/OpenFairWind/FairWindSK.git
cd FairWindSK
```

The branch name reflects the current project development branch; it contains the shared desktop, Android, and iOS codebase.

Define task-specific paths for subsequent command-line examples:

```bash
export FAIRWINDSK_QT_HOST="$HOME/Qt/6.8.3/macos"
export FAIRWINDSK_QT_IOS="$HOME/Qt/6.8.3/ios"
```

Verify the tools and Qt installations:

```bash
cmake --version
"$FAIRWINDSK_QT_IOS/bin/qt-cmake" --version
test -d "$FAIRWINDSK_QT_HOST/lib/cmake/Qt6"
test -d "$FAIRWINDSK_QT_IOS/lib/cmake/Qt6"
test -d "$FAIRWINDSK_QT_IOS/lib/cmake/Qt6WebView"
test -d "$FAIRWINDSK_QT_IOS/lib/cmake/Qt6VirtualKeyboard"
test -d "$FAIRWINDSK_QT_IOS/lib/cmake/Qt6Positioning"
test -d "$FAIRWINDSK_QT_IOS/lib/cmake/Qt6WebSockets"
```

## 5. Configure Qt Creator

1. Start Qt Creator.
2. Open **Qt Creator > Preferences > Kits > Qt Versions**.
3. Confirm that the selected Qt version exposes both a macOS installation and an iOS installation. Add the iOS `qmake` or `qtpaths` manually only if automatic detection failed.
4. Open **Preferences > Kits > Kits** and confirm that an iOS Simulator kit is available and valid.
5. Open **Preferences > Devices > iOS** and confirm that Xcode, the simulator runtimes, and available simulator devices are detected.
6. Open FairWindSK's top-level `CMakeLists.txt` as a project.
7. On the **Configure Project** page, enable the iOS Simulator kit. Do not select the desktop macOS kit for this build directory.
8. Choose a separate build folder such as `build-ios-simulator-qtcreator`.
9. Let CMake configure the project and inspect the **Issues** pane before building.

If the kit is unavailable, recheck that the Qt iOS package is installed, Xcode is selected with `xcode-select`, and at least one compatible simulator runtime is installed.

## 6. Build and run on an iPad Simulator with Qt Creator

1. In Qt Creator, select the configured iOS Simulator kit from the kit selector.
2. Open the run-device selector and choose the desired iPad simulator.
3. On Apple Silicon, choose an x86_64/Rosetta iPad destination. Select the `Debug` configuration for interactive testing or `Release` for an optimized smoke test.
4. Choose **Build > Build Project FairWindSK**.
5. Wait for CMake, Qt code generation, compilation, linking, and bundle creation to complete.
6. Choose **Build > Run**. Qt Creator starts the selected simulator, installs the application bundle, and launches FairWindSK.
7. If the simulator remains on its Home screen, open the FairWindSK icon manually or check Qt Creator's **Application Output** for deployment errors.

No Apple Developer Program membership or provisioning profile is required for an iOS Simulator build. A physical iPad build requires signing with an Apple development team and a compatible provisioning profile.

## 7. Configure an iPad Simulator build from Terminal

Use the iOS kit's `qt-cmake`; it loads Qt's iOS toolchain. `QT_HOST_PATH` points to the matching macOS installation so CMake can run host tools such as `moc`, `rcc`, and `lrelease`.

Use a CMake release supported by the installed Qt and Xcode combination. CMake 4.4 may report an unknown AppleClang compiler with Xcode 26; CMake 3.31 is verified for this workflow.

Qt 6.8's official prebuilt iOS kit uses x86_64 simulator libraries. Configure x86_64 on both Intel and Apple Silicon Macs; Apple Silicon executes this target through Rosetta:

```bash
"$FAIRWINDSK_QT_IOS/bin/qt-cmake" \
    -S . \
    -B build-ios-simulator \
    -G Xcode \
    -DQT_HOST_PATH="$FAIRWINDSK_QT_HOST" \
    -DCMAKE_OSX_SYSROOT=iphonesimulator \
    -DCMAKE_OSX_ARCHITECTURES=x86_64 \
    -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO
```

Do not select arm64 merely because the Mac uses Apple Silicon. With the Qt 6.8 prebuilt kit, that compiles application objects for arm64 Simulator but attempts to link Qt plugin objects built for an iOS device. The linker then rejects the mixed platforms. A later Qt release or a custom Qt build may provide different simulator architectures; inspect that release's Qt for iOS documentation before changing the value.

Never reuse a device or macOS build directory for the simulator. CMake caches the SDK, architecture, and toolchain; use a new folder when any of them changes.

## 8. Build the simulator application from Terminal

Build the application bundle without code signing:

```bash
cmake --build build-ios-simulator \
    --config Debug \
    --parallel \
    -- -sdk iphonesimulator CODE_SIGNING_ALLOWED=NO
```

Locate the generated application rather than assuming a Qt-version-specific output path:

```bash
find build-ios-simulator -type d -name 'FairWindSK.app' -print
```

For the Xcode generator, the expected location is normally:

```text
build-ios-simulator/Debug-iphonesimulator/FairWindSK.app
```

Confirm that it is a simulator bundle and inspect its executable architecture:

```bash
export FAIRWINDSK_IOS_APP="$PWD/build-ios-simulator/Debug-iphonesimulator/FairWindSK.app"
file "$FAIRWINDSK_IOS_APP/FairWindSK"
plutil -p "$FAIRWINDSK_IOS_APP/Info.plist"
```

The bundle's supported platform must be `iPhoneSimulator`. With the Qt 6.8 prebuilt kit, `file` must report an x86_64 Mach-O executable.

## 9. Create, boot, and prepare an iPad Simulator from Terminal

List available iPad device types and installed runtimes:

```bash
xcrun simctl list devicetypes | grep iPad
xcrun simctl list runtimes
```

Create a simulator by copying the exact device-type and runtime identifiers from those commands:

```bash
export FAIRWINDSK_SIMULATOR_NAME="FairWindSK iPad"
export FAIRWINDSK_SIMULATOR_DEVICE_TYPE="COPY_DEVICE_TYPE_ID_FROM_SIMCTL"
export FAIRWINDSK_SIMULATOR_RUNTIME="COPY_RUNTIME_ID_FROM_SIMCTL"

xcrun simctl create \
    "$FAIRWINDSK_SIMULATOR_NAME" \
    "$FAIRWINDSK_SIMULATOR_DEVICE_TYPE" \
    "$FAIRWINDSK_SIMULATOR_RUNTIME"
```

Identifiers vary with Xcode and installed runtimes. Do not copy the example identifiers without checking the local `simctl` output.

Boot the new simulator and open the Simulator application:

```bash
xcrun simctl boot "$FAIRWINDSK_SIMULATOR_NAME"
open -a Simulator
xcrun simctl bootstatus "$FAIRWINDSK_SIMULATOR_NAME" -b
```

If an iPad simulator is already booted, the later commands can use the special `booted` destination instead of its name or UDID.

## 10. Install and launch FairWindSK with `simctl`

Install the compiled bundle and launch its bundle identifier:

```bash
xcrun simctl install booted "$FAIRWINDSK_IOS_APP"
xcrun simctl launch --console-pty booted org.openfairwind.fairwindsk
```

Press `Control-C` to detach from console output; the application continues according to Simulator state. To launch without attaching the console:

```bash
xcrun simctl launch booted org.openfairwind.fairwindsk
```

After rebuilding, terminate and reinstall the application to ensure the latest bundle is running:

```bash
xcrun simctl terminate booted org.openfairwind.fairwindsk
xcrun simctl install booted "$FAIRWINDSK_IOS_APP"
xcrun simctl launch booted org.openfairwind.fairwindsk
```

To remove the application and its simulator data completely:

```bash
xcrun simctl uninstall booted org.openfairwind.fairwindsk
```

## 11. Connect to Signal K from the simulator

Start the Signal K server before launching FairWindSK. Configure FairWindSK with a URL reachable from the simulated iPad.

- Try the Mac's LAN hostname or LAN IP address, for example `http://192.168.1.20:3000`.
- Confirm that macOS Firewall permits incoming connections to the Signal K process.
- Use `http://127.0.0.1:3000` only after verifying the selected Simulator/runtime networking behavior; a LAN address is clearer and also resembles physical-iPad deployment.
- Make sure the server listens on the LAN interface rather than only on an inaccessible interface.
- Test the URL in Safari inside the same iPad Simulator before diagnosing FairWindSK.

FairWindSK does not compile desktop Zeroconf discovery on iOS. Enter the Signal K endpoint explicitly when automatic discovery is unavailable.

## 12. iPad simulator validation checklist

Test in both portrait and landscape orientations. Use **Device > Rotate Left/Right** in Simulator or the corresponding keyboard shortcuts.

- Confirm that FairWindSK remains a single application window.
- Confirm that the Top Bar, Bottom Bar, Application Area, launcher pages, and both drawers fit without overlap.
- Open embedded Signal K applications and confirm that navigation remains inside FairWindSK.
- Verify that WebView content loads, scrolls, and accepts touch-equivalent pointer input.
- Verify launcher tiles, settings rows, checkboxes, combo boxes, spin boxes, file browsing, and dialog close actions.
- Verify readable text, focus, pressed states, and finger-friendly hit areas at normal helm distance.
- Test `default`, `dawn`, `day`, `sunset`, `dusk`, and `night` comfort presets for contrast and visual hierarchy.
- Switch the simulator through representative iPad sizes and confirm stable MFD spacing.
- Change FairWindSK's language and confirm that the Qt shell and embedded web localization remain aligned.
- Test application background/foreground transitions and relaunch persistence.
- Confirm that unsupported desktop-only actions are absent or disabled rather than opening another window.

Mouse input in Simulator approximates touch but does not replace physical-device validation. Before release, repeat touch, safe-area, rotation, network, performance, and comfort-preset testing on at least one supported physical iPad.

## 13. Capture logs and screenshots

Stream FairWindSK logs from the booted simulator:

```bash
xcrun simctl spawn booted log stream \
    --level debug \
    --predicate 'process == "FairWindSK"'
```

Capture recent log entries after reproducing a problem:

```bash
xcrun simctl spawn booted log show \
    --last 10m \
    --predicate 'process == "FairWindSK"'
```

Take a simulator screenshot:

```bash
xcrun simctl io booted screenshot fairwindsk-ipad.png
```

Qt Creator's **Application Output**, **Issues**, and **Debugger** panes are the easiest sources for C++/Qt diagnostics during an interactive Debug build.

## 14. Common failures

- **No iOS kit in Qt Creator:** install the iOS package for the selected Qt version, select Xcode with `xcode-select`, install a simulator runtime, and restart Qt Creator.
- **Qt component not found:** add the missing module to the iOS installation and ensure `QT_HOST_PATH` names the same Qt version's macOS installation.
- **A macOS executable is produced:** configure with the iOS kit's `qt-cmake`, `iphonesimulator`, and a clean build directory.
- **No C++ compiler is detected:** finish Xcode first-launch setup, verify `xcrun --sdk iphonesimulator clang++`, and confirm that the active developer directory points to the intended Xcode.
- **CMake reports `No known features for CXX compiler`:** CMake 4.4 does not identify AppleClang correctly with this Xcode 26 setup. Configure the clean simulator directory with CMake 3.31.
- **CoreSimulator is out of date:** install all compatible macOS and Xcode updates, restart the Mac, and reinstall or update the simulator runtime.
- **No Rosetta iPad destination appears on Apple Silicon:** install Rosetta, install Xcode's universal iOS platform component, and enable Rosetta destinations under **Product > Destination > Destination Architectures**.
- **The requested simulator architecture is unavailable:** Qt 6.8's prebuilt simulator libraries use x86_64. Inspect the selected Qt release's documentation and frameworks with `lipo -info` rather than assuming the Mac's native architecture.
- **The linker says a Qt object was built for iOS rather than iOS Simulator:** configure the Qt 6.8 simulator target as x86_64 in a clean build directory; arm64 selects the device slice from the prebuilt Qt frameworks.
- **`simctl` cannot find the device:** boot it from Xcode's Devices and Simulators window or use its exact name/UDID from `xcrun simctl list devices available`.
- **Installation rejects the bundle:** confirm that it was built for `iphonesimulator`, not `iphoneos`, and that its architecture matches the host simulator.
- **FairWindSK launches to a blank web area:** verify the Signal K URL in Simulator Safari, then inspect FairWindSK logs and confirm that Qt WebView is installed in the iOS kit.
- **Build works for Simulator but not a device:** physical devices require signing, a development team, a provisioning profile, and an iOS deployment target supported by the device.
- **Layout is clipped:** test both supported landscape directions and multiple iPad sizes, then treat safe-area, touch-target, and MFD layout defects as release blockers.

## 15. Physical iPad and release notes

FairWindSK declares only `UIInterfaceOrientationLandscapeLeft` and `UIInterfaceOrientationLandscapeRight` for both iPhone and iPadOS. The operating system therefore keeps the single-window marine MFD shell in landscape on simulator and physical devices.

The simulator is suitable for build validation and most UI workflows, but it does not reproduce all device behavior. Physical-device deployment requires:

- an Apple ID configured in **Xcode > Settings > Accounts**;
- an Apple development team selected for the FairWindSK target;
- a unique bundle identifier when required by the selected team;
- a provisioning profile compatible with that identifier and device;
- explicit testing of local-network permissions, rotation, safe areas, performance, memory pressure, touch behavior, and background/foreground transitions.

For App Store or TestFlight distribution, create an Xcode archive from a clean Release build, validate signing and entitlements, and follow Apple's current submission requirements. Never commit signing certificates, private keys, exported profiles, or account credentials.

## 16. Release checklist

- Build from a clean, reviewed commit.
- Confirm `git diff --check` and relevant desktop/Android regression builds.
- Build both the iPhoneOS device target and the iOS Simulator target.
- Run on representative small and large iPad simulators and verify that rotation remains locked to both supported landscape directions.
- Repeat touch and MFD validation on a physical iPad.
- Test every comfort preset and every shipped language.
- Verify the Signal K connection and embedded WebView navigation.
- Verify application identity, version, deployment target, architectures, signing, and entitlements.
- Preserve signing assets securely outside the repository.
- Record the Qt, Xcode, SDK, simulator-runtime, macOS, and tested-device versions in release notes.
