# FairWindSK store publishing package

This directory contains the editable metadata and upload-ready graphics for
Apple App Store Connect and Google Play Console.

- [`appstore/`](./appstore/) contains iOS/iPadOS metadata and screenshots.
- [`playstore/`](./playstore/) contains Android metadata and preview assets.
- [`privacy-policy.md`](./privacy-policy.md) is the public privacy-policy source.
- [`support.md`](./support.md) is the public support-page source.

English (`en-US`) is the primary listing language. Italian (`it-IT`) mirrors
FairWindSK's shipped native translation. Keep both listings synchronized when
features or user-facing claims change.

Before submission, replace every `TODO(PUBLISHER)` value. These values require
the account holder's legal, contact, review, and release decisions and cannot be
derived safely from the source tree.

The screenshots are publication artwork built from real FairWindSK captures.
They contain no device frame and no alpha channel. Regenerate them after a
material UI change with:

```bash
python3 docs/stores/generate_store_assets.py
```

Validate all text limits and raster dimensions with:

```bash
python3 docs/stores/validate_store_assets.py
```

Regenerate `asset-manifest.json` after changing any upload image:

```bash
python3 docs/stores/generate_asset_manifest.py
```

After replacing publisher-owned values, run the strict submission check:

```bash
python3 docs/stores/validate_store_assets.py --strict
```

Use [`submission-checklist.md`](./submission-checklist.md) for the final
account, build, policy, localization, screenshot, and review-server checks.

The generated Google Play feature-graphic background was created with the
built-in OpenAI image-generation tool, then combined locally with the exact
repository app icon and product name. The source prompt is documented in
[`playstore/README.md`](./playstore/README.md).
