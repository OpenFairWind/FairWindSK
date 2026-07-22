# Building and running FairWindSK on Raspberry Pi OS

This is the authoritative step-by-step guide for building, configuring,
installing, and operating FairWindSK directly on Raspberry Pi OS. The Raspberry
Pi flavor is a Linux ARM desktop build: it uses Qt WebEngine Widgets, preserves
the single-window marine Multi-Functional Display shell, and includes optional
kiosk/autostart and OpenPlotter integration.

See [building.md](building.md) for the shared cross-platform contract and
[hardware.md](hardware.md) for marine-computer and power-system considerations.

## 1. Supported system profile

Use a maintained 64-bit Raspberry Pi OS release with the desktop environment.
Raspberry Pi OS Bookworm 64-bit on a Raspberry Pi 4, Raspberry Pi 5, or a
Compute Module of equivalent capability is the recommended baseline. A desktop
session is required for normal Qt WebEngine operation; Raspberry Pi OS Lite is
appropriate only after separately installing and configuring a supported
graphical stack.

Recommended resources:

- 4 GB RAM or more;
- at least 8 GB of free storage for sources, packages, and build output;
- active cooling for sustained native compilation;
- reliable Ethernet or Wi-Fi access to the Signal K server;
- a landscape display with touch input for the intended marine-MFD workflow.

Build natively on the Pi when possible. A cross-compile can prove compilation
for an ARM target, but it cannot validate the Raspberry Pi display stack,
WebEngine rendering, touch input, OpenPlotter integration, or helm readability.

## 2. Prepare Raspberry Pi OS

Update the package catalog and installed system packages, then reboot if the
kernel, firmware, or graphics stack changed:

```bash
sudo apt update
sudo apt full-upgrade
sudo reboot
```

After reconnecting, verify the operating system and architecture:

```bash
cat /etc/os-release
uname -m
getconf LONG_BIT
```

The expected architecture is `aarch64` and the reported word size is `64`.
Keep the system clock and timezone correct because TLS validation, diagnostics,
track timestamps, and Signal K data all depend on reliable timekeeping.

## 3. Install build and runtime dependencies

Install the compiler, CMake tools, Qt 6 desktop modules, WebEngine, virtual
keyboard support, multicast DNS support, and QML runtime modules:

```bash
sudo apt install build-essential cmake ninja-build git pkg-config python3 \
  nlohmann-json3-dev libnss-mdns avahi-utils \
  libavahi-compat-libdnssd-dev libxkbcommon-dev \
  qt6-base-dev qt6-base-dev-tools qt6-declarative-dev qt6-tools-dev \
  qt6-l10n-tools qt6-webengine-dev qt6-webengine-dev-tools \
  qt6-websockets-dev qt6-positioning-dev libqt6svg6-dev \
  qt6-virtualkeyboard-dev libqt6virtualkeyboard6 qt6-virtualkeyboard-plugin \
  qml6-module-qt-labs-folderlistmodel qml6-module-qtquick-window \
  qml6-module-qtquick-layouts qml6-module-qtqml-workerscript
```

Package names can differ between Raspberry Pi OS releases. Confirm that CMake
can find the corresponding Qt packages rather than silently omitting a module:

```bash
cmake --version
ninja --version
qmake6 -query QT_VERSION
test -d /usr/lib/aarch64-linux-gnu/cmake/Qt6WebEngineWidgets
test -d /usr/lib/aarch64-linux-gnu/cmake/Qt6VirtualKeyboard
```

Some Raspberry Pi OS images have shipped the Virtual Keyboard CMake package
without `Qt6VirtualKeyboardConfigVersionImpl.cmake`. Only if CMake reports that
specific missing file while the rest of the package is installed, apply this
distribution workaround:

```bash
sudo touch /usr/lib/aarch64-linux-gnu/cmake/Qt6VirtualKeyboard/Qt6VirtualKeyboardConfigVersionImpl.cmake
```

Do not use the workaround to conceal a genuinely missing Qt Virtual Keyboard
installation.

## 4. Clone the source

Clone the shared desktop/mobile development line, or substitute a release tag
when building a published release:

