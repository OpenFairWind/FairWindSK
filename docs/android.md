# Android build, signing, and launcher guide

FairWindSK supports Android 13/API 33 and newer. Android builds use Qt WebView rather than Qt WebEngine and may optionally be selected as the device Home application. Installation never changes the current Home application automatically.

This guide covers Linux, Windows, and macOS hosts, command-line and Qt Creator builds, APK signing, API verification, emulator/device deployment, and recovery from launcher configuration mistakes.

## 1. Required components

Use mutually compatible versions of every component:

- a 64-bit host operating system;
- Git, CMake, and Ninja;
- JDK 17;
- Android SDK Platform 33 or newer;
- Android SDK Build-Tools and Platform-Tools;
- the Android NDK required by the selected Qt release;
- one Qt release installed both as host tools and as an Android kit;
- Qt Android modules: WebView, Virtual Keyboard, Positioning, WebSockets, SVG, Quick, Quick Widgets, and the base Qt libraries.

The verified reference environment is Qt 6.8.3, JDK 17, NDK r26b (`26.1.10909125`), Android Platform 33, and an `arm64-v8a` Qt kit. A newer compile/target SDK is acceptable; the packaged minimum SDK must remain 33.

Do not use a desktop-only Homebrew, Linux distribution, or MSVC Qt installation as the Android kit. Android builds need Qt libraries compiled for the selected Android ABI plus matching desktop host tools.

## 2. Install Qt and Android tools

### Linux

1. Install Git, CMake, Ninja, a JDK 17 distribution, Python 3, `unzip`, and the standard C/C++ host tools. Debian/Ubuntu example:

   ```bash
   sudo apt update
   sudo apt install git cmake ninja-build openjdk-17-jdk python3 python3-venv unzip build-essential
   ```

2. Download the Qt Online Installer from the Qt website, sign in, and select one Qt release.
3. For that release select the desktop GCC host kit, Android `arm64-v8a`, and the modules listed above.
4. In Qt Creator open **Edit > Preferences > Devices > Android**, select JDK 17, and let Qt Creator install or validate the Android SDK, Build-Tools, Platform-Tools, Platform 33+, and the Qt-recommended NDK.
5. Accept every Android SDK license shown by Qt Creator.

Typical paths are `$HOME/Qt/6.8.3/gcc_64`, `$HOME/Qt/6.8.3/android_arm64_v8a`, and `$HOME/Android/Sdk`.

### macOS

1. Install the host utilities. Homebrew example:

   ```bash
   brew install cmake ninja openjdk@17 git
   ```

2. Install Qt with the Qt Online Installer. Select the macOS host kit and Android `arm64-v8a` for the same Qt version, plus the required modules.
3. In Qt Creator open **Preferences > Devices > Android**, select the JDK 17 bundle, then install or validate Platform 33+, Build-Tools, Platform-Tools, and the recommended NDK.
4. Accept the Android licenses.

Typical paths are `$HOME/Qt/6.8.3/macos`, `$HOME/Qt/6.8.3/android_arm64_v8a`, and `$HOME/Library/Android/sdk`.

Homebrew Qt is useful for desktop builds but normally contains only macOS binaries. It does not replace the Qt Android kit.

### Windows

1. Install Git for Windows, CMake, Ninja, and a 64-bit JDK 17 distribution.
2. Run the Qt Online Installer. Select an MSVC 2022 64-bit host kit and Android `arm64-v8a` for the same Qt version, plus the required modules.
3. In Qt Creator open **Edit > Preferences > Devices > Android**, select JDK 17, then install or validate Platform 33+, Build-Tools, Platform-Tools, and the recommended NDK.
4. Accept the Android licenses and use a Developer PowerShell or a terminal where CMake and Ninja are on `PATH`.

Typical paths are `C:\Qt\6.8.3\msvc2022_64`, `C:\Qt\6.8.3\android_arm64_v8a`, and `%LOCALAPPDATA%\Android\Sdk`.

## 3. Verify the toolchain

Clone the Android branch:

