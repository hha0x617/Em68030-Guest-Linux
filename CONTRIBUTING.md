# Contributing to Em68030-Guest-Linux

Thanks for your interest!  This repository builds a Linux guest
image for the [em68030](https://github.com/hha0x617/Em68030_CsWPF)
emulator family — kernel modules (framebuffer + input drivers
targeting the emulated `em68030fb` / `em68030input` devices), a
small upstream patch, kernel `.config` snapshots, and Docker-based
build scripts.

## Build prerequisites

The repo's build is fully containerised; you only need:

- **Docker** (or Docker Desktop on Windows)
- **bash** (Linux/macOS) or **PowerShell** (Windows)

`build.sh` (Linux/macOS) and `build.ps1` (Windows) wrap a multi-stage
build that produces the m68k Linux kernel and modules.  See
[`README.md`](README.md) for the recipe and the
[Pre-built Binaries](README.md#pre-built-binaries) section for what
the GitHub Releases page provides.

## Coding style

* Linux kernel modules under `drivers/` follow upstream Linux kernel
  coding style (tab indent, K&R braces, see
  `Documentation/process/coding-style.rst` in the kernel tree).
* Build scripts (`*.sh`) use POSIX-friendly bash, `set -euo pipefail`.
* PowerShell scripts (`*.ps1`) use `$ErrorActionPreference = 'Stop'`
  and avoid pipeline-state mutation across functions.
* `.config` snapshots in `configs/` are committed verbatim from
  `make oldconfig` output — do not hand-edit them.

## Reporting bugs / requesting features

Use the issue templates in
[`.github/ISSUE_TEMPLATE/`](.github/ISSUE_TEMPLATE/).  Security
vulnerabilities go through [`SECURITY.md`](SECURITY.md) instead.
For upstream Linux kernel issues unrelated to this repo's drivers
or patches, file them with the kernel project — not here.

## Code of Conduct

This project follows the [Contributor Covenant 2.1](CODE_OF_CONDUCT.md).
By participating you are expected to uphold those standards.
Reports of unacceptable behaviour go to the contact address listed
in the Code of Conduct.

## License

By submitting a contribution you agree it will be licensed under
the same **GPL-2.0-only** terms as the rest of the repository.
The kernel modules and patches in this repository are derivative
works of the GPL-2.0-licensed Linux kernel, so GPL-2.0 is required
rather than optional.
