# Security policy

## Supported versions

Only the latest release, and the `main` branch it is cut from, are supported. Fixes are not backported.

## What counts

Doombringer is firmware for a Eurorack module and a plugin loaded into VCV Rack. Please report anything that could cause harm beyond a bad sound, for example:

- a crash, hang or memory corruption in the VCV plugin that a patch file or input signal can trigger
- a release asset (`.bin` or `.vcvplugin`) that does not match the source it claims to be built from
- a GitHub Actions workflow in this repo that is open to injection or credential theft

Loud, harsh, chaotic and unstable sounds are the point of the module, not a vulnerability.

## Reporting a vulnerability

Do not open a public issue. Report it privately through GitHub's private vulnerability reporting:

**<https://github.com/rossedman/doombringer/security/advisories/new>**

Include the version or commit, the target (patch.Init firmware or VCV Rack), what an attacker could do, and steps or a patch file that reproduce it.

## What to expect

- An acknowledgement within 3 business days.
- A fix, or a plan with a timeline, within 30 days.
- Credit in the published advisory, unless you would rather stay anonymous.

## Verifying releases

Every release asset is signed keylessly with [cosign](https://docs.sigstore.dev/) by the release workflow, and carries a GitHub build provenance attestation. To check a download:

```sh
cosign verify-blob doombringer.bin \
  --bundle doombringer.bin.sigstore.json \
  --certificate-identity-regexp '^https://github.com/rossedman/doombringer/.github/workflows/release.yml@refs/tags/v' \
  --certificate-oidc-issuer https://token.actions.githubusercontent.com
sha256sum --ignore-missing -c SHA256SUMS
gh attestation verify doombringer.bin --repo rossedman/doombringer
```

Use the same commands for the `.vcvplugin` file and its `.sigstore.json` bundle.