```bash
git clone --branch android https://github.com/OpenFairWind/FairWindSK.git
cd FairWindSK
```

On Linux/macOS define task-specific variables. Adjust the host-kit folder for the operating system:

```bash
export FAIRWINDSK_QT_HOST="$HOME/Qt/6.8.3/gcc_64"
export FAIRWINDSK_QT_ANDROID="$HOME/Qt/6.8.3/android_arm64_v8a"
export FAIRWINDSK_ANDROID_SDK="$HOME/Android/Sdk"
export FAIRWINDSK_ANDROID_NDK="$FAIRWINDSK_ANDROID_SDK/ndk/26.1.10909125"
export FAIRWINDSK_JAVA_HOME="/path/to/jdk-17"
```

macOS normally uses:

```bash
export FAIRWINDSK_QT_HOST="$HOME/Qt/6.8.3/macos"
export FAIRWINDSK_ANDROID_SDK="$HOME/Library/Android/sdk"
```

PowerShell equivalent:

```powershell
$env:FAIRWINDSK_QT_HOST = "C:\Qt\6.8.3\msvc2022_64"
$env:FAIRWINDSK_QT_ANDROID = "C:\Qt\6.8.3\android_arm64_v8a"
$env:FAIRWINDSK_ANDROID_SDK = "$env:LOCALAPPDATA\Android\Sdk"
$env:FAIRWINDSK_ANDROID_NDK = "$env:FAIRWINDSK_ANDROID_SDK\ndk\26.1.10909125"
$env:FAIRWINDSK_JAVA_HOME = "C:\Program Files\Java\jdk-17"
```

Verify Java, CMake, Qt, the API 33 platform, and the NDK toolchain:

```bash
"$FAIRWINDSK_JAVA_HOME/bin/java" -version
cmake --version
"$FAIRWINDSK_QT_ANDROID/bin/qt-cmake" --version
test -f "$FAIRWINDSK_ANDROID_SDK/platforms/android-33/android.jar"
test -f "$FAIRWINDSK_ANDROID_NDK/build/cmake/android.toolchain.cmake"
```

Use `Test-Path` instead of `test -f` in PowerShell.

## 4. Configure and build an APK

Linux/macOS:

```bash
JAVA_HOME="$FAIRWINDSK_JAVA_HOME" \
ANDROID_SDK_ROOT="$FAIRWINDSK_ANDROID_SDK" \
ANDROID_NDK_ROOT="$FAIRWINDSK_ANDROID_NDK" \
"$FAIRWINDSK_QT_ANDROID/bin/qt-cmake" \
    -S . \
    -B build-android-arm64 \
    -G Ninja \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-33 \
    -DANDROID_SDK_ROOT="$FAIRWINDSK_ANDROID_SDK" \
    -DANDROID_NDK_ROOT="$FAIRWINDSK_ANDROID_NDK" \
    -DQT_CHAINLOAD_TOOLCHAIN_FILE="$FAIRWINDSK_ANDROID_NDK/build/cmake/android.toolchain.cmake" \
    -DQT_HOST_PATH="$FAIRWINDSK_QT_HOST" \
    -DCMAKE_BUILD_TYPE=Release

JAVA_HOME="$FAIRWINDSK_JAVA_HOME" \
ANDROID_SDK_ROOT="$FAIRWINDSK_ANDROID_SDK" \
cmake --build build-android-arm64 --parallel
```

PowerShell:

```powershell
& "$env:FAIRWINDSK_QT_ANDROID\bin\qt-cmake.bat" `
  -S . -B build-android-arm64 -G Ninja `
  -DANDROID_ABI=arm64-v8a `
  -DANDROID_PLATFORM=android-33 `
  -DANDROID_SDK_ROOT="$env:FAIRWINDSK_ANDROID_SDK" `
  -DANDROID_NDK_ROOT="$env:FAIRWINDSK_ANDROID_NDK" `
  -DQT_CHAINLOAD_TOOLCHAIN_FILE="$env:FAIRWINDSK_ANDROID_NDK\build\cmake\android.toolchain.cmake" `
  -DQT_HOST_PATH="$env:FAIRWINDSK_QT_HOST" `
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-android-arm64 --parallel
```

