# Building and running FairWindSK in a container

FairWindSK can be compiled and smoke-tested in a Linux container. This is useful for checking the Linux desktop flavor from a non-Linux host and for reproducing compiler or dependency issues in a clean environment. The shared flavor matrix and completion rules are maintained in [building.md](building.md).

FairWindSK is a Qt desktop GUI with Qt WebEngine, not a web service. A container therefore needs either a virtual X display for automated startup checks or access to a host X server for interactive use. The container does not replace the normal native installation described in [Building FairWindSK](building.md).

## Requirements

- Docker Engine or Docker Desktop with at least 4 GB of memory available to the Linux VM
- Internet access for Ubuntu packages and the pinned `QtZeroConf` and `QHotkey` source downloads
- Approximately 2 GB of free image and build-cache space
- An X11 server on the host only when the GUI must be used interactively

Docker builds the image for the host Docker architecture by default. On Apple Silicon this produces a native Linux `arm64` binary; on an x86-64 host it produces a Linux `amd64` binary. Pass `--platform linux/amd64` or `--platform linux/arm64` to both `docker build` and `docker run` when a particular architecture is required and the Docker installation supports emulation.

## Build the image

Run the following command from the FairWindSK repository root. It sends the Dockerfile through standard input while using the repository as the build context, so no generated Dockerfile is left in the working tree.

The repository `.dockerignore` keeps local CMake build trees, editor state, version-control metadata, and retained container logs out of the Docker context. Keep that file in place; otherwise a host build tree can add several gigabytes to every image build.

```bash
docker build --tag fairwindsk-linux --file - . <<'EOF'
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install --yes --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        dbus-x11 \
        dpkg-dev \
        git \
        libavahi-compat-libdnssd-dev \
        libnss-mdns \
        libqt6svg6-dev \
        ninja-build \
        nlohmann-json3-dev \
        qt6-base-dev \
        qt6-base-dev-tools \
        qt6-declarative-dev \
        qt6-positioning-dev \
        qt6-tools-dev \
        qt6-tools-dev-tools \
        qt6-virtualkeyboard-dev \
        qt6-webengine-dev \
        qt6-webengine-dev-tools \
        qt6-websockets-dev \
        xvfb \
    && qt_arch="$(dpkg-architecture -qDEB_HOST_MULTIARCH)" \
    && qt_vk_dir="/usr/lib/${qt_arch}/cmake/Qt6VirtualKeyboard" \
    && test -d "${qt_vk_dir}" \
    && touch "${qt_vk_dir}/Qt6VirtualKeyboardConfigVersionImpl.cmake" \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . /src

RUN cmake -S /src -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /build --parallel 2

ENV QTWEBENGINE_DISABLE_SANDBOX=1
ENV LIBGL_ALWAYS_SOFTWARE=1

CMD ["/build/FairWindSK"]
EOF
```

QtZeroConf and QHotkey declare their installed libraries as CMake external-project
byproducts. A clean Ninja build therefore downloads and builds them automatically
before linking FairWindSK; no separate dependency build step is required.

Ubuntu 24.04 may also omit `Qt6VirtualKeyboardConfigVersionImpl.cmake` even though the rest of the Qt Virtual Keyboard CMake package is installed. The image creates the empty compatibility file in the active multiarch Qt directory. Remove that workaround when the distribution package supplies the file.

## Run a headless startup smoke test

The following command starts a private D-Bus session and a 1280 by 800 virtual X display. FairWindSK is allowed to initialize for 20 seconds and is then stopped by `timeout`:

```bash
docker run --rm fairwindsk-linux \
  bash -lc 'timeout 20s dbus-run-session -- xvfb-run -a -s "-screen 0 1280x800x24" /build/FairWindSK; test $? -eq 124'
```

Exit status zero means the application remained alive until the expected timeout. Startup messages are written to the container output. This check exercises Linux process startup and Qt widget/WebEngine initialization, but it does not verify visual layout, touch behavior, hardware acceleration, marine-MFD readability, or interaction with a real Signal K server.

To retain diagnostic logs, mount a host directory at the Qt application-data location:

```bash
mkdir -p container-logs
docker run --rm \
  --mount type=bind,src="$(pwd)/container-logs",dst=/root/.local/share/uniparthenope.it/FairWindSK \
  fairwindsk-linux \
  bash -lc 'timeout 20s dbus-run-session -- xvfb-run -a -s "-screen 0 1280x800x24" /build/FairWindSK; test $? -eq 124'
```

`container-logs` is generated runtime data and should not be committed.

## Run interactively on a Linux X11 host

On a Linux host already running X11, allow the container root user to connect temporarily, share the X11 socket, and pass the display name:

```bash
xhost +si:localuser:root
docker run --rm --interactive --tty \
  --env DISPLAY \
  --mount type=bind,src=/tmp/.X11-unix,dst=/tmp/.X11-unix,readonly \
  fairwindsk-linux
xhost -si:localuser:root
```

The final `xhost` command removes the temporary permission. Wayland-only hosts need an XWayland-compatible display or a separately configured Wayland socket; the X11 command above does not grant Wayland access.

Docker Desktop on macOS and Windows does not provide a Linux GUI display socket. Install and configure an external X server before attempting an interactive run, or use the headless smoke test. Native macOS and Windows builds remain the recommended way to inspect the UI and validate touch-friendly marine-MFD behavior on those platforms.

## Connect to Signal K on the host

Inside Docker Desktop, `localhost` refers to the container. Use `host.docker.internal` to reach a Signal K server running on the macOS or Windows host, for example `http://host.docker.internal:3000`.

On Linux, add the host-gateway mapping when the Docker installation does not already provide that name:

```bash
docker run --rm \
  --add-host host.docker.internal:host-gateway \
  fairwindsk-linux
```

Configure FairWindSK with the corresponding host name through its normal configuration workflow. Persist the Qt application-data directory with a bind mount if the configuration must survive container removal.

## Inspect the result

Verify the executable architecture and check for unresolved shared libraries:

```bash
docker run --rm fairwindsk-linux \
  bash -lc 'uname -m; readelf -h /build/FairWindSK | sed -n "/Class:/p;/Machine:/p"; ! ldd /build/FairWindSK | grep "not found"'
```

Run the registered headless desktop suite after the image build:

```bash
docker run --rm fairwindsk-linux \
  ctest --test-dir /build --output-on-failure
```

The CTest suite validates core parsers, models, configuration helpers, Signal K
value objects, and the built-in safety regressions. Keep the startup smoke test
above as a separate check of Qt WebEngine initialization and the full MFD shell.

## Platform validation limits

The container validates only the Linux desktop Qt WebEngine path for its selected CPU architecture. It does not build or validate Windows, native macOS, Raspberry Pi OS hardware integration, Android, or iOS/iPadOS. It also cannot establish GUI conformance by itself. GUI changes still require interactive checks for finger-friendly targets, pressed and focus states, helm-distance readability, stable single-window behavior, translations, and all `default`, `dawn`, `day`, `sunset`, `dusk`, and `night` comfort presets on representative hardware. Follow the relevant flavor guide linked from [building.md](building.md) before claiming support.