```bash
git clone --branch mobile https://github.com/OpenFairWind/FairWindSK.git
cd FairWindSK
```

Keep build output outside the source files and never reuse a build directory
created for another operating system, architecture, Qt kit, or build type.

## 5. Configure the native build

Configure a release build with tests enabled:

```bash
cmake -S . -B build-raspberrypi -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON \
  -DCMAKE_INSTALL_PREFIX=/usr/local
```

The first clean desktop build downloads the pinned QtZeroConf and QHotkey
sources if their previously built copies are not available. CMake uses the
system `nlohmann-json` package installed above. Restore network access and
complete these dependencies before treating the build as successful.

For a diagnostic build, configure a separate directory with
`-DCMAKE_BUILD_TYPE=Debug`.

## 6. Compile without exhausting the Pi

Start with two parallel jobs on a 4 GB system. Increase the value only when
memory and cooling are adequate:

```bash
cmake --build build-raspberrypi --parallel 2
```

If the kernel terminates a compiler process, inspect memory pressure and retry
with one job:

```bash
dmesg --level=err,warn | tail -n 50
cmake --build build-raspberrypi --parallel 1
```

The resulting executable is normally `build-raspberrypi/FairWindSK`.

## 7. Run the automated tests

Run all host-runnable unit tests and application self-tests:

```bash
ctest --test-dir build-raspberrypi --output-on-failure
```

CTest validates core behavior but does not establish WebEngine, touch, display,
autostart, Signal K, or marine-MFD conformance. Those checks require an active
desktop session and representative hardware.

## 8. Run from the build tree

Run FairWindSK from a terminal inside the graphical desktop session so startup
diagnostics remain visible:

```bash
./build-raspberrypi/FairWindSK
```

On Linux ARM, FairWindSK automatically selects an available Qt platform plugin,
creates a safe fallback `XDG_RUNTIME_DIR` when necessary, and defaults WebEngine
to software rendering with GPU/Vulkan features disabled. These conservative
defaults favor stability on marine Raspberry Pi installations. Override
`QT_QPA_PLATFORM`, `QT_OPENGL`, or `QTWEBENGINE_CHROMIUM_FLAGS` only after
testing the complete embedded-web workflow on the target image.

## 9. Configure FairWindSK and Signal K

On first run, FairWindSK creates `fairwindsk.ini` and `fairwindsk.json` in the
current user's Qt application-configuration directory. Configure the server in
**Settings > Connection**, or edit `connection.server` in `fairwindsk.json`:

```json
{
  "connection": {
    "server": "http://signalk-host.local:3000"
  }
}
```

Use a URL reachable from the Pi. Verify the server and application catalog
before troubleshooting FairWindSK:

```bash
curl --fail http://signalk-host.local:3000/signalk
curl --fail http://signalk-host.local:3000/signalk/v1/apps/list
```

For a touch-only helm, enable `main.virtualKeyboard` in **Settings > Main** and
restart FairWindSK. Set `main.windowMode` to `maximized` for a desktop-panel
installation or `fullscreen` for a frameless kiosk. Review all configuration
keys in [configuring.md](configuring.md).

## 10. Install on the Pi

The Raspberry Pi install is system-wide. It installs the executable under
`/usr/local/bin`, helper libraries under `/usr/local/lib`, the application-menu
entry and icon, the startup helper, and a Raspberry Pi OS autostart entry:

```bash
sudo cmake --install build-raspberrypi
sudo ldconfig
```

Verify the installed files and launch once before rebooting:

```bash
command -v FairWindSK
command -v fairwindsk-startup
test -f /usr/local/share/applications/fairwindsk.desktop
test -f /etc/xdg/autostart/fairwindsk-startup.desktop
fairwindsk-startup
```

The startup helper waits up to 45 seconds for the configured Signal K
application catalog, applies the Qt virtual-keyboard environment only when
enabled, launches FairWindSK from `PATH`, and restarts it only when the
application explicitly exits with code `1`.

To inspect a package layout without enabling autostart on the running host, use
`DESTDIR` with the `/usr/local` configuration above:

```bash
DESTDIR="$PWD/stage-raspberrypi" cmake --install build-raspberrypi
find stage-raspberrypi -maxdepth 6 -type f -print
```

