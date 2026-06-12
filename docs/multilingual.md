# Multilingual support in FairWindSK

This document explains how FairWindSK currently handles multilingual support and how to add or improve a language without breaking the operator experience, the build, or the marine-MFD UI rules.

## Current model

FairWindSK currently ships with:

- `system`: follow the operating-system language when it is supported
- `en`: force English
- `it`: force Italian

Unsupported system languages and unsupported explicit selections currently fall back to English.

The runtime language path is split across a few layers:

- [Localization.hpp](../Localization.hpp) and [Localization.cpp](../Localization.cpp): normalize the selected language, choose the effective locale, and build the web `Accept-Language` header and translation resource path
- [main.cpp](../main.cpp): loads the Qt translator during startup and sets the default `QLocale`
- [Configuration.hpp](../Configuration.hpp) and [Configuration.cpp](../Configuration.cpp): persist the `main.language` setting
- [ui/settings/Main.cpp](../ui/settings/Main.cpp): exposes the language selector in the touch UI
- [CMakeLists.txt](../CMakeLists.txt): compiles `.ts` files into `.qm` resources with `qt_add_translations(...)`
- [docs/configuring.md](./configuring.md): documents the configuration values the user can set

## Rules to follow

- Every new user-visible Qt string must be translatable with `tr(...)` or the translation-aware pattern already used in the surrounding code.
- Native widgets and embedded web content must agree on the effective locale.
- Translations must preserve the marine electronics MFD style: clear wording, high readability, stable layout, and finger-friendly controls.
- English is the current fallback language unless the fallback contract is intentionally changed in code and docs together.
- A localization change is not complete until code, translations, build configuration, settings UI, and docs are all aligned.

## Improve an existing language

Use this path when the language already exists, for example improving Italian strings.

### 1. Change or add translatable source text

- Update the C++/Qt source to use `tr(...)` where needed.
- Keep wording short, operational, and consistent with marine display language.
- Avoid introducing multiple English variants for the same concept if one shared phrase already exists.

Typical places to check:

- settings pages
- top bar and bottom bar widgets
- drawer messages
- alarms, warnings, and retry/fallback text
- web-view bridge text and embedded HTML fallbacks

### 2. Update the translation source file

FairWindSK currently keeps the Italian translation in:

- [resources/i18n/fairwindsk_it.ts](../resources/i18n/fairwindsk_it.ts)

Update the `.ts` file with Qt Linguist or with the Qt translation tools available in your environment.

Typical workflow:

```bash
lupdate . -ts resources/i18n/fairwindsk_it.ts
linguist resources/i18n/fairwindsk_it.ts
```

Then rebuild so CMake regenerates the `.qm` resource through `qt_add_translations(...)`.

### 3. Rebuild and verify

Rebuild the project and verify:

- the application starts with the translated language
- no strings disappear because of a missing translation resource
- labels, tabs, buttons, and dialogs still fit in the available touch layout
- the text remains readable in `default`, `dawn`, `day`, `sunset`, `dusk`, and `night`
- embedded web views still expose the correct language and culture values

## Add a new language

Use this path when the language does not exist yet.

For the examples below, replace `fr` with the new language code you want to add.

### 1. Add the new translation source file

Create a new Qt translation source file named with the same pattern used today:

- `resources/i18n/fairwindsk_fr.ts`

The resource name should stay aligned with the runtime lookup in [Localization.cpp](../Localization.cpp), which expects:

- `:/resources/i18n/fairwindsk_<language>.qm`

### 2. Teach the build to compile it

Update [CMakeLists.txt](../CMakeLists.txt) in the `qt_add_translations(...)` block and add the new `.ts` file to `TS_FILES`.

Example:

```cmake
qt_add_translations(${PROJECT_NAME}
        TS_FILES
            resources/i18n/fairwindsk_it.ts
            resources/i18n/fairwindsk_fr.ts
        RESOURCE_PREFIX
            /resources/i18n
)
```

### 3. Teach localization code that the language is supported

Update [Localization.cpp](../Localization.cpp):

- `normalizeLanguageSelection(...)`
  Add the new supported language code.
- `effectiveLanguageCodeForSelection(...)`
  Add the new supported language code.
- `defaultLocaleForLanguage(...)`
  Return the correct default `QLocale` for the new language.
- `isSupportedSystemLanguage(...)`
  Add the language if FairWindSK should honor that OS language automatically when `system` is selected.

If you change the supported-language contract, review [Localization.hpp](../Localization.hpp) too in case comments or declarations need to stay aligned.

### 4. Expose the language in Settings

Update the language selector in [ui/settings/Main.cpp](../ui/settings/Main.cpp):

- add a new `comboBox_language->addItem(...)` entry
- make the label translatable if needed
- verify the selection persists and restart messaging still makes sense

Example pattern:

```cpp
ui->comboBox_language->addItem(tr("French"), "fr");
```

### 5. Update configuration documentation

Update [docs/configuring.md](./configuring.md):

- add the new language code to the documented `main.language` options
- update the fallback wording if needed
- mention restart requirements if they still apply

### 6. Translate the strings

Generate or update the `.ts` file with Qt translation tools, then complete the translations in Qt Linguist or your preferred Qt translation workflow.

Typical command:

```bash
lupdate . -ts resources/i18n/fairwindsk_fr.ts
```

Then translate the unfinished entries.

### 7. Rebuild and verify runtime loading

Rebuild the app and verify that:

- the `.qm` file is produced by the build
- the app loads it at startup
- `main.cpp` no longer falls back to English for the new language
- the settings page correctly persists `main.language`

## Embedded web locale behavior

FairWindSK does not only localize native Qt widgets. It also propagates locale information into embedded web content.

Relevant code:

- [FairWindSK.cpp](../FairWindSK.cpp)
- [ui/web/WebView.cpp](../ui/web/WebView.cpp)
- [Localization.cpp](../Localization.cpp)

When changing localization behavior, verify all of the following:

- `QLocale::setDefault(...)` is correct for the chosen language
- the generated web `Accept-Language` header is correct
- the exposed `language` and `culture` values inside the embedded web layer are correct
- HTML fallbacks and web-shell overlays use the same language as the native shell

## Testing checklist

At minimum, verify the following after adding or changing a language:

- Application starts successfully.
- Translator resource loads successfully.
- Unsupported selections still fall back predictably.
- `system` uses the OS language only when that language is explicitly supported.
- Settings > Main shows the right current language.
- Restart-required messaging is still translated.
- Top Bar, Bottom Bar, launcher, drawers, and settings tabs still fit on touch layouts.
- Alarm, safety, retry, and fallback text remains concise and unambiguous.
- Text remains readable in `default`, `dawn`, `day`, `sunset`, `dusk`, and `night`.
- Native shell and embedded web content use the same locale.
- macOS, Windows, Linux, Raspberry Pi OS, Android, and iOS impact is reviewed, or unsupported platforms are explicitly called out.

## Contributor notes

- Keep translation keys and English source text stable when possible, because unnecessary source churn creates avoidable translation churn.
- Prefer improving existing phrases over creating near-duplicate wording for the same operator action.
- If a translation requires a longer phrase than English, adjust layout carefully instead of shrinking text until readability suffers.
- When localization behavior changes, update this document and the corresponding guidance in [AGENTS.md](../AGENTS.md).
