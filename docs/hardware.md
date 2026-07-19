# Hardware requirements

FairWindSK is a Qt 6 marine Multi-Functional Display shell for Signal K based systems. It can run on desktop computers during development, but vessel installations should use hardware designed for continuous operation, marine power, vibration, heat, and network integration.

## Reference hardware class

[Hat Labs HALPI2](https://docs.hatlabs.fi/halpi2/) is the reference class of hardware for a FairWindSK boat computer. It is a Raspberry Pi Compute Module 5 based system in an IP65 aluminium enclosure with protected wide-range DC input, NVMe storage, RTC, CAN FD/NMEA 2000, RS-485/NMEA 0183, Ethernet, USB, HDMI, and external antenna support. Use it as the baseline when selecting equivalent hardware.

Recommended platform characteristics:

- Raspberry Pi Compute Module 5 or an equivalent ARM64/x86_64 computer with enough GPU support for Qt 6 widgets and the selected web backend.
- 4 GB RAM minimum; 8 GB RAM recommended when running FairWindSK together with Signal K Server, OpenCPN, browser-based instrument panels, or logging services.
- Reliable solid-state storage. NVMe SSD storage is recommended for logs, charts, cache files, and application updates.
- 9-36 VDC or 10-32 VDC vessel power input, with reverse-polarity, over-voltage, transient, and EMI protection.
- Graceful shutdown support for power loss. A super-capacitor, UPS HAT, or monitored auxiliary power path is recommended.
- Passive or sealed thermal design suitable for unattended helm or nav-station operation.
- Real-time clock with backup battery so timestamps remain meaningful when GNSS or network time is unavailable.
- Gigabit Ethernet for Signal K, chart, and service traffic.
- Wi-Fi and Bluetooth only as secondary links; critical navigation data should use wired or marine network interfaces.
- Marine data interfaces appropriate to the vessel: CAN/CAN FD for NMEA 2000 and isolated RS-422/RS-485 for NMEA 0183.
- A display output compatible with the target helm display, normally HDMI or DSI/MIPI depending on the enclosure and monitor.
- USB ports with known current limits for GNSS receivers, touch controllers, storage, and maintenance devices.

## Display and touch

The primary display should be readable at normal helm distance in day, dusk, night, and red/low-light conditions. Use a sunlight-readable marine monitor or a protected panel display when the unit is installed outside a dry cabin.

Minimum display guidance:

- 1024 x 600 minimum resolution for the full shell.
- 7 inch minimum diagonal for comfortable touch operation; 10 inch or larger is recommended for MFD-style use.
- Capacitive or resistive touch supported by the operating system as a normal pointer device.
- Touch targets should remain comfortable while the vessel is moving, so avoid using FairWindSK on very small touch panels for primary navigation.

## Operating system and Qt

FairWindSK uses the CMake Qt 6 build. Desktop builds require Qt 6 with Qt WebEngine. Android and iOS builds use Qt WebView instead of Qt WebEngine. Android devices must run Android 13/API 33 or newer; test launcher deployments on a representative API 33 tablet or emulator before helm use.

Recommended onboard software base:

- Raspberry Pi OS, HaLOS, OpenPlotter, or another maintained Debian-family ARM64 image for Raspberry Pi class hardware.
- Qt 6 packages matching the FairWindSK target platform.
- Signal K Server reachable on localhost or the vessel network.
- System services for time synchronization, controlled shutdown, and log rotation.

## Vessel integration

A robust installation should include:

- Dedicated fused power feed sized for the computer, display, and attached USB devices.
- Common vessel ground and shielding practice appropriate for the installation.
- NMEA 2000 drop cable and proper bus termination when connected to a CAN/NMEA 2000 backbone.
- Isolated NMEA 0183 wiring for legacy instruments.
- Ethernet to the onboard router or Signal K network segment.
- Physical strain relief, drip loops, and service access for every connector.

## Minimum development hardware

For development and testing away from the vessel:

- macOS, Linux, or Windows workstation supported by Qt 6.
- 8 GB RAM minimum; 16 GB recommended for Qt WebEngine builds.
- CMake, a C++17 compiler, and the Qt modules described in [Building FairWindSK](./building.md).
- Network access to a Signal K test server or recorded Signal K data.

## Suitability checklist

Before treating hardware as FairWindSK-ready, verify:

- FairWindSK starts in the intended window mode without opening external windows.
- The Top Bar, Application Area, drawer dialogs, and Bottom Bar remain readable in every comfort preset: dawn, day, default, dusk, night, and sunset.
- Touch actions are reliable on the selected display at the installed helm distance.
- Signal K subscriptions remain stable over the intended wired network.
- Shutdown and restart behavior is safe when vessel power is removed and restored.
- CPU, storage, and enclosure temperatures remain within the hardware vendor limits during sustained chart, browser, and instrument use.
