# Google Play package

## Upload assets

- High-resolution icon: `graphics/app-icon-512.png` (512×512 RGB PNG).
- Feature graphic: `graphics/feature-graphic-1024x500.png`.
- Phone screenshots: `screenshots/<locale>/phone/` (1920×1080 PNG).
- Seven-inch or larger tablet screenshots: `screenshots/<locale>/tablet/`
  (2560×1600 PNG).
- Localized product text: `metadata/<locale>/`.

Each listing has five ordered screenshots. Google Play requires at least two
screenshots to publish; the supplied landscape images also match FairWindSK's
enforced marine-MFD orientation.

## Feature graphic source

The background was generated with the built-in OpenAI image-generation tool
using this production prompt:

> Create a premium abstract maritime technology backdrop with deep navy ocean
> tones, subtle cobalt-blue flowing wave contours, a restrained cyan horizon
> glow, and faint marine chart grid lines. Use a wide 2.048:1 composition,
> keeping the left third calm and dark for later logo placement. Background
> only: no UI, device, boat, logo, badge, text, letters, numbers, or watermark.

`generate_store_assets.py` composites that background with the exact shipping
FairWindSK icon and deterministic text. This avoids distorted generated branding.

## Console forms

`store-settings.yaml`, `data-safety.yaml`, `content-rating.yaml`, and
`app-access.yaml` are source-controlled worksheets for Play Console. Confirm
every answer against the release build and publisher account before submission.

