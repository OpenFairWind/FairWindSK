# Releasing FairWindSK

FairWindSK follows Semantic Versioning 2.0.0. The canonical version is stored in
`VERSION.txt`; prerelease identifiers such as `-alpha`, `-beta.1`, and `-rc.1` are
supported. CMake uses the numeric core for its project version and the complete
string for application and package metadata.

## Release procedure

1. Update `VERSION.txt` and move the relevant entries from **Unreleased** into a
   dated section in `CHANGELOG.md`.
2. Configure the Android and iOS signing secrets listed in [building.md](building.md).
3. Commit the release preparation.
4. Create and push a tag named `v` followed by the exact `VERSION.txt` value, for
   example `v0.1.0`.
5. Approve the protected `release` environment if repository policy requires it.

The release workflow validates the tag and signing configuration, then builds
macOS arm64 and x86-64 DMGs, a Windows x86-64 installer, Debian and Red Hat
x86-64 packages, a Raspberry Pi OS arm64 package, an Android arm64 APK, and an
iOS arm64 IPA. Publication is atomic: a missing or invalid platform package
prevents the GitHub Release from being created. The release also contains
`SHA256SUMS`, `release-manifest.json`, its Sigstore signature, and the signing
certificate.

## Published installers

Every binary release publishes the following versioned artifacts from the same
tag and commit:

| Build flavor | Release artifact | Dependency handling |
| --- | --- | --- |
| macOS Apple Silicon | `FairWindSK-<version>-macos-arm64.dmg` | Qt frameworks, plug-ins, and helper executables are embedded in the application bundle. |
| macOS Intel | `FairWindSK-<version>-macos-x86_64.dmg` | Qt frameworks, plug-ins, and helper executables are embedded in the application bundle. |
| Windows x86-64 | `FairWindSK-<version>-windows-x86_64-setup.exe` | The NSIS installer contains the MSVC Qt runtime, plug-ins, and WebEngine payload. |
| Debian/Ubuntu x86-64 | `FairWindSK-<version>-linux-debian-x86_64.deb` | The package carries deployable Qt files and declares remaining system-library dependencies for APT. |
| Red Hat x86-64 | `FairWindSK-<version>-linux-redhat-x86_64.rpm` | The package carries deployable Qt files and uses RPM automatic dependency generation for remaining system libraries. |
| HaLOS Desktop / Debian Trixie ARM64 | `fairwindsk_<version>-1~halos13_arm64.deb` | The native package declares its Qt and system dependencies and is installed with APT. |
| Raspberry Pi OS ARM64 | `FairWindSK-<version>-raspberry-pi-os-arm64.deb` | The native package derives its shared-library requirements for installation with APT. |
| Android ARM64 | `FairWindSK-<version>-android-arm64-v8a.apk` | The signed APK contains the required Qt libraries and Android plug-ins. |
| iOS ARM64 | `FairWindSK-<version>-ios-arm64.ipa` | The signed IPA embeds its frameworks and is exported with the configured provisioning profile. |

Install the HaLOS, Raspberry Pi OS, and Debian packages with
`sudo apt install ./<package>.deb`, rather than `dpkg -i`, so their declared
dependencies are resolved in the same operation. Use the platform-native
installer for every other flavor. Do not copy a build-tree executable between
machines: it is not a substitute for the dependency-aware release artifact.

## Verifying a download

Verify the package hash with `sha256sum -c SHA256SUMS`. Then install Cosign and
verify that the manifest was signed by this repository's GitHub Actions workflow:

```sh
cosign verify-blob \
  --certificate release-manifest.pem \
  --signature release-manifest.sig \
  --certificate-identity-regexp 'https://github.com/OpenFairWind/FairWindSK/.github/workflows/release.yml@refs/tags/v.*' \
  --certificate-oidc-issuer 'https://token.actions.githubusercontent.com' \
  release-manifest.json
```

After signature verification, compare the manifest's commit and tag with the
release page and use its SHA-256 value to verify the selected package.
