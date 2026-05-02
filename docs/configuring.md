# Configuring FairWindSK

FairWindSK stores its runtime settings in `fairwindsk.json`, with the location recorded in `fairwindsk.ini` (managed by Qt). Both files live in the per-user FairWindSK configuration directory. If the JSON file is missing, defaults are copied from the embedded `resources/json/configuration.json`.

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

- `server`: Base URL of the Signal K server. The application appends `/signalk` for websocket data and `/signalk/v1/apps/list` for standard application discovery, with a legacy fallback to `/skServer/webapps`. Leave this empty to start offline; the desktop loads but no remote apps will appear.

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

- `name`: URL for the app. Use `https://` or `http://` so the app stays embedded in the FairWindSK application area; `file://` app entries are retained for configuration compatibility but are blocked by single-window mode.
- `fairwind.active`: Whether the app tile is shown.
- `fairwind.order`: Ordering on the desktop (lower numbers appear first).
- `fairwind.zoomPercent`: Per-app web zoom level, expressed as a percentage. The default is `100`.
- `signalk.*`: Presentation metadata used by the UI. Icon paths support Qt resource prefixes (`:/resources/...`), filesystem URLs, and the bundled `file://icons/...` icon set. Bundled icons are also compiled into the application resources so Raspberry Pi OS launches keep their artwork even when the copied icon directory is not available yet.

