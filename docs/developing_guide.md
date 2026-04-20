# FairWindSK Developing Guide

## Purpose and audience

This guide is the contributor-oriented companion to the user-facing documents in `docs/`.
It describes how FairWindSK works as a Qt6 application shell for Signal K, how data moves through the runtime, how the GUI is organized, and how to think about cross-platform and touch-friendly development decisions.

Use this guide when you need to:

- understand where a feature belongs before editing code
- trace a value from configuration or Signal K into the visible UI
- add or refactor reusable touch-oriented widgets
- change the shell behavior without breaking marine electronics usability expectations
- prepare changes that must compile across desktop and mobile targets

## Project context

FairWindSK is a C++17 and Qt6 application designed to host Signal K web applications inside a native shell that is optimized for leisure-boat and helm-adjacent usage.
The project sits at the intersection of:

- a native Qt widget shell
- embedded web applications and desktop-local applications
- Signal K REST and websocket data streams
- marine-operation bars such as POB, autopilot, anchor, and alarms
- a configurable comfort-preset system for daypart-aware visibility

The application is intentionally opinionated toward marine multi-functional display (MFD) behavior:

- persistent top and bottom shell bars
- stable spatial layout
- high-contrast comfort presets
- finger-friendly controls
- low-friction modal workflows through the bottom drawer

## Repository map

The most important directories and files are:

| Area | Main files | Responsibility |
| --- | --- | --- |
| Bootstrap | `main.cpp` | Qt startup, platform bootstrap, splash behavior, deferred runtime startup |
| Core runtime | `FairWindSK.cpp`, `FairWindSK.hpp` | Singleton orchestration, configuration, Signal K lifecycle, app registry, runtime reconfiguration |
| Configuration | `Configuration.cpp`, `Configuration.hpp` | Persistent JSON and `QSettings` backed preferences and tokens |
| Signal K client | `signalk/Client.cpp`, `signalk/Client.hpp` | Discovery, REST access, websocket stream, reconnect and resubscribe logic |
| Main shell | `ui/MainWindow.*` | Main window composition, overlays, bottom drawer hosting |
| Top shell | `ui/topbar/TopBar.*` | Global data widgets, title, time, status surface |
| Bottom shell | `ui/bottombar/BottomBar.*` and bar widgets | Main navigation, app shortcuts, vessel-operation entry points |
| Settings | `ui/settings/*` | Configuration UI including Comfort, Apps, Connection, Units, System, Signal K |
| MyData | `ui/mydata/*` | Files, waypoints, history tracks, resources, previews |
| Web integration | `ui/web/*` | Desktop WebEngine path and mobile WebView facade |
| Touch widgets | `ui/widgets/*` | Reusable touch-oriented controls and pickers |
| Shared dialog hosting | `ui/DrawerDialogHost.*` | Native bottom drawer dialog APIs |
| Resources | `resources.qrc`, `resources/stylesheets/*`, `resources/svg/*`, `resources/json/*` | Themes, icons, defaults, configuration seeds |

## Application architecture

### Architectural layers

FairWindSK is best understood as five cooperating layers:

1. **Platform bootstrap layer**
   `main.cpp` configures Qt attributes, Linux runtime fallbacks, splash handling, desktop WebEngine or mobile WebView initialization, and deferred startup sequencing.
2. **Runtime orchestration layer**
   `FairWindSK` owns the configuration, Signal K client, app registry, runtime reconfiguration, comfort preset application, and shared web profile handle.
3. **Data access layer**
   `Configuration` and `signalk::Client` provide persistent preferences and live vessel/server data.
4. **Shell and navigation layer**
   `MainWindow`, `TopBar`, `BottomBar`, and `Launcher` define the persistent FairWindSK chrome and page transitions.
5. **Feature/UI layer**
   Settings pages, MyData pages, native bar panels, and reusable touch widgets implement specific workflows.

### Singleton-centered runtime

`FairWindSK` is the central application service object.
This keeps cross-cutting concerns in one place:

- configuration loading and saving
- current comfort preset resolution
- runtime UI refresh triggers
- app discovery and registry maintenance
- access to the shared `signalk::Client`
- access to the shared desktop `QWebEngineProfile` when available

This design means most UI features should ask first:

