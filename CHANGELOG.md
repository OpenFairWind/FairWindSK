# Changelog

All notable changes to FairWindSK are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
- **Settings > System**: Reset now undoes the changes made since Settings was opened; before, it
  had nothing to discard because every change is saved as it is made.

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

[Unreleased]: https://github.com/OpenFairWind/FairWindSK/compare/v0.1.0-alpha...HEAD
[0.1.0-alpha]: https://github.com/OpenFairWind/FairWindSK/releases/tag/v0.1.0-alpha
