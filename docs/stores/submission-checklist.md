# Store submission checklist

This checklist separates repository-prepared deliverables from decisions and
evidence that only the publisher account holder can supply.

## Shared release checks

- [ ] Build the exact release commit for iOS/iPadOS and Android.
- [ ] Confirm the displayed version and build numbers.
- [ ] Test first launch, location denial, location approval, offline startup,
      Signal K discovery, authentication, reconnect, and configuration import.
- [ ] Verify every claim in both localized descriptions against the release.
- [ ] Verify English and Italian text in the native shell and embedded-web locale
      propagation.
- [ ] Check Default, Dawn, Day, Sunset, Dusk, and Night comfort presets.
- [ ] Confirm all controls remain finger-friendly on representative phones and
      tablets.
- [ ] Confirm the app remains single-window on iOS/iPadOS.
- [ ] Confirm ordinary launcher behavior and optional Home behavior on Android
      API 33 and the current target API.
- [ ] Confirm the privacy policy and support page are published at stable HTTPS
      URLs and contain the publisher's legal contact details.
- [ ] Replace all `TODO(PUBLISHER)` values.
- [ ] Run `python3 docs/stores/validate_store_assets.py --strict`.

## Screenshot checks

- [ ] Capture the release build with production-quality sample data.
- [ ] Use only fictional or consented vessel names, coordinates, routes, files,
      accounts, and server addresses.
- [ ] Do not expose access tokens, usernames, passwords, private positions, or
      personal filesystem paths.
- [ ] Ensure the connection-status panel does not show transient errors.
- [ ] Regenerate store artwork and visually inspect all 40 localized screenshots.
- [ ] Confirm screenshot ordering tells the same feature story in both languages.

## Apple App Store Connect

- [ ] Create the app record using bundle ID `org.openfairwind.fairwindsk`.
- [ ] Complete SKU, copyright, availability, price, and release method.
- [ ] Upload and select the iOS/iPadOS build.
- [ ] Upload the English and Italian metadata and screenshots.
- [ ] Complete the age-rating questionnaire.
- [ ] Complete App Privacy from a fresh audit of the signed release.
- [ ] Complete accessibility declarations only for criteria tested on hardware.
- [ ] Complete export-compliance questions for the networking and cryptography
      present in the signed build.
- [ ] Provide a stable Signal K review server, test credentials, and review notes.
- [ ] Verify the Support URL exposes actual contact information.
- [ ] Submit manually and retain the review configuration until review completes.

## Google Play Console

- [ ] Create the app using package name `org.openfairwind.fairwindsk`.
- [ ] Upload the signed Android App Bundle to an internal test track first.
- [ ] Confirm target API, Android 13 minimum, signing, and Play App Signing.
- [ ] Upload English and Italian listing text and graphics.
- [ ] Complete App access using the stable Signal K review environment.
- [ ] Complete Ads, Content rating, Target audience, News, and Data safety forms.
- [ ] Reconcile `data-safety.yaml` with all Qt, WebView, and bundled SDK behavior.
- [ ] Declare location permissions and justify their user-facing core purpose.
- [ ] Confirm package visibility remains limited to `MAIN + LAUNCHER`.
- [ ] Verify installation does not select FairWindSK as Home automatically.
- [ ] Complete pricing and country/region availability.
- [ ] Run internal, closed, or production testing required by the publisher
      account before requesting production access.

