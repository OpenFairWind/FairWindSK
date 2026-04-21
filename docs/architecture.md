# FairWindSK architecture

FairWindSK is a Qt6 shell that launches Signal K web applications and exposes native bars for vessel operations. The core is written in C++17 and organized around a small set of classes that manage configuration, networking, and UI composition.

## High-level components

- **Application bootstrap (`main.cpp`)**: Initializes Qt, installs translations, shows the splash screen on desktop targets, and hands control to the `FairWindSK` singleton before opening the main window. Desktop builds also configure WebEngine features such as accelerated canvas and plugin support, while mobile builds initialize Qt WebView.
- **Singleton core (`FairWindSK`)**: Central orchestrator that loads configuration, negotiates the Signal K connection, synchronizes installed web apps, and exposes global services (the shared desktop `QWebEngineProfile` when available, the `signalk::Client`, and the app registry).
- **Configuration management (`Configuration`)**: Loads and saves `fairwindsk.json`, seeds defaults from `resources/json/configuration.json`, and persists user choices (window geometry, unit preferences, application list, and plugin-specific settings). Token storage lives in `fairwindsk.ini` via `QSettings`.
- **Application registry (`AppItem`)**: Represents a web or local application, including metadata (name, description, icon), activation state, ordering, and optional settings/help/about URLs. Items are refreshed from the Signal K server’s Apps API (`/signalk/v1/apps/list`) and merged with local overrides. Desktop builds can launch local `file://` applications.
- **Signal K client (`signalk::Client`)**: Manages REST and websocket communication with the server using URLs derived from `connection.server` plus `/signalk`, including authentication tokens shared through the WebEngine cookie store. Startup and reconnect discovery are now timer-driven instead of retry-loop driven, and the client publishes explicit `connecting`, `live`, `stale`, `reconnecting`, `degraded`, and `disconnected` shell states together with the last live-update timestamp.
- **UI layers (`ui/` directory)**: Implements the Qt widgets for the desktop, top/bottom bars, settings panels, and embedded web views. A shared `ui/web/WebView` facade keeps the desktop `QWebEngineView` path and the mobile Qt WebView path behind the same widget-oriented API so the higher-level UI classes do not fork.

The formal names and behavioral rules for the shell surfaces are defined in [docs/ui_shell.md](./ui_shell.md). In particular, contributor discussions should distinguish between the **Top Bar**, **Bottom Bar**, **Application Area**, **Bottom Bar horizontal drawer**, and **Application Area vertical drawer** instead of using generic terms such as popup, right menu, or main view.

## Data and configuration flow

1. **Startup**: `FairWindSK::loadConfig()` reads `fairwindsk.ini` for the JSON configuration path and debug flag, then loads or seeds `fairwindsk.json`.
2. **Server connection**: `FairWindSK::startSignalK()` builds parameters from the configuration and starts a non-blocking Signal K discovery/connection cycle, persisting any returned token back to `fairwindsk.ini` once authentication succeeds.
3. **Application discovery**: `FairWindSK::loadApps()` requests `/signalk/v1/apps/list` from the configured server and falls back to the legacy `/skServer/webapps` path if needed. Each returned web app becomes an `AppItem`. Local apps in `fairwindsk.json` are merged, preserving order and activation. Inactive or missing server apps are demoted but retained.
4. **UI initialization**: The main window consumes the `AppItem` registry to populate the desktop. Bars subscribe to Signal K paths defined in `resources/json/configuration.json`, matching autopilot, anchor, POB, and alarm data fields.

## Restart and reconnect recovery

- When the websocket disconnects unexpectedly, the Signal K client schedules an automatic recovery attempt instead of staying passive.
- Recovery performs a fresh server discovery request before reopening the websocket, so endpoint or capability changes caused by a server restart are picked up.
- Existing subscriptions are re-sent after reconnect. Exact-path subscriptions are also hydrated with a fresh snapshot, which keeps native bars and status widgets consistent even if no new delta arrives immediately.
- REST-backed models such as MyData resource collections listen for the reconnect resynchronization event and reload their data after the stream recovers.
- `FairWindSK` itself refreshes unit preferences, app discovery, and automatic comfort-view availability after a recovered connection so the overall runtime state matches the restarted server.

## Shell health and operator trust

- `FairWindSK` now aggregates Signal K connectivity, app-catalog refresh status, and foreground hosted-app degradation into a compact runtime-health model that feeds the shell chrome.
- `ui/widgets/SignalKStatusIconsWidget` presents that model as a helm-readable badge plus REST/stream indicators, while `ui/widgets/SignalKServerBox` echoes the same state with last-live-update freshness text.
- Critical operational readouts in the Top Bar and the anchor/autopilot bars now render explicit `live`, `stale`, and `missing` states instead of silently disappearing or leaving stale values looking current.
- Alarm buttons in the bottom bar use accent styling when active so emergency conditions remain visually dominant across comfort presets, including low-glare/night modes.

## Comfort auto mode

- Automatic comfort view selection no longer probes `environment.sun` through a synchronous REST lookup during runtime reconfiguration.
- When the `environment.sun` Signal K path is configured, `FairWindSK` now listens for streamed environment updates and resolves the active comfort preset from the received sun state or elevation when available.
- If no usable environment context has arrived yet, comfort auto mode falls back to the deterministic clock-based preset mapping already used by the shell.

## Authentication model

Tokens are stored outside the main JSON configuration to avoid accidental check-in. `Configuration::getToken()` and `setToken()` read/write `fairwindsk.ini`. On desktop builds, `FairWindSK` injects the token into the shared `QWebEngineProfile` cookie store as `JAUTHENTICATION`, so embedded web apps reuse that authentication context. Mobile builds use the alternate Qt WebView backend and therefore have a narrower shared-cookie integration surface than desktop.

## Extending the application

- **Adding custom web apps**: Insert entries under `apps` in `fairwindsk.json` with `name` pointing to an HTTPS, HTTP, or `file://` URL. Provide `signalk.appIcon` and `signalk.displayName` to control the tile presentation.
- **Integrating native features**: Bars and custom widgets should consume data paths already listed under the `signalk` block in `resources/json/configuration.json` or add new keys that map to Signal K paths.
- **Platform adaptations**: The singleton exposes window sizing and mode fields (fullscreen vs. windowed). Desktop-only integrations such as `QHotkey`, ZeroConf discovery, and native `file://` application launching remain part of the desktop builds, while Android and iOS now rely on the alternate Qt WebView based rendering path and keep those desktop-specific integrations disabled.
