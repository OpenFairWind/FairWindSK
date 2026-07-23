# FairWindSK owner manuals

The English and Italian owner manuals are maintained as parallel LaTeX sources in
`docs/manual/english` and `docs/manual/italian`. Update both manuals whenever a
code or documentation change affects installation, configuration, visible UI,
operator guidance, or supported platforms.

## Build both PDFs locally

Install Qt 6 and the normal FairWindSK desktop dependencies, XeLaTeX with the
packages used by the manuals, DejaVu fonts, Poppler tools, Xvfb, and
`xauth`. On Ubuntu the CI workflow is the authoritative dependency example.

Build FairWindSK and then run:

```bash
cmake -S . -B build-manual -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build-manual --parallel 2
xvfb-run -a -s '-screen 0 1280x800x24' scripts/manual/build_manuals.sh --build-dir build-manual
```

On a desktop with a display server, omit `xvfb-run`. The build launches the real
application in its automated screenshot mode, exercises the launcher,
Settings, MyData, and Bottom Bar panels, writes localized PNG files into each
manual's `figures` directory, and then runs XeLaTeX three times per language.

The outputs are `docs/manual/english/fairwindsk-manual.pdf` and
`docs/manual/italian/fairwindsk-manuale.pdf`. These PDFs are generated artifacts
and are not committed.

If screenshots are already current, use
`scripts/manual/build_manuals.sh --skip-screenshots`. GitHub Actions also offers
the **Manuals** workflow through `workflow_dispatch`; it rebuilds screenshots and
uploads both PDFs as artifacts. The workflow runs automatically when manual,
user-facing documentation, UI, localization, or screenshot automation files
change.
