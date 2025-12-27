# AGENTS: FairWindSK

These instructions apply to the entire repository.

## Code style
- Match the existing C++17/Qt6 style (4-space indentation, brace placement, and include ordering).
- Keep header and source files in sync; prefer small, focused changes that respect current patterns.
- Avoid introducing new third-party dependencies unless absolutely necessary.
- Do not wrap imports or includes in `try/catch`.

## Build & testing
- Use the CMake build (`cmake -S . -B build` then `cmake --build build`) when you need to compile locally; Qt6 and QtWebEngine are required.
- Run available tests or sanity checks when feasible; note any environment limitations in your report if you cannot run them.

## Documentation & resources
- Update user-facing strings or documentation when behavior changes; keep resource paths (e.g., `resources.qrc`) consistent if assets move.

## Operational guidelines
- Prefer `rg` for searches over recursive `ls`/`grep`.
- Follow repo-level reporting norms: provide a concise summary of changes and list the commands you ran, prefixing each command with the appropriate status emoji.