Qt's finalizer normally creates the APK during the regular build. If the selected Qt release exposes explicit packaging targets, `cmake --build build-android-arm64 --target apk` is also valid. Locate artifacts rather than assuming a release-specific path:

```bash
find build-android-arm64 -type f -name '*.apk' -print
```

PowerShell:

```powershell
Get-ChildItem build-android-arm64 -Recurse -Filter *.apk
```

## 5. Create and protect a release signing key

Create a production key once. Store it outside the repository and back it up securely. Losing it prevents compatible upgrades of an installed application.

```bash
"$FAIRWINDSK_JAVA_HOME/bin/keytool" -genkeypair \
    -keystore /secure/path/fairwindsk-release.jks \
    -alias fairwindsk-release \
    -keyalg RSA \
    -keysize 4096 \
    -validity 10000
```

Never commit the keystore, passwords, `keystore.properties`, or signing environment variables. A debug key is not suitable for distribution.

## 6. Align and sign the release APK

Choose the installed Build-Tools version and the unsigned release APK reported by the build:

```bash
export FAIRWINDSK_BUILD_TOOLS="$FAIRWINDSK_ANDROID_SDK/build-tools/34.0.0"
export FAIRWINDSK_UNSIGNED_APK="/absolute/path/to/android-build-release-unsigned.apk"
export FAIRWINDSK_ALIGNED_APK="/absolute/path/to/FairWindSK-release-aligned.apk"
export FAIRWINDSK_SIGNED_APK="/absolute/path/to/FairWindSK-release-signed.apk"

"$FAIRWINDSK_BUILD_TOOLS/zipalign" -f -p 4 \
    "$FAIRWINDSK_UNSIGNED_APK" "$FAIRWINDSK_ALIGNED_APK"

"$FAIRWINDSK_BUILD_TOOLS/apksigner" sign \
    --ks /secure/path/fairwindsk-release.jks \
    --ks-key-alias fairwindsk-release \
    --out "$FAIRWINDSK_SIGNED_APK" \
    "$FAIRWINDSK_ALIGNED_APK"
```

The tools prompt for passwords without exposing them in shell history. On Windows use the platform-specific executable names:

```powershell
$env:FAIRWINDSK_BUILD_TOOLS = "$env:FAIRWINDSK_ANDROID_SDK\build-tools\34.0.0"
$env:FAIRWINDSK_UNSIGNED_APK = "C:\absolute\path\android-build-release-unsigned.apk"
$env:FAIRWINDSK_ALIGNED_APK = "C:\absolute\path\FairWindSK-release-aligned.apk"
$env:FAIRWINDSK_SIGNED_APK = "C:\absolute\path\FairWindSK-release-signed.apk"

& "$env:FAIRWINDSK_BUILD_TOOLS\zipalign.exe" -f -p 4 `
  "$env:FAIRWINDSK_UNSIGNED_APK" "$env:FAIRWINDSK_ALIGNED_APK"

& "$env:FAIRWINDSK_BUILD_TOOLS\apksigner.bat" sign `
  --ks "C:\secure\path\fairwindsk-release.jks" `
  --ks-key-alias fairwindsk-release `
  --out "$env:FAIRWINDSK_SIGNED_APK" `
  "$env:FAIRWINDSK_ALIGNED_APK"
