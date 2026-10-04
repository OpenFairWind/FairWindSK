# Apple App Store package

The package targets the currently implemented iOS and iPadOS builds.

## Upload assets

- App icon: `app-icon-1024.png` (1024×1024 RGB PNG, no alpha).
- iPhone screenshots: `screenshots/<locale>/iphone-6.9-landscape/`
  (2796×1290 PNG).
- iPad screenshots: `screenshots/<locale>/ipad-13-landscape/`
  (2752×2064 PNG).
- Localized product text: `metadata/<locale>/`.

The 6.9-inch iPhone and 13-inch iPad sizes are accepted by App Store Connect as
of 2026-07-23. Each set contains five ordered screenshots, within Apple's limit
of ten. App previews are optional and are intentionally omitted.

## App Store Connect fields

`app-information.yaml` contains non-localized classification and submission
fields. `review-information.yaml` contains the review checklist and publisher
placeholders. The localized files map directly to App Store Connect fields.

The app icon is copied from the shipping Apple asset catalog, not redrawn.