- Is this state persistent? Then it probably belongs in `Configuration`.
- Is this state derived from server connectivity or app discovery? Then it probably belongs in `FairWindSK` or `signalk::Client`.
- Is this state local to a page or widget? Then it should stay in the page/widget unless it truly becomes shared runtime behavior.

## Data flow

### Startup data flow

At startup, the main path is:

1. `main.cpp` creates `QApplication`.
2. `FairWindSK::getInstance()` creates the singleton.
3. `FairWindSK::loadConfig()` loads `fairwindsk.json` and supporting `QSettings`.
4. `MainWindow` is constructed and the shell widgets are created.
5. A deferred startup task calls:
   - `FairWindSK::startSignalK()`
   - `FairWindSK::loadApps()`
   - `MainWindow::applyRuntimeConfiguration()`
   - `MainWindow::prewarmPersistentPagesAfterStartup()`
6. The event loop continues with the live websocket and native/widget/web content running together.

### Runtime data flow

Most runtime values follow one of these paths:

#### Configuration-driven UI values

`Configuration` -> `FairWindSK::applyUiPreferences()` / `reconfigureRuntime()` -> shell/pages/widgets -> visible UI

Examples:

- comfort presets and generated stylesheets
- app activation and ordering
- units and view preferences
- saved file paths and theme image paths

#### Signal K live values

Signal K server -> `signalk::Client` websocket or REST -> native widget/page model -> visible UI

Examples:

- top bar data widgets
- bottom bar operational panel data
- reconnect-sensitive MyData resources
- unit preferences synchronized from the server

#### App catalog values

Signal K apps API plus local configuration -> `FairWindSK::loadApps()` -> `AppItem` registry -> launcher and navigation shortcuts

Examples:

- launcher tile set
- open app shortcuts in the Bottom Bar
- application details in Settings

## Application lifecycle

### 1. Bootstrap

`main.cpp` configures process-wide behavior before showing the main shell:

- organization and application names
- diagnostics support
- shared OpenGL contexts
- Linux environment fallbacks for runtime directory and Qt platform plugins
- desktop splash screen
- desktop WebEngine or mobile WebView backend initialization

### 2. Configuration load

The singleton loads configuration and establishes the baseline runtime state.
This includes:

- JSON-backed application configuration
- token storage via `QSettings`
- initial comfort view and runtime preferences

### 3. Main shell creation

`MainWindow` creates and wires:

- `TopBar`
- `Launcher`
- `BottomBar`
- persistent overlay pages such as `Settings`, `MyData`, and `About` when needed
- the bottom horizontal drawer infrastructure

### 4. Deferred runtime startup

Deferred startup avoids blocking shell construction with server work.
The runtime then:

- connects to Signal K
- configures web profile state
- loads or refreshes apps
- prewarms persistent pages
- applies runtime configuration to the shell

### 5. Steady-state operation

During normal operation, the application simultaneously handles:

- websocket updates
- REST refreshes
- web app hosting
- drawer dialogs
- comfort preset changes
- reconnect recovery after Signal K restarts

### 6. Reconfigure and recovery

The runtime supports targeted reconfiguration through `FairWindSK::reconfigureRuntime(...)`.
Use the runtime flags in `FairWindSK.hpp` when a settings change should refresh only specific domains:

- `RuntimeUi`
- `RuntimeUnits`
- `RuntimeSignalKConnection`
- `RuntimeApps`
- `RuntimeSignalKPaths`

Signal K recovery is also lifecycle-aware:

- disconnect triggers reconnect attempts
- rediscovery refreshes endpoints
- subscriptions are re-sent
- exact-path values are hydrated again
- REST-backed models can reload after reconnect

### 7. Shutdown

Shutdown is mostly standard Qt shutdown, with diagnostics support notified through `aboutToQuit`.

## GUI organization

The formal shell vocabulary is defined in `docs/ui_shell.md`.
Contributor work should keep using those names consistently.

### Main shell regions

- **Top Bar**
  Global information, current app identity, time, status icons, and data widgets.
- **Application Area**
  The primary working surface for launcher, settings, MyData, and hosted applications.
- **Bottom Bar**
  Persistent navigation surface for app shortcuts, core native features, and Signal K server status.
- **Bottom Bar horizontal drawer**
  Modal upward-opening dialog host for touch-friendly workflows.
- **Application Area vertical drawer**
  Contextual, typically non-modal drawer for app-specific tools.

### Main window responsibilities

`ui/MainWindow.*` coordinates:

