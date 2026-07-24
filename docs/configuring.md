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

## Data widgets and bar layouts

Top Bar and Bottom Bar instrument readouts are defined by the `dataWidgets` array. Each entry has an `id`, operator-facing `name`, `icon`, display `type`, Signal K path, optional source/default units, and Signal K subscription timing fields:

```jsonc
{
  "dataWidgets": [
    {
      "id": "sog",
      "name": "SOG",
      "icon": ":/resources/svg/OpenBridge/lcd-sog.svg",
      "type": "numerical",
      "signalKPath": "navigation.speedOverGround",
      "sourceUnit": "ms-1",
      "defaultUnit": "kn",
      "visualizationMode": "text",
      "updatePolicy": "instant",
      "period": 1000,
      "minPeriod": 200,
      "valueTextSize": 11,
      "valueTextColor": "",
      "labelTextSize": 10,
      "labelTextColor": "",
      "trendTextSize": 10,
      "trendIncreasingColor": "",
      "trendDecreasingColor": "",
      "showIcon": false,
      "showText": true
    }
  ]
}
```

Supported data types are `numerical`, `gauge`, `position`, `position-rows`, `datetime`, and `waypoint`. `position-rows` reads the same Signal K position object as `position`, but renders latitude and longitude on separate rows for a more readable MFD bar layout. For numeric values, `visualizationMode` can be `text` or `gauge`; the legacy `type: "gauge"` form is still accepted for compatibility.

The text styling fields let each data widget set value text size/color, label text size/color, trend marker size, and separate increasing/decreasing trend colors. Empty color strings use the active comfort preset. `showIcon` and `showText` are the widget defaults used when the widget is first placed on a bar or when an older bar entry does not specify per-entry display toggles.

`period`, `minPeriod`, and `updatePolicy` are passed through to the Signal K websocket subscription; FairWindSK sends `instant` by default and also supports `fixed` for servers that require a fixed cadence. The **Settings > Widgets** tab edits these definitions, while **Settings > Top Bar** and **Settings > Bottom Bar** place enabled widgets into `barLayouts.top` and `barLayouts.bottom`.

Each bar-layout entry stores the item kind (`widget`, `separator`, or `stretch`), the widget id when applicable, ordering, width/height expansion, and display toggles for icon, text, units, and trend. Signal K data widgets can be shown on both the Top Bar and Bottom Bar at the same time; each visible instance subscribes independently. Numeric and gauge data widgets render as MFD readouts: the header line contains text and/or the optional icon, while the value line contains the optional trend marker, converted value, display unit, and compact native Qt gauge when gauge mode is selected. The compact bar gauge is implemented with native Qt painting to keep the desktop, Raspberry Pi, Android, and iOS dependency surface stable; [Qwt](https://qwt.sourceforge.io/index.html) remains the preferred external Qt Widgets gauge family if FairWindSK later adds larger standalone dial or compass instruments. Fixed shell widgets are available from both bar editors and can be moved between the Top Bar and Bottom Bar, including the open-app strip, MyData, POB, Autopilot, Home, Anchor, Alarms, Settings, Signal K box, Clock and Status, and the compact Status indicator.

If an older configuration does not contain `dataWidgets`, FairWindSK derives the standard Position, COG, SOG, HDG, STW, DPT, waypoint, route, and VMG widgets from the legacy `signalk` paths so existing installs keep showing data.

## Window and input settings

```jsonc
{
  "main": {
    "language": "system", // or "en", "fr", "es", "it"
    "windowMode": "windowed", // "windowed", "centered", "maximized", or "fullscreen"
    "windowWidth": 1024,
    "windowHeight": 600,
    "windowLeft": 0,
    "windowTop": 20,
    "virtualKeyboard": false,
    "autopilot": ""
  }
}
```

- `language`: Selects the application language. `system` follows the operating-system language when FairWindSK supports it and falls back to English for every unsupported language; `en` forces English, `fr` French, `es` Spanish, and `it` Italian. Any other value is treated as English. Restart FairWindSK after changing it so all native widgets and embedded web views start with the same language and culture.
- `windowMode`: Choose `windowed`, `centered`, `maximized`, or `fullscreen`. On Raspberry Pi OS Linux ARM builds, `maximized` uses the desktop work area directly instead of relying on the window manager's maximize geometry, while `fullscreen` requests a frameless topmost kiosk window.
- `windowWidth/Height/Left/Top`: Window geometry when using `windowed` or `centered`; the settings UI constrains these values to the current screen work area.
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

On Android 13/API 33 and newer builds, **Settings > Android** lists installed activities that advertise themselves to the Android application launcher. Discovery runs outside the GUI thread so slow package metadata and icon loading cannot freeze scrolling on physical devices. The **Start FairWindSK as the Android launcher** touch checkbox opens Android's system-owned Home-role chooser; clearing it opens Home settings so the operator can choose another launcher. The native app icon is an immediate launch target; a separate reusable marine checkbox controls palette availability without overlapping the icon. Checked applications are stored as typed entries in the shared `apps` array and therefore appear alongside Signal K and configured web applications in **Settings > Applications**. They can be dragged or assigned to any launcher page. Android application identity, icon, package, and activity metadata are managed by the Android selector, so web-only edit and remove actions are disabled when an Android entry is selected; deselect it from **Settings > Android** instead. Deselecting it also clears its existing launcher-page slots.

Android entries use a stable name in the form `android:<package>/<activity>` and store `fairwind.source` as `android`, with `fairwind.androidPackage` and `fairwind.androidActivity` identifying the explicit launch component. Other platforms preserve these configuration records but do not expose the Android settings page or attempt to launch them.
Launcher, settings, and embedded web interactions should follow the formal shell vocabulary from [docs/ui_shell.md](./ui_shell.md), especially when a flow uses the Bottom Bar horizontal drawer or the Application Area vertical drawer.

## Authentication and tokens

Authentication tokens are stored in `fairwindsk.ini` under the `token` key and injected into the shared `QWebEngineProfile` cookie store as `JAUTHENTICATION`. This keeps sensitive credentials out of the JSON configuration. To reset authentication, remove the token from `fairwindsk.ini` and relaunch.

## Debugging

Set `debug=true` in `fairwindsk.ini` to enable verbose logging from the Signal K client, configuration loader, and application discovery routines. Debug mode also enables additional Qt WebEngine logging categories.

Independent of `debug=true`, the application now keeps its own diagnostics log lifecycle based on the `diagnostics` section above.
