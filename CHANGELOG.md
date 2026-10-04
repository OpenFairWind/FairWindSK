# Changelog

All notable changes to FairWindSK are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] - 2026-10-04

### Added

- Installable release packages for macOS on Apple Silicon and Intel, Windows x86-64,
  Debian x86-64, Red Hat compatible x86-64 distributions, Raspberry Pi OS arm64,
  Android arm64, and iOS arm64.
- Release-time package inspection verifies architecture, signatures, dependency metadata,
  and the presence of bundled Qt runtimes before publication.

### Fixed

- The Signal K stream is opened once per connection attempt instead of twice, and a server that
  advertises no stream endpoint now triggers a retry instead of leaving the client stalled.
- Pausing or reconfiguring the Signal K connection while discovery is in flight no longer lets the
  superseded reply reopen the stream against the previous settings.
- Subscriptions can be added, removed, or lose their receiver during delta dispatch and snapshot
  hydration without corrupting the subscription list.
- Subscription paths match whole path segments, so `navigation.speedThroughWater` no longer
  receives `navigation.speedThroughWaterTransverse` updates.
- Malformed application entries from the server catalog or from `fairwindsk.json` no longer abort
  the application registry rebuild, and `Configuration::findApp()` returns the real array index.
- **Settings > System**: Reset, Restore Defaults and Import no longer destroy the page while its
  own button handler is still running; Import stays on the System tab and applies the imported
  configuration to the running shell.
- **Settings > System**: exporting onto the active `fairwindsk.json` (the suggested name) no longer
  deletes it, and a failed export leaves an existing target file intact.
- **Settings > Main**: window position and size are applied when editing finishes, with a minimum
  size of 480x320, instead of resizing the window to every half-typed value.
- **Settings > Signal K**: path mappings missing from an older configuration file are listed and
  can be filled in; entered paths are trimmed.
- **Settings > Connection**: the access token is no longer written to the debug log, placeholder
  server URLs were removed from the suggestions, and a pending access request that the server no
  longer knows stops polling.
- **Settings > Widgets**: leaving a field unchanged no longer rewrites the configuration, and only
  path, policy or period changes restart the Signal K connection.
- **Settings > Connection**: a host name, FQDN or IP address typed in the server field is now
  actually used. Before, the field reported the selected list item instead of the typed text, and
  a discovered server could overwrite the text while it was being typed. Pressing Connect (or
  Return) on a new address connects to it even while another server is live, and the address is
  remembered in `connection.servers` so it stays in the drop-down list after a restart.
- **Settings > Comfort**: Reset All asks for confirmation.
- **Launcher**: server applications are no longer parked as inactive before the catalog has been
  received, which left the launcher empty after the first connection; applications parked that way
  come back when the server lists them again.
- **Top Bar**: the default COG and SOG widgets carried a `vessels.self.` prefix in their path and
  never showed a value; the factory defaults are corrected and such paths are normalized on load.
- **Settings > Widgets**: the editor is filled for the widget selected when the page opens. It
  used to stay blank, and the first touched field saved that blank form over the widget.
- **Settings**: tabs create their pages again after Reset, Restore Defaults or Import; the
  tab-change handler was dropped during the rebuild, leaving every other tab blank.
- **Settings > Connection**: picking a server from the list only selects it; the configured server
  changes when Connect is pressed, so Connect/Pause always refers to the live connection.
- Signal K client: a 404 on a read is treated as "no data" instead of an error shown in the status
  area; bundled application icons are no longer requested from the server; the missing
  `alerts.svg` resource is registered; the socket is no longer written to after being aborted.
- **Settings > System**: Reset now undoes the changes made since Settings was opened; before, it
  had nothing to discard because every change is saved as it is made.

- **Access token**: the token is sent in the `Authorization` header (REST and stream) instead of
  being copied into request bodies, where it was stored inside saved waypoints, routes and notes
  and turned every read into a GET with a body. The stream is now authenticated too.
- **Access token**: a token the server rejects (expired, revoked, issued by another server) is
  detected, dropped and reported; the client continues in public mode and Request Token becomes
  available again. A token is tied to the server that issued it and is never offered to another.
- **Request Token** reuses one client id per installation instead of registering a new device on
  the server for every request, and shows the server's explanation when a request is refused.
- **Write operations** (waypoints, routes, regions, notes, course to waypoint): a refused or failed
  write is reported as such instead of success, with a message pointing to Settings > Connection
  when an access token is missing.
- **POB** uses the position received on the stream, so marking a person overboard no longer depends
  on a REST request answering at that moment.
- Connecting in public mode no longer posts an empty login to the server, and tokens are no longer
  written to the debug log.

- **Alarms bar**: pressing an idle alarm now raises it and pressing an active one cancels it. The
  action was inverted (the button had already toggled itself when its state was read), so alarms
  could not be raised from the bar. A request the server refuses no longer shows the alarm as active.
- **Anchor bar**: every radius value received from the server was sent straight back as a "set
  radius" command, and dragging the slider sent one command per step. Received values are now only
  displayed, a drag sends one command on release, and the value is converted from the units shown.
- **Autopilot bar**: Gybe buttons follow pilot availability like the other controls, a refused
  command is shown on the panel, the pilot state read from the autopilot API is kept when the
  stream carries none, and the rudder indicator can no longer be dragged.
- The connection summary no longer alternates between "REST online" and "Stream online" on every
  request and delta, which broadcast a health change to the whole interface each time.
- Settings no longer re-enters "apply" while a previous apply is still running, which restarted
  the Signal K connection several times after Reset.

- **Autopilot bar**: Auto, Wind and Route are offered when the pilot lists them as states (as the
  Raymarine provider does) and are sent to the state endpoint; before, only the "modes" list was
  read and these buttons were missing. The pilot's answer to a refused command stays on the panel
  for a few seconds instead of being overwritten by the next stream update.
- **Anchor bar**: every action reports the server's answer; panel signals are emitted only for
  accepted commands, a refused radius returns the slider to the server's value, and a windlass
  stop is repeated up to three times, with a warning if it is never confirmed.
- **Alarms bar**: raising Abandon, Adrift, Fire, Piracy or Sinking asks for confirmation. POB stays
  immediate and cancelling never asks.
- **Units**: server unit preferences and per-path display units are fetched in the background.
  They were fetched with blocking requests while widgets were being drawn, which froze the display
  on a slow server and let connection handling and settings re-enter each other.
- Requests are no longer issued before the server has been discovered (they went out without a
  host), and background requests no longer overwrite the outcome of the last operator command.

### Changed

- Settings writes the configuration file once per change instead of up to four times, and the
  System diagnostics stop refreshing while the page is hidden.
- **Settings > Units** loads the server unit preferences asynchronously on every platform, so
  opening the page no longer blocks the interface on a slow server.
- Remapping Signal K paths or editing data widgets no longer restarts the Signal K connection;
  only connection changes (server URL, pause/connect, token) open a new session.

## [0.1.0-alpha] - 2026-07-22

### Added

- Initial alpha release of the FairWindSK marine multi-functional display.
- Release packages for Raspberry Pi OS 64-bit, Linux x86-64, Windows, and macOS.
- SHA-256 checksums and a keyless Sigstore-signed release manifest.

[Unreleased]: https://github.com/OpenFairWind/FairWindSK/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/OpenFairWind/FairWindSK/compare/v0.1.0-alpha...v0.1.0
[0.1.0-alpha]: https://github.com/OpenFairWind/FairWindSK/releases/tag/v0.1.0-alpha
