# FairWindSK architecture

FairWindSK is a Qt6 desktop shell that launches Signal K web applications and exposes native bars for vessel operations. The core is written in C++17 and organized around a small set of classes that manage configuration, networking, and UI composition.

## High-level components

- **Application bootstrap (`main.cpp`)**: Initializes Qt, installs translations, shows the splash screen, and hands control to the `FairWindSK` singleton before opening the main window. It sets up default WebEngine features such as accelerated canvas and plugin support.
- **Singleton core (`FairWindSK`)**: Central orchestrator that loads configuration, negotiates the Signal K connection, synchronizes installed web apps, and exposes global services (the shared `QWebEngineProfile`, the `signalk::Client`, and the app registry).
- **Configuration management (`Configuration`)**: Loads and saves `fairwindsk.json`, seeds defaults from `resources/json/configuration.json`, and persists user choices (window geometry, unit preferences, application list, and plugin-specific settings). Token storage lives in `fairwindsk.ini` via `QSettings`.
- **Application registry (`AppItem`)**: Represents a web or local application, including metadata (name, description, icon), activation state, ordering, and optional settings/help/about URLs. Items are refreshed from the Signal K server’s `/skServer/webapps` listing and merged with local overrides.
- **Signal K client (`signalk::Client`)**: Manages websocket communication with the server using URLs derived from `connection.server` plus `/signalk`, including authentication tokens shared through the WebEngine cookie store.
- **UI layers (`ui/` directory)**: Implements the Qt widgets for the desktop, top/bottom bars, settings panels, and embedded web views. Bars read the `Configuration` paths (e.g., `navigation.anchor.*`, autopilot targets) to bind to Signal K data.

## Data and configuration flow

1. **Startup**: `FairWindSK::loadConfig()` reads `fairwindsk.ini` for the JSON configuration path and debug flag, then loads or seeds `fairwindsk.json`.
2. **Server connection**: `FairWindSK::startSignalK()` builds parameters from the configuration and attempts websocket connectivity, persisting any returned token back to `fairwindsk.ini`.
3. **Application discovery**: `FairWindSK::loadApps()` requests `/skServer/webapps` from the configured server. Each returned entry tagged with `signalk-webapp` becomes an `AppItem`. Local apps in `fairwindsk.json` are merged, preserving order and activation. Inactive or missing server apps are demoted but retained.
4. **UI initialization**: The main window consumes the `AppItem` registry to populate the desktop. Bars subscribe to Signal K paths defined in `resources/json/configuration.json`, matching autopilot, anchor, POB, and alarm data fields.

## Authentication model

Tokens are stored outside the main JSON configuration to avoid accidental check-in. `Configuration::getToken()` and `setToken()` read/write `fairwindsk.ini`, while `FairWindSK` injects the token into the shared `QWebEngineProfile` cookie store as `JAUTHENTICATION`. Web apps launched through the profile reuse this authentication context.

## Extending the application

- **Adding custom web apps**: Insert entries under `apps` in `fairwindsk.json` with `name` pointing to an HTTPS, HTTP, or `file://` URL. Provide `signalk.appIcon` and `signalk.displayName` to control the tile presentation.
- **Integrating native features**: Bars and custom widgets should consume data paths already listed under the `signalk` block in `resources/json/configuration.json` or add new keys that map to Signal K paths.
- **Platform adaptations**: The singleton exposes window sizing and mode fields (fullscreen vs. windowed). On embedded platforms, adjust these in `fairwindsk.json` to match the display resolution and desired kiosk behavior.