- shell composition
- overlay page switching
- active foreground application changes
- dialog drawer execution and completion
- top-bar synchronization
- prewarming of persistent pages

### Settings organization

`ui/settings/Settings` groups the major configuration surfaces:

- `Main`
- `Comfort`
- `Connection`
- `SignalK`
- `Units`
- `Apps`
- `System`

The `Comfort` page is especially important because it now drives the active UI immediately and is intended to be the single source of truth for configurable UI colors and theme assets.

### MyData organization

`ui/mydata` contains a native data-management suite:

- file browser and viewer
- waypoint and resource management
- history tracks
- preview widgets for geo and JSON resources

This area is important because it exercises both:

- touch-native widgets
- server-backed resource synchronization

## Touch-friendly strategy

FairWindSK should behave like a marine electronics interface first and a desktop utility second.
That affects layout, motion, density, and interaction patterns.

### Core principles

1. **Large hit targets**
   Controls should be comfortably tappable with wet or gloved fingers on compact displays.
2. **High contrast**
   Text, icons, and active states must remain readable at helm distance and across comfort presets.
3. **Spatial stability**
   Frequently used controls should not drift, reorder unexpectedly, or collapse into dense clusters.
4. **Shallow interaction depth**
   Common actions should require as few steps as practical.
5. **Predictable modal behavior**
   Modal flows should use the bottom drawer consistently, avoiding surprise popups or floating desktop-style dialogs.

### What this means in practice

- Prefer `QToolButton`, `QPushButton`, and custom touch widgets with clear pressed and selected states.
- Avoid tiny list affordances or low-contrast borders.
- Keep the Top Bar and Bottom Bar visible and uncovered.
- Fit drawer content into the available area between the Top Bar and Bottom Bar.
- Use iconography and labels together for critical navigation when possible.
- Keep comfort preset theming coherent across all reusable shell widgets.

## Reusable touch components

The reusable widgets in `ui/widgets/` are the primary toolkit for touch-safe workflows.

### Touch-oriented widgets

| Widget | Role |
| --- | --- |
| `TouchComboBox` | Touch-friendly preset and option selection |
| `TouchCheckBox` | Large-state boolean selection |
| `TouchSpinBox` | Finger-friendly numeric adjustment |
| `TouchScrollArea` | Scroll container aligned with FairWindSK chrome behavior |
| `TouchFileBrowser` | Reusable file selection surface designed for the bottom drawer |
| `TouchIconBrowser` | Icon-selection surface with touch-oriented browsing |
| `TouchColorPicker` | Color selection surface aligned with drawer-host constraints |
| `SignalKServerBox` | Reusable server-status surface in the shell |

### Drawer-hosted workflows

`ui/DrawerDialogHost.*` provides FairWindSK-native dialog entry points such as:

- file open/save
- icon browsing
- text entry
- log exploration
- geo coordinate editing
- message dialogs

This layer exists so the application can avoid desktop-default dialogs that do not fit the marine shell model.

### Guidance for new reusable widgets

When adding a new shared touch widget:

1. Design it to fit in the Bottom Bar horizontal drawer if it is modal.
2. Keep all colors derived from the Comfort system instead of hardcoded literals.
3. Make state changes visible through touch-friendly contrast and spacing.
4. Handle platform-neutral sizing first, then document any platform limitation explicitly.
5. Prefer layout strategies that avoid vertical scrolling in the drawer when possible.

## Comfort preset system

The comfort system is both a user feature and a core architecture concern.
It affects nearly every visible Qt-based shell surface.

### Sources of truth

- Base preset stylesheets live in `resources/stylesheets/*.qss`
- Runtime overrides are generated from `ui/settings/Comfort.cpp`
- Persisted user customizations live in `Configuration`

### Contributor rules

- New visible UI colors should map to comfort preset items.
- Avoid introducing hardcoded colors in shared shell widgets.
- If a widget must derive a computed color, derive it from a comfort-provided semantic color.
- Update shipped `.qss` files when a new reusable state becomes part of the common shell language.

## Web integration model

FairWindSK uses a unified higher-level web-hosting API with two implementation paths:

- **Desktop**
  Qt WebEngine Widgets
- **Android and iOS**
  Qt WebView hosted inside `QQuickWidget` containers

This split is important because contributor changes should usually target the shared UI contract rather than assuming a single backend.

### Desktop-only capabilities

