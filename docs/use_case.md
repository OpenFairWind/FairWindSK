# Typical use cases

FairWindSK provides a kiosk-like desktop for running Signal K web applications alongside native controls. Below are common workflows that align with the current feature set.

## Navigation dashboard with Signal K apps

- Connect FairWindSK to a vessel’s Signal K server and let it discover installed web apps such as Freeboard-SK or KIP through `/skServer/webapps`.
- Use the desktop tiles to launch each app in its own web view. SHIFT+TAB returns focus to the FairWindSK desktop when a web app occupies the screen.
- Keep the bottom bars visible for quick access to alarms, person overboard recovery, anchor status, and autopilot state.

## Mixed local and remote applications

- Add local navigation tools (e.g., OpenCPN) using `file://` URLs in `fairwindsk.json`. Provide custom icons and display names through the `signalk` metadata block.
- Combine these with remote web apps from the Signal K catalog; ordering is controlled via the `fairwind.order` field so both local and server-hosted tools appear in one grid.

## Autopilot and anchor workflows

- Configure the autopilot and anchor plugin identifiers in `fairwindsk.json` under `applications.autopilot` and `applications.anchor`. The bottom bars check these entries to decide whether to expose controls.
- Map the relevant Signal K paths (rudder angle, target heading, anchor radius, alarms) in the `signalk` section so UI widgets display live data and actions.

## Onboard file and data management

- Use the “MyData” components to browse Signal K resources such as waypoints, tracks, routes, and files when the corresponding server plugins are available.
- The file browser and viewers handle images, PDFs, and text when delivered through the server’s resource endpoints.

## Embedded and kiosk deployments

- Set `main.windowMode` to `fullscreen` and adjust window geometry for the target display resolution in `fairwindsk.json`.
- Enable the Qt virtual keyboard via `main.virtualKeyboard=true` for touch-only installations.
- Utilize the provided autostart entries (`extras/fairwindsk-startup.desktop` and `extras/fairwindsk-startup`) on Raspberry Pi OS to launch at boot.
