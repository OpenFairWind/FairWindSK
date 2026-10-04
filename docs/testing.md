# Automated testing

FairWindSK uses CTest as the common entry point and Qt Test for C++ tests. The
desktop test build is intentionally independent of the application executable
where possible, so failures point to a focused unit or component.

## Run the suite

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The simulated Signal K component binds ephemeral loopback TCP and WebSocket
ports. Sandboxed build agents must explicitly allow loopback listeners.

## Current layers

### Unit tests

The initial suite covers configuration load/save and invalid input, coordinate
formatting and parsing, property-list persistence, data-widget configuration,
Signal K value objects and subscriptions, delta parsing, stale-data boundaries,
and reconnect-state transitions. Data-driven rows keep boundary cases visible in
Qt Test output and bring the non-visual suite into the initial 50–70-case range.

Production `Client` delta dispatch and stale detection use the same
`signalk/ProtocolUtils` functions exercised by these tests.

### Component tests

`tests/support/SimulatedSignalKService` is a reusable in-process Signal K test
double. It supports discovery, authentication acceptance/rejection, REST
resources, WebSocket deltas, malformed stream messages, delayed HTTP responses,
connection loss, restart, and resource mutation during restart. The component
suite verifies each behavior deterministically, including resources created
during a restart being visible after recovery.

### UI smoke tests

GUI automation remains the next layer because it needs stable accessibility
names and a hermetic application profile before it can safely drive the actual
single-window shell. Add scenarios in this order:

1. first-run startup and unavailable-server state;
2. connection to `SimulatedSignalKService`;
3. application launch and Apps/home return;
4. Bottom Bar, POB, Anchor, Alarms, and Autopilot overlays;
5. POB activation and cancellation;
6. every comfort preset and supported language;
7. configuration import and export.

UI assertions must check touch-target geometry, visible pressed/focus feedback,
translated-label fit, and the `default`, `dawn`, `day`, `sunset`, `dusk`, and
`night` MFD comfort presets. They must not create another top-level window.

## Platform expectations

Host-runnable Qt Test binaries are built for macOS, Windows, Linux, and Raspberry
Pi OS desktop kits. Android and iOS builds do not run host CTest binaries; the
mobile CI jobs therefore compile the application and smoke-test Android startup,
while device-side Qt Test coverage remains future work.
Release verification still requires API 33 and current-target Android devices,
an iOS/iPadOS simulator and physical device, and touch/MFD checks on every
desktop flavor. Automated tests reduce that matrix; they do not claim platform
coverage that CI has not executed.

## Pull-request verification matrix

The `Verify` GitHub Actions workflow keeps each FairWindSK flavor visible as a
separate required check. A successful desktop build is not evidence that a
mobile or Raspberry Pi flavor compiles.

| Flavor | Required automated validation |
| --- | --- |
| Linux x86-64 | Build, unit tests, simulated Signal K integration tests, and Xvfb/WebEngine startup smoke test |
| Windows MSVC x86-64 | Build and unit tests |
| macOS arm64 | Build and unit tests |
| Android API 33 | Configure and compile an arm64 APK while preserving API 33 as the minimum runtime |
| Android current API | Configure and compile an x86-64 APK, install it in an emulator, launch it, and verify its process remains alive |
| iOS Simulator | Configure and compile the unsigned x86-64 Simulator bundle |
| Raspberry Pi ARM64 | ARM64 architecture build; scheduled physical-device validation remains required |

Independent mandatory checks cover formatting, clang-tidy policy, CMake
configuration, translation completeness, newly added unwrapped Qt strings, JSON
schemas, documentation links, dependency review, secret scanning, and
release-candidate artifact generation. Configure branch protection to require
every `Verify` job before merge.

Translation completeness has a reviewed debt ceiling in the workflow. It may
decrease without coordination, but an increase fails CI. Emulator and headless
smoke tests verify startup, not marine-MFD usability. GUI changes still require
touch-target, pressed/focus-state, translated-label fit, single-window, and all
comfort-preset checks on representative hardware.
