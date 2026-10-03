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

## [0.1.0-alpha] - 2026-07-22

### Added

- Initial alpha release of the FairWindSK marine multi-functional display.
- Release packages for Raspberry Pi OS 64-bit, Linux x86-64, Windows, and macOS.
- SHA-256 checksums and a keyless Sigstore-signed release manifest.

[Unreleased]: https://github.com/OpenFairWind/FairWindSK/compare/v0.1.0-alpha...HEAD
[0.1.0-alpha]: https://github.com/OpenFairWind/FairWindSK/releases/tag/v0.1.0-alpha
