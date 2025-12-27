# Graphical user interface overview

FairWindSK presents a desktop-style grid of applications backed by Qt6 widgets and Qt WebEngine views. Native bars along the top and bottom provide quick vessel controls and status.

## Desktop and navigation

- **Application grid**: Tiles correspond to `AppItem` entries discovered from the Signal K server or defined locally. Icons and labels come from the `signalk` metadata block (e.g., `displayName`, `appIcon`).
- **Focus management**: When a web app takes over the view, press `SHIFT+TAB` to return to the FairWindSK desktop.
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

## Splash and status messages

- On startup, a splash screen shows progress as the application loads configuration, connects to Signal K, and fetches the web app catalog. Messages mirror the lifecycle in `main.cpp`.

## Accessibility and input

- **Virtual keyboard**: If `main.virtualKeyboard` is true and the Qt VirtualKeyboard module is available, the on-screen keyboard becomes available for touch-only deployments.
- **Keyboard shortcuts**: Use `SHIFT+TAB` to exit an active web app session and return to the desktop; other shortcuts may be provided by individual web apps.