Apps discovered from the Signal K server (`signalk-webapp` entries) are merged with this list during startup. Local overrides for ordering and activation are preserved. `file://` entries can still carry local metadata such as icons, but they are not launched as external native applications.

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
  "unitPreferences": {
    "overrides": {
      "speed": "kn",
      "distance": "nm"
    }
  },
  "signalk": {
    "sog": "navigation.speedOverGround",
    "stw": "navigation.speedThroughWater",
    "hdg": "navigation.headingTrue"
  }
}
```

FairWindSK now prefers Signal K server unit preferences when the server exposes `displayUnits` metadata. The dedicated **Settings > Units** tab shows the current server preset, lets you sync the server choices into local configuration, and lets you enable per-category local overrides that are saved under `unitPreferences.overrides`.

The legacy `units` object is still kept for compatibility with parts of the UI that have not yet moved to the server-driven metadata path. FairWindSK updates those legacy keys when you save local category overrides from the Units tab.

These mappings drive UI bars (autopilot, anchor, alarms, POB) and instrument readouts. For the POB workflow on a regular Signal K server, keep `notifications.pob` mapped to the standard `notifications.mob` path unless you intentionally need a custom server extension.

## Window and input settings

```jsonc
{
  "main": {
    "language": "system", // or "en", "it"
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

- `language`: Selects the application language. `system` follows the operating-system language when FairWindSK supports it and falls back to English for every unsupported language; `en` forces English; `it` forces Italian. Any other value is treated as English. Restart FairWindSK after changing it so all native widgets and embedded web views start with the same language and culture.
- `windowMode`: Choose `windowed` or `fullscreen` (useful for kiosk deployments).
- `windowWidth/Height/Left/Top`: Window geometry when not fullscreen.
- `virtualKeyboard`: Enables the Qt virtual keyboard module if available. The setting is read during startup, so restart FairWindSK after changing it.
- `autopilot`: Default autopilot identifier for widgets that target a specific device.

## Diagnostics and logging

```jsonc
{
  "diagnostics": {
    "logLevel": "off",
    "persistentLogs": true,
    "interactionHistory": true,
    "email": "hpsclab@uniparthenope.it",
    "subject": "FairWindSK diagnostics"
  }
}
```

- `logLevel`: Logging verbosity. Supported values are `off`, `critical`, `warning`, `info`, `debug`, and `full`. The default is `off` (`No logging` in the UI).
- `persistentLogs`: When true, FairWindSK stores per-run message logs in the per-user application data directory. This is enabled by default.
- `interactionHistory`: When true, FairWindSK stores a lightweight journal of operator navigation and control actions beside each run log. This is enabled by default.
- `email`: Optional fallback email address recorded in the diagnostics bundle manifest. Single-window mode does not open the platform email composer.
- `subject`: Subject line recorded for the diagnostics handoff.

At startup, FairWindSK checks whether the previous run ended gracefully. If it did not, it creates a persistent crash-report bundle in the per-user diagnostics reports directory. The bundle includes a text summary, a JSON manifest, the previous run log when available, the previous interaction journal when available, and platform details such as operating system, CPU architecture, Qt version, host information, and primary screen configuration.

If a fallback email address is configured, FairWindSK records it beside the stored report bundle so the operator can attach the files later without FairWindSK opening an external mail window.

The **Settings > System** tab exposes the same diagnostics options in the touch-friendly UI and shows both the persistent log directory and the crash-report directory currently used by the application.
It keeps the persistent-log and interaction-history toggles readable on compact touch layouts by separating each switch from its descriptive text and relies on the in-application log explorer so operators can inspect both stored run logs and stored crash summaries without leaving FairWindSK.

## Comfort presets and icon color

FairWindSK ships with editable comfort preset stylesheets for `default`, `dawn`, `day`, `sunset`, `dusk`, and `night`. The bundled presets are now aligned to a marine multifunction-display visual language (high contrast, large touch targets, and predictable accent behavior), and each preset can also store:

- a generated visual palette override
- optional background images for major surfaces and control states
- a preset-specific default SVG icon tint (`comfortViewPalette.<preset>.iconDefault`)
- preset-level palette colors for:
  - application background (`comfortViewPalette.<preset>.applicationBackground`)
  - button background (`comfortViewPalette.<preset>.buttonBackground`)
  - scroll bar background (`comfortViewPalette.<preset>.scrollBarBackground`)
  - scroll bar knob (`comfortViewPalette.<preset>.scrollBarKnob`)

The **Settings > Comfort** page writes those values back into `fairwindsk.json`, so different presets can keep different icon colors without changing the base SVG assets.
Its touch color picker also keeps a reusable custom-color set in the per-user settings store, provides a shade selector for fast tap-driven color choice, and uses a compact scroll-safe layout that fits the shared Bottom Bar horizontal drawer interaction pattern.

## My Data behavior

- **Waypoints**: the details page shows the selected waypoint in the reusable embedded Signal K app view when a Freeboard-SK app is available on the connected server.
- **Files**: file actions operate on the active view selection, so search-result rows and normal directory rows follow the same open, rename, delete, copy, cut, and paste workflows.

## Launcher editing

The **Settings > Apps** page stores launcher pages and folders under the `launcherLayout` object in `fairwindsk.json`.
The editor now exposes grouped actions for:

- page-level actions such as adding pages, showing page details, assigning applications to the selected page, and clearing applications from the current page or all pages
- palette-level actions such as creating applications, showing selected application details, removing selected applications from the palette, and adding all available applications to the current page

Application details also expose the stored web zoom percentage so each embedded web app can be enlarged or reduced without affecting the others.
Launcher, settings, and embedded web interactions should follow the formal shell vocabulary from [docs/ui_shell.md](./ui_shell.md), especially when a flow uses the Bottom Bar horizontal drawer or the Application Area vertical drawer.

## Authentication and tokens

Authentication tokens are stored in `fairwindsk.ini` under the `token` key and injected into the shared `QWebEngineProfile` cookie store as `JAUTHENTICATION`. This keeps sensitive credentials out of the JSON configuration. To reset authentication, remove the token from `fairwindsk.ini` and relaunch.

## Debugging

Set `debug=true` in `fairwindsk.ini` to enable verbose logging from the Signal K client, configuration loader, and application discovery routines. Debug mode also enables additional Qt WebEngine logging categories.

Independent of `debug=true`, the application now keeps its own diagnostics log lifecycle based on the `diagnostics` section above.
