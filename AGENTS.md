# AGENTS: FairWindSK

These instructions apply to the entire repository.

## Code style
- Match the existing C++17/Qt6 style (4-space indentation, brace placement, and include ordering).
- Keep header and source files in sync; prefer small, focused changes that respect current patterns.
- Avoid code duplication; when the same behavior or UI pattern appears in more than one place, factor it into a fully reusable component instead of copying logic.
- Avoid introducing new third-party dependencies unless absolutely necessary.
- Do not wrap imports or includes in `try/catch`.
- Line-by-line pedagogical comments.

## Build & testing
- Use the CMake build (`cmake -S . -B build` then `cmake --build build`) when you need to compile locally; desktop builds require Qt6 with QtWebEngine, while Android and iOS builds require Qt6 with Qt WebView instead of QtWebEngine.
- Run available tests or sanity checks when feasible; note any environment limitations in your report if you cannot run them.
- Be sure that each new modification is checked for behavior and build impact across macOS, Windows, Linux, Raspberry Pi OS, Android, and iOS; when a platform is not currently supported by the codebase, call out the limitation explicitly instead of implying coverage.
- For every GUI change, explicitly verify touch-friendly behavior and marine electronics multi-functional display (MFD) consistency before considering the task done.
- Touch targets must be finger-friendly by default (comfortable hit area, clear focus/pressed states, and readable high-contrast text at normal helm distance).
- Layout, spacing, and visual hierarchy must remain stable in rough-use conditions: avoid dense controls, ambiguous labels, or low-contrast decorative styling that reduces operational clarity.
- Ensure all comfort presets: 1) fit the marine electronics UI style; 2) ensure the best visibility for down, day, default, dusk, night, and sunset.

## Documentation & resources
- Update user-facing strings or documentation when behavior changes; keep resource paths (e.g., `resources.qrc`) consistent if assets move.
- Update existing Markdown documentation under `docs/` and add new Markdown files when a change introduces a substantial workflow, feature, or maintenance improvement that should be captured for future contributors or users.

## Built feature definitions
- **MyData**: lets the user interact with `Waypoints`, `Routes`, `Regions`, `Notes`, `Charts`, `Tracks`, and `Files`, including both local and remote files.
- **POB**: tracks one or more Person(s) Over Board. Clicking the icon sets a new POB. The POB dialog overlays the Bottom Bar and shows the last known position of the current POB, with support for selecting among multiple POBs, together with the bearing, range, and elapsed time. The user can cancel the active POB from the dialog. To close the POB dialog and return to the regular Bottom Bar, the user must click the rightmost POB dialog icon.
- **Autopilot**: the icon is active only when the Autopilot plugin is installed and its APIs make autopilot interaction consistent. When the user clicks the Autopilot icon, the Autopilot dialog appears covering the Bottom Bar. The Autopilot dialog lets the user go to the next waypoint, switch to wind-vane mode, tack, gybe, trim by `1` degree or `10` degrees to port or starboard, select the route, dodge, and set auto mode. To close the Autopilot dialog and return to the regular Bottom Bar, the user must click the rightmost dialog icon.
- **Apps**: acts as the home button. When clicked, the current application shown in the Application Area is hidden and the launcher home is shown with the application tiles. Applications can be organized in pages and sub-pages. Clicking an application tile launches that application.

## Operational guidelines
- Prefer `rg` for searches over recursive `ls`/`grep`.
- Follow repo-level reporting norms: provide a concise summary of changes and list the commands you ran, prefixing each command with the appropriate status emoji.
