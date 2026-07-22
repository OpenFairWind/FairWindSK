# Building FairWindSK on Windows

This is the authoritative step-by-step guide for creating a native Windows
build environment, compiling, testing, running, staging, and packaging
FairWindSK. The Windows desktop flavor uses Qt WebEngine Widgets; Android builds
hosted on Windows use a separate Qt Android kit and are covered in
[android.md](android.md).

---

## 1. Requirements

Use mutually compatible versions of every component:

- 64-bit host operating system (Windows 10 or later recommended)
- Git, CMake ≥ 3.16, and Ninja
- Qt 6 (6.7.x or later recommended; 6.8.x LTS preferred)
- MSVC 2022 64-bit compiler (Visual Studio 2022 Build Tools or full IDE)
- Qt modules: Core, Gui, Widgets, Qml, Concurrent, Network, WebSockets, Xml,
  Svg, SvgWidgets, Positioning, WebEngineWidgets, VirtualKeyboard,
  PrintSupport, and LinguistTools
- Internet access during the first clean build (downloads QtZeroConf and QHotkey)

> **Note:** MinGW is not supported for the desktop build. Qt WebEngine requires
> MSVC. If you select a MinGW kit, CMake will abort with a clear error message.

---

## 2. Install the build dependencies

1. Install [Git for Windows](https://git-scm.com/download/win).
2. Install [CMake](https://cmake.org/download/) and add it to `PATH`.
3. Install [Ninja](https://ninja-build.org/) and add it to `PATH`.
4. Run the **Qt Online Installer** from [qt.io](https://www.qt.io/download).
5. Under `Qt → Qt 6.x.x`, select:
   - `MSVC 2022 64-bit`
   - `Qt WebEngine` (required for desktop)
   - `Qt Virtual Keyboard`
   - Any other modules listed in section 1
6. Accept the licenses and complete the installation.

A typical installation path is `C:\Qt\6.8.3\msvc2022_64`.

Open an **"x64 Native Tools Command Prompt for VS 2022"** for all subsequent
steps. This ensures the MSVC environment is correctly loaded.

---

## 3. Clone FairWindSK

```cmd
git clone --branch mobile https://github.com/OpenFairWind/FairWindSK.git
cd FairWindSK
```

The `mobile` branch contains the current shared desktop and mobile development
line. Use a different branch name if targeting a specific release.

---

## 4. Configure a native Windows build

Keep every platform and Qt kit in its own build directory to avoid conflicts:

```cmd
set FAIRWINDSK_QT_WINDOWS=C:\Qt\6.8.3\msvc2022_64

cmake -S . -B build-windows -G Ninja ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_PREFIX_PATH="%FAIRWINDSK_QT_WINDOWS%"
```

**First-run note:** The first clean configure will download the pinned sources
for `QtZeroConf` and `QHotkey` from GitHub. Ensure network access is available.
Mobile-only dependencies (`Qt::WebView`, `Qt::Quick`) are not used by this build.

For a debuggable build, use a separate directory:

```cmd
cmake -S . -B build-windows-debug -G Ninja ^
      -DCMAKE_BUILD_TYPE=Debug ^
      -DCMAKE_PREFIX_PATH="%FAIRWINDSK_QT_WINDOWS%"
```

> Never reuse a build directory that was configured with different settings.
> Delete it and reconfigure from scratch if you need to switch build type or kit.

---

## 5. Compile

```cmd
cmake --build build-windows --parallel
```

If Ninja reports a missing `QtZeroConf` or `QHotkey` library on a first build,
build those targets explicitly before resuming the full build:

```cmd
cmake --build build-windows --target QtZeroConf qhotkey --parallel
cmake --build build-windows --parallel
```

> Do not treat skipped dependency downloads as a successful build. Restore
> network access, remove the affected build directory, and reconfigure.

---

## 6. Run the tests

Run the host-runnable unit and application self-tests:

```cmd
ctest --test-dir build-windows --output-on-failure
```

These tests cover core and regression behavior. They do not replace interactive
validation of Qt WebEngine, Signal K connectivity, touch targets, the
single-window marine-MFD layout, or every comfort preset.

---

## 7. Run from the build tree

The desktop helper DLLs are generated under `build-windows\external\bin`.
Temporarily add that directory to `PATH` before starting an unstaged build:

```cmd
set PATH=%CD%\build-windows\external\bin;%PATH%
build-windows\FairWindSK.exe
```

Configure the Signal K endpoint in **Settings > Connection**. Confirm that web
applications remain embedded in the single FairWindSK window.

---

## 8. Stage and package

Stage the executable and the QtZeroConf/QHotkey helper DLLs without writing to
system installation directories:

```cmd
cmake --install build-windows --prefix stage-windows
```

The staged executable is `stage-windows\bin\FairWindSK.exe`. Add the Qt runtime
and plugins before moving the staged directory to another computer:

```cmd
windeployqt --release --compiler-runtime stage-windows\bin\FairWindSK.exe
```

`windeployqt` is a packaging step, not a substitute for CTest or the runtime
validation checklist below.

---

## 9. Qt Creator workflow

1. Launch Qt Creator and select `File → Open File or Project...`.
2. Choose the top-level `CMakeLists.txt` from your cloned repository.
3. When prompted for kits, select a **Desktop Qt 6 MSVC 2022 64-bit** kit.
4. Set a dedicated build directory (e.g., `build-qtcreator-debug`).
5. Select **Debug** or **Release** and click **Configure Project**.
6. Build with `Build → Build Project`.
7. Add `build-qtcreator-debug\external\bin` to the run environment's `PATH`, or
   run the staged application after building.
8. Run the application via the Run button and confirm:
   - The application opens as a single window.
   - Embedded web apps do not escape into external browser windows.

If the build environment becomes corrupted, right-click the project root in
Qt Creator, select **Clear CMake Configuration**, then **Run CMake**.

---

## 10. Troubleshooting

### Qt WebEngine is missing

Verify that `Qt6WebEngineWidgets` is present in your selected kit:

```cmd
dir "%FAIRWINDSK_QT_WINDOWS%\lib\cmake\Qt6WebEngineWidgets"
```

If the directory is missing, rerun the Qt Online Installer and install the
**Qt WebEngine** component for your Qt version and MSVC kit.

### A downloaded dependency fails

Confirm Git and HTTPS access to GitHub from your machine. Remove only the
affected disposable build directory, then reconfigure it:

```cmd
rmdir /s /q build-windows
cmake -S . -B build-windows -G Ninja ...
```

### Embedded content is blank

- Confirm network reachability to the Signal K server.
- Verify TLS certificates are trusted on the host machine.
- Check Signal K authentication tokens in the app settings.
- Confirm the web application is permitted to render inside an embedded view.

### MSVC environment not found

Ensure you are running from an **"x64 Native Tools Command Prompt for VS 2022"**
or that you have called `vcvars64.bat` before invoking CMake.

### A helper DLL is missing at startup

For a build-tree run, add the build's `external\bin` directory to `PATH`. For a
staged or packaged run, confirm that `QtZeroConf.dll` and `qhotkey.dll` are next
to `FairWindSK.exe`. Re-run `cmake --install` before `windeployqt` if either
helper is absent.

---

## 11. Windows validation checklist

Before marking a Windows change as complete, verify all of the following:

1. CMake configure, full dependency build, application build, and tests all
   succeed without errors or warnings treated as errors.
2. Signal K discovery and manual connection, REST data polling, WebSocket
   updates, token authentication, reconnect after disconnect, and
   server-restart recovery all work correctly.
3. Web applications load inside the embedded view; icons, launcher pages, and
   the Apps home action work without opening external browser windows.
4. Settings, MyData, POB, alarms, anchor monitoring, and autopilot controls
   (where configured) remain responsive.
5. Touch targets and text remain readable at helm distance in all comfort
   presets: `default`, `dawn`, `day`, `sunset`, `dusk`, and `night`.
6. English and Italian text fit without clipping at all supported comfort
   presets and screen sizes.
7. The staged package starts on a clean Windows test environment and contains
   the MSVC runtime, required Qt plugins, `QtZeroConf.dll`, and `qhotkey.dll`.
8. Relevant behavior remains coherent with macOS, Linux, Raspberry Pi OS,
   Android, and iOS/iPadOS; platform-specific differences are documented.

## Related guides

- [Cross-platform build overview](building.md)
- [Android builds hosted on Windows](android.md)
