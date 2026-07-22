# Releasing FairWindSK

FairWindSK follows Semantic Versioning 2.0.0. The canonical version is stored in
`VERSION.txt`; prerelease identifiers such as `-alpha`, `-beta.1`, and `-rc.1` are
supported. CMake uses the numeric core for its project version and the complete
string for application and package metadata.

## Release procedure

1. Update `VERSION.txt` and move the relevant entries from **Unreleased** into a
   dated section in `CHANGELOG.md`.
2. Commit the release preparation.
3. Create and push a tag named `v` followed by the exact `VERSION.txt` value, for
   example `v0.1.0-alpha`.
4. Approve the protected `release` environment if repository policy requires it.

The release workflow validates the tag, builds packages for Raspberry Pi OS
64-bit, Linux x86-64, Windows, and macOS, and publishes them to a GitHub Release.
It also publishes `SHA256SUMS`, `release-manifest.json`, the Sigstore signature,
and the signing certificate.

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
