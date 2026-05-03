# Graphical user interface overview

FairWindSK presents a grid of applications backed by Qt6 widgets. Desktop builds render embedded web content with Qt WebEngine Widgets, while Android and iOS use a Qt WebView based widget host behind the same application-facing classes. Native shell components around the Application Area provide quick vessel controls and status.

The formal shell vocabulary used below is defined in [docs/ui_shell.md](./ui_shell.md).

## Desktop and navigation

- **Application grid**: Tiles correspond to `AppItem` entries discovered from the Signal K server or defined locally. Icons and labels come from the `signalk` metadata block (e.g., `displayName`, `appIcon`).
- **Focus management**: On desktop targets, when a web app takes over the view, press `SHIFT+TAB` to return to the FairWindSK desktop.
- **Icons and resources**: Icons can reference Qt resources (`:/resources/...`) or filesystem paths (`file://icons/...`). Ensure any new assets are registered in `resources.qrc` if bundled.

## Top Bar, Bottom Bar, and Application Area

- **Top Bar**: Provides the application icon, available data widgets, the current application name, date/time, and status icons.
- **Bottom Bar**: Provides the open web-application strip, the main application icons, and Signal K status indicators.
- **Application Area**: Hosts `MyData`, `Settings`, and embedded Signal K web applications between the Top Bar and Bottom Bar.
- **Bottom-bar functions**: Dedicated widgets for:
  - **Autopilot**: Shows autopilot state, target heading/wind angle, and rudder information using paths from the `signalk` configuration block.
  - **Anchor**: Displays anchor position, radius, rode length, and actions such as drop, raise, and radius adjustments (mapped to plugin action paths).
  - **POB (Person over board)**: Raises the standard Signal K `notifications.mob` alarm, creates a normal waypoint resource for the incident, sets it as destination, and shows bearing/distance/time from the incident position.
  - **Alarms**: Surfaces Signal K notifications (e.g., fire, piracy, abandon, sinking) and the anchor alarm state.

Availability of these bars depends on the configured plugin identifiers (`applications.autopilot`, `applications.anchor`) and the presence of the corresponding Signal K data paths.

## Settings and configuration UI

- The settings pane reads from `fairwindsk.json` to manage the application list, ordering, and activation. Server-discovered apps are merged with local overrides, and changes are saved back to the configuration.
- The **Settings > Top Bar** page uses a WYSIWYG editor with a live horizontal preview, a full-width bottom widget palette, drag-and-drop placement, explicit left/right ordering buttons, and per-item width/height expansion controls that are saved in the `barLayouts.top` configuration entries.
- Unit selection, window sizing, and virtual keyboard toggles are reflected in the `main` and `units` sections of the configuration.
- The **System** tab now includes touch-friendly diagnostics controls for:
  - logging level (`No logging`, `Critical`, `Warning`, `Info`, `Debug`, `Full`)
  - persistent message log storage with a compact checkbox-plus-description row
  - diagnostics email destination
  - the effective diagnostics subject and persistent log directory
  - an in-app persistent log explorer shown through the shared Bottom Bar horizontal drawer, with log files on the left and parsed log rows on the right
- The **Performance** pane now keeps process memory, total memory, memory usage, and per-core workload together in the left pane, while diagnostics and logging controls live on the right.
- The **Comfort** tab now exposes preset-level marine-theme tuning for the bundled `Default`, `Dawn`, `Day`, `Sunset`, `Dusk`, and `Night` QSS palettes, including dedicated controls for application background, button background, scroll-bar background, scroll-bar knob, and default SVG icon color.
- The touch color picker used by comfort editing now supports quick shade selection, custom color storage, custom color removal, and a compact scroll-safe layout that fits the Bottom Bar horizontal drawer presentation.
- The **My Data > Waypoints** details page reuses the embedded Signal K web view to show the waypoint on Freeboard-SK whenever the `@signalk/freeboard-sk` app is active on the connected server.
- The **My Data > Files** page now keeps directory actions, search results, rename/create flows, and copy/move/paste behavior aligned so the same operations work more consistently from both the browser and search views.
- The **Settings > Apps** layout now groups the right-pane toolbar into page actions and application palette actions, uses bundled marine-style SVG icons for the launcher-management buttons, includes bulk clear actions for removing applications from the current page or from every page, and lets operators tune each web app's zoom level (default `100%`) from the application details view.

## Startup and status messages

- FairWindSK opens directly into the main shell and performs startup work inside that single window. Runtime status messages mirror the lifecycle in `main.cpp`.
- If the Signal K server restarts while FairWindSK is running, the status widgets now show the reconnect cycle and the UI refreshes once the server has been rediscovered and the stream is resynchronized. When the restart is triggered from the embedded Signal K admin web app, FairWindSK now treats it as a planned restart, pauses its reconnect loop briefly, and then resumes recovery automatically instead of fighting the restart sequence.

## Drawers and web dialogs

- Web `alert`, `confirm`, and `prompt` popups are now routed through the shared Bottom Bar horizontal drawer instead of native modal dialogs, so embedded web apps stay visually consistent with the rest of the FairWindSK interface.
- Contextual right-side application menus should use the Application Area vertical drawer, which is non-modal and opens leftward from the right edge of the Application Area.

## Accessibility and input

- **Virtual keyboard**: If `main.virtualKeyboard` is true and the Qt VirtualKeyboard module is available, the on-screen keyboard becomes available for touch-only deployments.
- **Keyboard shortcuts**: On desktop targets, use `SHIFT+TAB` to exit an active web app session and return to the desktop; other shortcuts may be provided by individual web apps.
