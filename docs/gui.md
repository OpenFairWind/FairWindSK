# Graphical user interface overview

FairWindSK presents a desktop-style grid of applications backed by Qt6 widgets and Qt WebEngine views. Native bars along the top and bottom provide quick vessel controls and status.

## Desktop and navigation

- **Application grid**: Tiles correspond to `AppItem` entries discovered from the Signal K server or defined locally. Icons and labels come from the `signalk` metadata block (e.g., `displayName`, `appIcon`).
- **Focus management**: On desktop targets, when a web app takes over the view, press `SHIFT+TAB` to return to the FairWindSK desktop.
- **Icons and resources**: Icons can reference Qt resources (`:/resources/...`) or filesystem paths (`file://icons/...`). Ensure any new assets are registered in `resources.qrc` if bundled.

## Top and bottom bars

- **Top bar**: Provides quick status indicators and access points to settings depending on the active view.
- **Bottom bars**: Dedicated widgets for:
  - **Autopilot**: Shows autopilot state, target heading/wind angle, and rudder information using paths from the `signalk` configuration block.
  - **Anchor**: Displays anchor position, radius, rode length, and actions such as drop, raise, and radius adjustments (mapped to plugin action paths).
  - **POB (Person over board)**: Drops a waypoint, sets it as destination, and shows bearing/distance/time from the incident position.
  - **Alarms**: Surfaces Signal K notifications (e.g., fire, piracy, abandon, sinking) and the anchor alarm state.

Availability of these bars depends on the configured plugin identifiers (`applications.autopilot`, `applications.anchor`) and the presence of the corresponding Signal K data paths.

## Settings and configuration UI

- The settings pane reads from `fairwindsk.json` to manage the application list, ordering, and activation. Server-discovered apps are merged with local overrides, and changes are saved back to the configuration.
- Unit selection, window sizing, and virtual keyboard toggles are reflected in the `main` and `units` sections of the configuration.
- The **System** tab now includes touch-friendly diagnostics controls for:
  - logging level (`No logging`, `Critical`, `Warning`, `Info`, `Debug`, `Full`)
  - persistent message log storage with a compact checkbox-plus-description row
  - diagnostics email destination
  - the effective diagnostics subject and persistent log directory
  - an in-app persistent log explorer shown through the shared bottom drawer, plus a direct `Open logs` action for the host file manager
- The **Performance** pane now keeps process memory, total memory, memory usage, and per-core workload together in the left pane, while diagnostics and logging controls live on the right.
- The **Comfort** tab now exposes preset-level marine-theme tuning for the main QSS palettes and a dedicated default SVG icon color for each comfort preset.
- The touch color picker used by comfort editing now supports quick shade selection, custom color storage, and custom color removal directly from the touch UI.
- The **My Data > Waypoints** details page reuses the embedded Signal K web view to show the waypoint on Freeboard-SK whenever the `@signalk/freeboard-sk` app is active on the connected server.
- The **My Data > Files** page now keeps directory actions, search results, rename/create flows, and copy/move/paste behavior aligned so the same operations work more consistently from both the browser and search views.
- The **Settings > Apps** layout now groups the right-pane toolbar into page actions and application palette actions, uses bundled marine-style SVG icons for the launcher-management buttons, and includes bulk clear actions for removing applications from the current page or from every page.

## Splash and status messages

- On desktop targets, a splash screen shows progress as the application loads configuration, connects to Signal K, and fetches the web app catalog. Messages mirror the lifecycle in `main.cpp`.
- If the Signal K server restarts while FairWindSK is running, the status widgets now show the reconnect cycle and the UI refreshes once the server has been rediscovered and the stream is resynchronized.

## Web dialogs

- Web `alert`, `confirm`, and `prompt` popups are now routed through the shared BottomBar horizontal drawer instead of native modal dialogs, so embedded web apps stay visually consistent with the rest of the FairWindSK interface.

## Accessibility and input

- **Virtual keyboard**: If `main.virtualKeyboard` is true and the Qt VirtualKeyboard module is available, the on-screen keyboard becomes available for touch-only deployments.
- **Keyboard shortcuts**: On desktop targets, use `SHIFT+TAB` to exit an active web app session and return to the desktop; other shortcuts may be provided by individual web apps.
