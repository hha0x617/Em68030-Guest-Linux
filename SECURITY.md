# Security Policy

## Supported Versions

Only the latest commit on `main` and the most recent tagged release
artifacts on the GitHub Releases page receive security updates.

| Version | Supported |
| ------- | --------- |
| Latest tagged release / `main` | ✅ |
| Older tagged releases          | ❌ |

## Reporting a Vulnerability

Please **do not** open a public GitHub issue for security
vulnerabilities.

Report privately via GitHub's **Private Vulnerability Reporting**:

1. Go to the **Security** tab of this repository.
2. Click **Report a vulnerability**.

Alternatively, email: <hha0x617@users.noreply.github.com>

Please include:

- Affected commit hash or release tag
- Steps to reproduce
- Impact assessment
- Proof-of-concept if available

## Response

- Initial response: within 7 days
- Fix timeline depends on severity and complexity
- A GitHub Security Advisory will be published after the fix is
  released
- Reporters will be credited unless they request anonymity

## In Scope

This repository contains:

- Custom kernel modules (`drivers/em68030fb`, `drivers/em68030input`)
  built against the upstream Linux kernel and licensed under
  GPL-2.0.
- Patches against upstream Linux kernel source.
- Build scripts (`build.sh`, `build.ps1`, `docker/`).
- Released binary artifacts (kernel image + module bundle) on the
  GitHub Releases page.

In-scope security issues include:

- Vulnerabilities introduced by this repo's kernel modules or
  patches that go beyond what upstream Linux exposes.
- Build scripts that leak credentials or fetch upstream source
  without integrity verification.
- Released binary artifacts that contain unintended secrets or
  malware.

## Out of Scope

- Vulnerabilities in the upstream Linux kernel itself — please
  report those to the kernel security team
  (<security@kernel.org>) and the relevant subsystem maintainers.
- Vulnerabilities in downstream consumers (the em6809 / em68030
  emulator family); file those against the relevant emulator
  repository.
- Issues requiring physical access to the host machine running the
  emulator.
- Theoretical exploits in MC68030 guest programs running inside
  this Linux build — this is an emulated guest, not a sandbox.