## 11. Autostart and OpenPlotter behavior

The installed `/etc/xdg/autostart/fairwindsk-startup.desktop` applies to
graphical desktop sessions system-wide. If FairWindSK should not start
automatically, remove that entry through the system's desktop-session autostart
settings before rebooting.

When the installer detects OpenPlotter, it also attempts to copy the normal
FairWindSK launcher into known OpenPlotter menu directories. OpenPlotter layouts
vary between releases, so this menu integration is best-effort; the standard
XDG application launcher remains installed even when no dedicated OpenPlotter
directory is found.

## 12. Display, touch, and marine-MFD setup

Validate the final display in its installed orientation and at normal helm
distance:

1. Confirm the landscape resolution and desktop scaling before launching.
2. Calibrate the touchscreen using the Raspberry Pi OS/display-vendor tools.
3. Confirm every control has a comfortable finger target and visible pressed
   and focus states.
4. Check the Top Bar, Application Area, Bottom Bar, and drawers for clipping.
5. Exercise `default`, `dawn`, `day`, `sunset`, `dusk`, and `night` comfort
   presets under representative ambient light.
6. Check English and Italian text for clipping and sufficient contrast.
7. Confirm embedded applications never escape into external windows.

Do not infer touch or MFD compliance from a remote desktop session alone.

## 13. Troubleshooting

### CMake cannot find a Qt module

Read the missing component name in the CMake error and install its Raspberry Pi
OS development package. Do not point this build at an x86 desktop Qt prefix or
an Android Qt kit.

### The build is terminated or the Pi becomes unresponsive

Reduce compilation to one job, close WebEngine-heavy applications, confirm free
storage with `df -h`, and inspect kernel messages for out-of-memory events.

### The application cannot load a platform plugin

Run from the Raspberry Pi desktop session and inspect the reported plugin path.
If necessary, set `QT_QPA_PLATFORM=xcb` for an X11 session or
`QT_QPA_PLATFORM=wayland` for a complete Wayland Qt installation. FairWindSK
falls back to `xcb`, `eglfs`, or `linuxfb` only when the corresponding plugin is
available.

### Embedded pages are blank or slow

Verify server reachability, DNS, system time, and TLS trust first. Keep the ARM
software-rendering defaults unless target-specific GPU testing proves a stable
alternative. Capture terminal output and diagnostics from **Settings > System**.

### Autostart does not launch FairWindSK

Run `fairwindsk-startup` manually from a terminal. Confirm that `/usr/local/bin`
is in `PATH`, the installed helper libraries are visible after `ldconfig`, the
configuration belongs to the desktop user, and the Signal K URL is reachable.

### OpenPlotter does not show a dedicated menu item

Confirm that the standard FairWindSK entry exists in the desktop applications
menu. OpenPlotter-specific menu placement is intentionally best-effort because
its directory conventions vary across images.

## 14. Raspberry Pi OS validation checklist

Before calling a Raspberry Pi OS change complete, verify:

1. Clean configure, pinned dependency build, application build, and CTest pass.
2. FairWindSK starts from the build tree, application menu, installed helper,
   and a fresh graphical login when autostart is enabled.
3. Signal K discovery/manual connection, authentication, REST, websocket data,
   reconnect, and server-restart recovery work.
4. Web applications, launcher pages, Settings, MyData, POB, alarms, anchor, and
   available autopilot controls remain inside the single-window shell.
5. Touch input, virtual keyboard, display rotation, fullscreen/maximized modes,
   and all comfort presets are usable on the physical display.
6. English and Italian interfaces remain readable and unclipped.
7. Reboot, network-delay, and Signal K startup-order behavior are acceptable.
8. OpenPlotter menu behavior is checked when OpenPlotter is part of the target.
9. Relevant behavior remains coherent with Linux, macOS, Windows, Android, and
   iOS/iPadOS; unavoidable platform differences are documented.

## Related guides

- [Cross-platform build overview](building.md)
- [Configuration](configuring.md)
- [Hardware](hardware.md)
- [Getting started](getting_started.md)
- [Container-based Linux checks](container.md)
