# FairWindSK owner manuals

The English and Italian owner manuals are maintained as parallel LaTeX sources in
`docs/manual/english` and `docs/manual/italian`. Update both manuals whenever a
code or documentation change affects installation, configuration, visible UI,
operator guidance, or supported platforms.

## Build both PDFs locally

Install Qt 6 and the normal FairWindSK desktop dependencies, XeLaTeX with the
packages used by the manuals, DejaVu fonts, Poppler tools, Xvfb, and
`xauth`. On Ubuntu the CI workflow is the authoritative dependency example.

### macOS dependencies with Homebrew

Install the non-privileged Homebrew TeX Live formula and the manual fonts:

```bash
brew install texlive poppler
brew install --cask font-dejavu
fc-cache -f "$HOME/Library/Fonts"
```

The `texlive` formula supplies XeLaTeX without the administrator-password prompt
used by the BasicTeX package installer. Confirm the installation with
`xelatex --version`. If XeLaTeX reports that DejaVu fonts cannot be found after
installing them, rerun the `fc-cache` command from a normal terminal session.

On macOS, build FairWindSK and run the manual script directly from the logged-in
desktop session; Xvfb is only needed for headless Linux environments.

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
