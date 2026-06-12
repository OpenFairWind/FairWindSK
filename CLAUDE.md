# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

FairWindSK is a Qt6/C++17 browser application that acts as a UI shell for [Signal K](https://signalk.org/) marine data servers. It hosts web-based Signal K applications and provides native vessel-operation controls (POB, Autopilot, Anchor, Alarms) alongside MyData management (Waypoints, Routes, Regions, Notes, Charts, Tracks, Files). The app targets six platforms: macOS, Linux, Raspberry Pi OS, Windows, Android, and iOS.

## Build Commands

```bash
# Configure
cmake -S . -B build

# Build
cmake --build build --parallel

# Install (Linux: registers desktop launcher and XDG integration)
cmake --install build
```

Desktop builds require Qt6 with **QtWebEngineWidgets**. Android/iOS builds require Qt6 with **Qt WebView** instead — `QtWebEngineWidgets`, `QtZeroConf`, `QHotkey`, and `PrintSupport` are desktop-only. External dependencies (nlohmann/json, QtZeroConf, QHotkey) are auto-downloaded by CMake on first build.

There is no formal automated test suite; verification is manual and cross-platform.

## Code Style

- C++17/Qt6 conventions, 4-space indentation, consistent brace placement and include ordering.
- Keep header and source files in sync; prefer small focused changes.
- Avoid code duplication — factor repeated UI or logic patterns into reusable components.
- Avoid new third-party dependencies unless absolutely necessary.
- Do not wrap `#include` directives in `try/catch`.
- Line-by-line pedagogical comments are expected.
- Prefer `rg` for searches over recursive `ls`/`grep`.

## Architecture

### Core Singleton: `FairWindSK`

`FairWindSK` (root-level `FairWindSK.cpp/hpp`) is the central orchestrator. It manages:
- Loading and saving configuration (`Configuration` class → `fairwindsk.json` + `fairwindsk.ini`)
- Signal K server connectivity via `signalk::Client`
- App registry (`AppItem` objects), including merging server-provided app lists with local overrides
- **Automatic restart recovery**: on disconnect → rediscovery → reconnect → re-subscribe → hydrate snapshots → reload REST-backed MyData models

### Configuration

- `fairwindsk.json`: user configuration seeded from `resources/json/configuration.json`. Holds Signal K path mappings, app list, UI scale/mode, coordinate format, window geometry.
- `fairwindsk.ini` (via `QSettings`): persists authentication tokens and the debug flag.
- `resources/json/configuration.json`: factory default that seeds `fairwindsk.json` on first run.

### Signal K Client: `signalk/`

`signalk::Client` handles REST and WebSocket communication with automatic reconnect. Supporting classes: `Subscription`, `Waypoint`, `Note`.

### UI Layer: `ui/`

`ui/MainWindow` orchestrates three zones defined in `docs/ui_shell.md`:
- **Top Bar** (`ui/topbar/`) — status and navigation
- **Bottom Bar** (`ui/bottombar/`) — vessel operation controls; specialized bars (POB, Autopilot, Anchor, Alarms) overlay this bar when active
- **Application Area** — hosts the active web or local app

Other UI modules:
- `ui/launcher/` — home screen with application tiles (pages and sub-pages)
- `ui/settings/` — settings pages: Apps, Comfort, Connection, SignalK, Units, System
- `ui/mydata/` — MyData resource management (Waypoints, Routes, Regions, Notes, Charts, Tracks, Files)
- `ui/web/` — **platform abstraction layer**: uses `QWebEngineView` on desktop and `QWebView` on mobile; all web embedding goes through this layer
- `ui/widgets/` — touch-friendly UI primitives (ComboBox, CheckBox, SpinBox, ColorPicker, etc.)

### Resources: `resources/`

- `resources/json/configuration.json` — factory Signal K path mappings and default app list
- `resources/json/units.json` — unit definitions consumed by `Units.cpp`
- `resources/stylesheets/` — QSS theme files for comfort presets (Default, Day, Dusk, Dawn, Sunset, Night)
- `resources/images/` and `resources/svg/` — UI assets

## GUI Change Requirements

For every GUI change, explicitly verify before considering the task done:
- **Touch targets**: finger-friendly hit areas, clear focus/pressed states, readable high-contrast text at helm distance.
- **MFD consistency**: layout and visual hierarchy must stay stable in rough-use conditions; avoid dense controls or low-contrast decorative styling.
- **Comfort presets**: all six presets (Default, Day, Dusk, Dawn, Sunset, Night) must remain readable and fit the marine electronics UI style.
- **Platform coverage**: call out explicitly when a change cannot be verified on all six platforms (macOS, Linux, Raspberry Pi OS, Windows, Android, iOS).

## Feature Definitions

- **POB**: clicking the icon sets a new Person Over Board marker. The POB dialog overlays the Bottom Bar and shows bearing, range, and elapsed time for the current POB (supports multiple). Cancel from the dialog; close the dialog by clicking the rightmost POB dialog icon.
- **Autopilot**: active only when the Autopilot plugin is installed. Dialog overlays the Bottom Bar and provides next-waypoint, wind-vane, tack/gybe, 1°/10° trim, route selection, dodge, and auto mode. Close by clicking the rightmost dialog icon.
- **Apps (launcher)**: the home button. Clicking hides the current Application Area content and shows the launcher with application tiles organized in pages and sub-pages.

## Documentation

All user-facing and developer documentation lives in `docs/`. Update relevant Markdown files when behavior changes or a new substantial feature is added. Use the formal UI terminology defined in `docs/ui_shell.md` (Top Bar, Bottom Bar, Application Area, drawers) consistently throughout code and docs.