```

Verify alignment, signature, package identity, and API levels:

```bash
"$FAIRWINDSK_BUILD_TOOLS/zipalign" -c -v 4 "$FAIRWINDSK_SIGNED_APK"
"$FAIRWINDSK_BUILD_TOOLS/apksigner" verify --verbose --print-certs "$FAIRWINDSK_SIGNED_APK"
"$FAIRWINDSK_BUILD_TOOLS/aapt2" dump badging "$FAIRWINDSK_SIGNED_APK" | grep -E 'package:|sdkVersion:|targetSdkVersion:|launchable-activity:'
```

The output must contain `sdkVersion:'33'` and `org.openfairwind.fairwindsk.FairWindSKActivity`.

## 7. Install and test

Enable developer options and USB debugging on a device, or start an API 33 virtual device:

```bash
"$FAIRWINDSK_ANDROID_SDK/platform-tools/adb" devices -l
"$FAIRWINDSK_ANDROID_SDK/platform-tools/adb" install -r "$FAIRWINDSK_SIGNED_APK"
"$FAIRWINDSK_ANDROID_SDK/platform-tools/adb" shell am start -W \
    -n org.openfairwind.fairwindsk/.FairWindSKActivity
```

Test installed-app refresh, icon/name layout, checkbox selection, selection persistence after restart, assignment in **Settings > Applications**, native activity launch, Back return, every comfort preset, and landscape layout at the target MFD resolution.

## 8. Optional Home/launcher operation

Open Android **Settings > Apps > Default apps > Home app** and select FairWindSK. Vendor wording differs. FairWindSK remains a normal launchable application and never selects itself automatically.

When FairWindSK is the current Home app and Android reports Back, Home, or App Switch hardware keys absent, the Bottom Bar displays corresponding soft controls. Home returns to FairWindSK's launcher. Recents opens a single-window strip of Android applications previously launched through FairWindSK; this avoids privileged system-recents APIs unavailable to ordinary applications.

Select applications under **Settings > Android**. The app icon launches the native activity immediately; the separate high-contrast checkbox controls whether it is available for assignment to FairWindSK launcher pages.

To recover another Home app during development:

```bash
"$FAIRWINDSK_ANDROID_SDK/platform-tools/adb" shell am start -a android.settings.HOME_SETTINGS
```

## 9. Debugging

Build with `-DCMAKE_BUILD_TYPE=Debug` for a debug-signed, inspectable APK. Clear logs, reproduce once, and capture diagnostics:

```bash
adb logcat -c
adb shell am force-stop org.openfairwind.fairwindsk
adb shell am start -W -n org.openfairwind.fairwindsk/.FairWindSKActivity
adb logcat -d -v threadtime > fairwindsk-android.log
adb shell dumpsys meminfo org.openfairwind.fairwindsk
adb shell dumpsys activity activities
```

On a debuggable build, private FairWindSK logs and configuration can be inspected without rooting the device:

```bash
adb shell run-as org.openfairwind.fairwindsk find files -maxdepth 4 -type f -print
adb shell run-as org.openfairwind.fairwindsk cat files/settings/fairwindsk.json
```

Common failures:

- **Desktop compiler selected:** provide the Android Qt kit, `QT_HOST_PATH`, NDK root, and chainload toolchain explicitly.
- **Qt component not found:** install the missing module for both the host and Android kit of the same Qt version.
- **GLES/EGL missing:** confirm the NDK toolchain is being chainloaded rather than the host compiler.
- **APK installs only on newer Android:** inspect `sdkVersion` with `aapt2`; FairWindSK's minimum must remain 33.
- **Keyboard opens unexpectedly or the Qt surface flickers:** use the shipped `adjustNothing` soft-input mode, keep editable touch controls on explicit-click focus, and confirm that no device overlay has replaced those manifest settings.
- **Signature mismatch on update:** uninstall the differently signed development build or sign with the same protected release key. Uninstalling clears application data.
- **Signal K unavailable:** use the server's LAN address; Android `localhost` refers to the Android device.

## 10. Distribution checklist

- Build from a clean, reviewed commit.
- Confirm `git diff --check` and desktop regression builds.
- Test an API 33 device/emulator and the current target API.
- Verify touch targets and all comfort presets at helm distance.
- Verify the APK signature certificate and SHA-256 digest.
- Preserve the release keystore and passwords offline.
- Publish checksums with the signed APK.
- Prefer an Android App Bundle for stores that require it: `cmake --build build-android-arm64 --target aab` when supported by the selected Qt release.
