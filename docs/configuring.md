# Configuring FairWindSK

FairWindSK stores its runtime settings in `fairwindsk.json`, with the location recorded in `fairwindsk.ini` (managed by Qt). If the JSON file is missing, defaults are copied from the embedded `resources/json/configuration.json`.

## Configuration files

- **`fairwindsk.json`**: Main user-editable configuration containing connection details, app definitions, UI geometry, and unit preferences.
- **`fairwindsk.ini`**: Qt-managed settings that track the JSON file path (`config`), the debug flag, and the last authentication token (`token`).
- **`resources/json/configuration.json`**: Bundled defaults used to seed a new installation.

## Connection settings

```jsonc
{
  "connection": {
    "server": "http://your-signalk-host:3000"
  }
}
```

- `server`: Base URL of the Signal K server. The application appends `/signalk` for websocket data and `/skServer/webapps` for application discovery. Leave this empty to start offline; the desktop loads but no remote apps will appear.

## Application definitions

Applications live under the `apps` array. Each entry represents a web or local app:

```jsonc
{
  "name": "https://spotify.com",
  "description": "Spotify web application",
  "fairwind": {
    "active": true,
    "order": 990
  },
  "signalk": {
    "appIcon": "file://icons/spotify_icon.png",
    "displayName": "Spotify",
    "aboutUrl": "",
    "settingsUrl": "",
    "helpUrl": ""
  }
}
```

- `name`: URL for the app (supports `https://`, `http://`, and `file://` for local desktop tools like OpenCPN).
- `fairwind.active`: Whether the app tile is shown.
- `fairwind.order`: Ordering on the desktop (lower numbers appear first).
- `signalk.*`: Presentation metadata used by the UI. Icon paths support Qt resource prefixes (`:/resources/...`) or filesystem URLs.

Apps discovered from the Signal K server (`signalk-webapp` entries) are merged with this list during startup. Local overrides for ordering and activation are preserved.

## Units and data paths

`resources/json/configuration.json` defines the default Signal K data paths and units. You can override these in `fairwindsk.json`:

```jsonc
{
  "units": {
    "vesselSpeed": "kn",
    "windSpeed": "kn",
    "distance": "nm",
    "range": "rm",
    "depth": "mt"
  },
  "signalk": {
    "sog": "navigation.speedOverGround",
    "stw": "navigation.speedThroughWater",
    "hdg": "navigation.headingTrue"
  }
}
```

These mappings drive UI bars (autopilot, anchor, alarms, POB) and instrument readouts. Ensure any custom paths exist on your Signal K server.

## Window and input settings

```jsonc
{
  "main": {
    "windowMode": "windowed", // or "fullscreen"
    "windowWidth": 1024,
    "windowHeight": 600,
    "windowLeft": 0,
    "windowTop": 20,
    "virtualKeyboard": false,
    "autopilot": ""
  }
}
```

- `windowMode`: Choose `windowed` or `fullscreen` (useful for kiosk deployments).
- `windowWidth/Height/Left/Top`: Window geometry when not fullscreen.
- `virtualKeyboard`: Enables the Qt virtual keyboard module if available.
- `autopilot`: Default autopilot identifier for widgets that target a specific device.

## Authentication and tokens

Authentication tokens are stored in `fairwindsk.ini` under the `token` key and injected into the shared `QWebEngineProfile` cookie store as `JAUTHENTICATION`. This keeps sensitive credentials out of the JSON configuration. To reset authentication, remove the token from `fairwindsk.ini` and relaunch.

## Debugging

Set `debug=true` in `fairwindsk.ini` to enable verbose logging from the Signal K client, configuration loader, and application discovery routines. Debug mode also enables additional Qt WebEngine logging categories.