Desktop builds keep features such as:

- `QWebEngineProfile`
- richer shared cookie handling
- desktop-local `file://` application launching
- `QHotkey`
- desktop-specific popup/download handling

### Mobile constraints

Mobile builds use the alternate `Qt::WebView` path and therefore intentionally avoid desktop-specific WebEngine hooks.
When documenting or implementing behavior, call this out explicitly instead of implying identical capabilities.

## Multiple build targets

FairWindSK has one codebase and multiple build/runtime targets.

### Linux

- Desktop-feature-complete path
- Qt WebEngine Widgets backend
- supports normal desktop and kiosk-like deployments
- Linux startup logic includes runtime directory, Qt platform, and graphics fallbacks

### Raspberry Pi OS

- follows the Linux desktop path
- important target for helm-adjacent touch screens
- kiosk and autostart helpers live in `extras/`
- installation now also includes Raspberry Pi OS specific desktop integration: the startup helper is installed into the system autostart path and OpenPlotter-aware menu integration is attempted when OpenPlotter is detected on the target system
- interaction density and rendering performance matter more here than on a desktop workstation

### macOS

- full desktop feature set
- app bundle output from CMake
- desktop WebEngine path

### Windows

- full desktop feature set
- standard `windeployqt` deployment flow after building
- desktop-local apps remain relevant here

### Android

- mobile-safe web backend through `Qt::WebView`
- surrounding shell remains widget-based
- no desktop-only hotkey/Zeroconf/WebEngine-only integrations

### iOS

- same architectural split as Android
- `Qt::WebView` backend
- desktop-specific integrations remain disabled

## Cross-platform contributor checklist

For any non-trivial change, review impact in these dimensions:

| Concern | Questions |
| --- | --- |
| Build split | Does this rely on desktop-only Qt modules or APIs? |
| Web backend | Does this assume `QWebEngineView` when mobile uses `Qt::WebView`? |
| Touch behavior | Is the control comfortably usable on Raspberry Pi touch displays and tablets? |
| Drawer behavior | Does modal content fit between the Top Bar and Bottom Bar? |
| Comfort presets | Are colors and states mapped through the Comfort system? |
| Operational clarity | Would this still be readable and stable in rough-use marine conditions? |

If a platform is not fully supported by the change, document the limitation clearly.

## Typical contributor workflows

### Add a new settings option

1. Add persistent state to `Configuration`.
2. Expose the option in the relevant `ui/settings/*` page.
3. Route runtime consequences through `FairWindSK::reconfigureRuntime(...)` where needed.
4. If the setting changes visible shell/UI behavior, ensure it updates live when practical.

### Add a new touch dialog flow

1. Decide whether the interaction belongs in the Bottom Bar horizontal drawer.
2. Create or reuse a touch widget in `ui/widgets/`.
3. Expose it through `ui/DrawerDialogHost.*` if it is intended to be reusable.
4. Verify the content fits between the Top Bar and Bottom Bar.

### Add a new Comfort-controlled shell state

1. Add a semantic color or state in `ui/settings/Comfort.cpp`.
2. Persist it through `Configuration`.
3. Update the preset `.qss` files.
4. Replace any local hardcoded colors in the affected widgets.

## Recommended reading order for new contributors

1. `README.md`
2. `docs/building.md`
3. `docs/architecture.md`
4. `docs/ui_shell.md`
5. `docs/developing_guide.md`
6. `main.cpp`
7. `FairWindSK.cpp`
8. `ui/MainWindow.*`
9. the specific feature area you plan to edit

## Current architectural limits

Contributors should keep these current realities in mind:

- The runtime is singleton-centered, which simplifies access but can make hidden coupling easier to introduce.
- The shell is strongly widget-based even on mobile, with a bridged web layer rather than a full QML rewrite.
- Some custom-rendered or embedded-web surfaces may still require extra work to stay fully aligned with Comfort semantics.
- Desktop and mobile web backends share interfaces, but they do not have identical capabilities.

## Summary

FairWindSK is a Qt-native marine shell around Signal K and related boat applications.
Successful contributions usually respect four principles at the same time:

- keep the runtime flow coherent through `FairWindSK`, `Configuration`, and `signalk::Client`
- preserve the shell vocabulary and drawer behaviors
- maintain touch-friendly, MFD-consistent interaction design
- keep features honest about desktop versus mobile differences
