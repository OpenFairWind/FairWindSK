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

## Documentation & resources
- Update user-facing strings or documentation when behavior changes; keep resource paths (e.g., `resources.qrc`) consistent if assets move.
- Update existing Markdown documentation under `docs/` and add new Markdown files when a change introduces a substantial workflow, feature, or maintenance improvement that should be captured for future contributors or users.

## Operational guidelines
- Prefer `rg` for searches over recursive `ls`/`grep`.
- Follow repo-level reporting norms: provide a concise summary of changes and list the commands you ran, prefixing each command with the appropriate status emoji.
