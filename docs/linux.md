# Building and running FairWindSK on Linux

This is the authoritative guide for native Linux desktop builds of FairWindSK.
The Linux flavor uses Qt WebEngine Widgets and the same single-window,
touch-first marine Multi-Functional Display shell as macOS, Windows, and
Raspberry Pi OS. Raspberry Pi ARM systems have additional requirements covered
in [raspberrypi.md](raspberrypi.md).

## 1. Requirements

- A maintained 64-bit Linux desktop distribution;
- Git, CMake 3.16 or newer, Ninja, and a C++17 compiler;
- Qt 6 Core, Gui, Widgets, Qml, Concurrent, Network, WebSockets, Xml, Svg,
  SvgWidgets, Positioning, WebEngine Widgets, Virtual Keyboard, Print Support,
  and LinguistTools;
- internet access for the first clean build of pinned QtZeroConf and QHotkey
  dependencies.

## 2. Install dependencies on Debian or Ubuntu

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build git pkg-config \
  nlohmann-json3-dev libavahi-compat-libdnssd-dev libnss-mdns avahi-utils \
  qt6-base-dev qt6-base-dev-tools qt6-declarative-dev qt6-tools-dev \
  qt6-l10n-tools qt6-webengine-dev qt6-webengine-dev-tools \
  qt6-websockets-dev qt6-positioning-dev libqt6svg6-dev \
  qt6-virtualkeyboard-dev libqt6printsupport6
```

Package names differ on Fedora, Arch, openSUSE, and other distributions. Install
the equivalent development packages and confirm that the selected Qt prefix
contains every component listed above.

## 3. Clone and configure

```bash
git clone --branch main https://github.com/OpenFairWind/FairWindSK.git
cd FairWindSK
cmake -S . -B build-linux -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON
```

Use `--branch <release-tag>` instead for a published release. Pass
`-DCMAKE_PREFIX_PATH=/path/to/qt` when Qt is not discoverable through the
distribution's standard CMake paths. Never reuse a build directory created for
another Qt kit, architecture, platform, or build type.

## 4. Build and test

```bash
cmake --build build-linux --parallel
ctest --test-dir build-linux --output-on-failure
```

The first clean desktop build may download pinned QtZeroConf and QHotkey
sources. Do not treat a build that skipped or failed those dependencies as a
complete Linux build.

## 5. Run from the build tree

Launch from a terminal in the active desktop session:

```bash
./build-linux/FairWindSK
```

FairWindSK selects an available Qt platform integration and creates a private
fallback `XDG_RUNTIME_DIR` if the session value is missing or unsafe. Configure
the Signal K endpoint under **Settings > Connection** and consult
[configuring.md](configuring.md) for persistent settings.

## 6. Stage or install

For a package-style staging tree whose embedded launcher paths target
`/usr/local`:

```bash
cmake -S . -B build-linux -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr/local
DESTDIR="$PWD/stage-linux" cmake --install build-linux
```

For a direct system installation:

```bash
sudo cmake --install build-linux
sudo ldconfig
```

The Linux install includes the executable, QtZeroConf/QHotkey helper libraries,
startup helper, XDG application entry, and hicolor application icon. Package
the matching Qt runtime and plugins when deploying to a machine whose system Qt
does not provide them.

## 7. Troubleshooting

- If CMake cannot find Qt, verify the chosen Qt prefix and the missing module's
  development package.
- If the application cannot load `xcb` or Wayland, install the matching Qt
  platform plugin and run inside an active graphical session.
- If embedded pages are blank, verify Signal K reachability, TLS trust, system
  time, and Qt WebEngine diagnostics from a terminal.
- If multicast discovery fails, verify Avahi is running and that the firewall
  permits mDNS; manual Signal K server configuration remains available.

Use [container.md](container.md) for a reproducible Ubuntu compile and headless
startup check. Container success does not establish native display, GPU, touch,
audio, or marine-MFD conformance.

## 8. Linux validation checklist

Before calling a Linux change complete, verify:

1. Clean configure, dependency build, application build, and CTest pass.
2. Build-tree and staged/system-installed launches succeed.
3. Signal K discovery/manual connection, authentication, REST, websocket data,
   reconnect, and server-restart recovery work.
4. Web apps and native workflows remain within the single FairWindSK window.
5. Mouse, keyboard, and representative touchscreen interaction remain usable.
6. `default`, `dawn`, `day`, `sunset`, `dusk`, and `night` presets remain
   readable at helm distance.
7. English and Italian interfaces fit without clipping.
8. Relevant behavior remains coherent with macOS, Windows, Raspberry Pi OS,
   Android, and iOS/iPadOS.

## Related guides

- [Cross-platform build overview](building.md)
- [Raspberry Pi OS](raspberrypi.md)
- [Container-based Linux checks](container.md)
- [Configuration](configuring.md)
