# em68030-guest-linux

[Japanese documentation (README_ja.md)](README_ja.md)

Guest-side Linux kernel modules, patches, and configurations for the
Em68030 MC68030/MVME147 emulator
([Em68030_WinUI3Cpp](https://github.com/hha0x617/Em68030_WinUI3Cpp),
[Em68030_CsWPF](https://github.com/hha0x617/Em68030_CsWPF)).

*The repository-specific assets — the `em68030fb` / `em68030input`
drivers under `drivers/`, the kernel patches under `patches/`, the
kernel configs under `configs/`, the build glue under `docker/` and
`build.sh` / `build.ps1`, and the GitHub Actions workflows — are
authored through vibe coding with
[Claude Code](https://docs.anthropic.com/en/docs/claude-code).
The upstream Linux kernel source itself is **not** AI-generated; it
is fetched verbatim from kernel.org at build time.*

## License

This repository is licensed under GPL-2.0-only. See [LICENSE](LICENSE) for details.

The emulator itself is licensed under Apache License 2.0 in separate repositories
([Em68030_WinUI3Cpp](https://github.com/hha0x617/Em68030_WinUI3Cpp),
[Em68030_CsWPF](https://github.com/hha0x617/Em68030_CsWPF)).

## Pre-built Binaries

Pre-built kernel image and modules are available on the
[Releases](https://github.com/hha0x617/Em68030-Guest-Linux/releases) page.

Release assets use suffixed filenames to identify the build (e.g.,
`em68030fb-abc1234.ko`, `em68030input-abc1234.ko`).
**Rename the `.ko` files to their original names before installing:**

```bash
mv em68030fb-*.ko em68030fb.ko
mv em68030input-*.ko em68030input.ko
```

The `vmlinux` image can be loaded directly by the emulator (File → Open ELF).

## Building with Docker

Requires Docker only — no cross-compiler installation needed.

**Linux / macOS / WSL:**
```bash
./build.sh
```

**Windows (PowerShell):**
```powershell
.\build.ps1
```

The kernel source is downloaded automatically from kernel.org.
Output files are placed in `output/`.

## Documentation

| Guide | Description |
|-------|-------------|
| [Framebuffer Display Setup](docs/setup_framebuffer.md) | fbcon and X Window System setup on the Linux guest |
| [フレームバッファディスプレイ セットアップ](docs/setup_framebuffer_ja.md) | 日本語版 |

## Contents

### drivers/em68030fb/

Loadable kernel module that registers a `simple-framebuffer` platform device
by reading framebuffer parameters from the EMFB control registers at `$FFFE8000`.

Works with any resolution/BPP configured in the emulator settings — no
hardcoded values.

**Prerequisites:**
- Kernel built with `CONFIG_FB_SIMPLE=y` (or `=m`)
- Kernel built with `CONFIG_TRIM_UNUSED_KSYMS` disabled — this option strips
  unexported symbols (`ioremap`, `platform_device_register`, etc.) from the
  running kernel, causing `insmod` to fail with "Unknown symbol in module"
- Verify: `zcat /proc/config.gz | grep FB_SIMPLE`

**Cross-compile on the host:**

Building kernel modules natively on the emulated m68k guest is impractical because
the kernel build system requires host-architecture tools (in `scripts/`) that cannot
run on the guest. Cross-compile the module on the host instead.

On the **host** (cross-build environment), with the kernel source tree used to build
the running kernel. The kernel must have been fully built (`vmlinux`) first.
```bash
# Build the kernel first (skip if you already have vmlinux)
cd /path/to/linux-6.12.17
make ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- vmlinux -j$(nproc)

# Kernel 6.12+ writes exported symbols to vmlinux.symvers, but out-of-tree
# module builds read Module.symvers. Copy if Module.symvers is stale/empty.
cp vmlinux.symvers Module.symvers

# Cross-compile the module
cd /path/to/em68030-guest-linux/drivers/em68030fb
make ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- -C /path/to/linux-6.12.17 M=$(pwd) modules
```

Copy the resulting `em68030fb.ko` to the guest disk image.

**Load on the guest:**
```bash
insmod /path/to/em68030fb.ko
```

**Verify:**
```bash
ls -la /dev/fb0
cat /dev/urandom > /dev/fb0   # noise should appear in emulator's framebuffer window
```

**Install for auto-load:**

On the host, cross-compile the install target:
```bash
make ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- -C /path/to/linux-6.12.17 M=$(pwd) \
  INSTALL_MOD_PATH=/path/to/guest-rootfs modules_install
```

Or manually on the guest:
```bash
mkdir -p /lib/modules/$(uname -r)/extra
cp em68030fb.ko /lib/modules/$(uname -r)/extra/

# Create metadata files if missing (not installed by "make vmlinux" alone)
cd /lib/modules/$(uname -r)
touch modules.order modules.builtin modules.builtin.modinfo

depmod -a

# systemd (Debian, Gentoo with systemd):
echo em68030fb > /etc/modules-load.d/em68030fb.conf

# OpenRC (Gentoo with OpenRC):
# Add modules="em68030fb" to /etc/conf.d/modules
```

### drivers/em68030input/

Loadable kernel module that provides virtual keyboard and mouse input devices
by reading events from the EMKM control registers at `$FFFE9000`.

The emulator host captures keyboard and mouse events in the framebuffer window
and pushes them into an MMIO event FIFO. This driver polls the FIFO at 100 Hz
and reports events via the Linux input subsystem (`evdev`).

Registers three input devices:
- **Keyboard** (`/dev/input/event0`): Linux `KEY_*` codes
- **Tablet** (`/dev/input/event1`): Absolute coordinates for X Window System / libinput
- **Mouse** (`/dev/input/event2`): Relative coordinates computed from absolute position deltas, for gpm (fbcon text selection)

**Prerequisites:**
- Kernel built with `CONFIG_INPUT=y` and `CONFIG_INPUT_EVDEV=y` (or `=m`)
- Kernel built with `CONFIG_TRIM_UNUSED_KSYMS` disabled
- Framebuffer enabled in emulator settings (EMKM device is only present when
  framebuffer is active)

**Cross-compile on the host:**
```bash
cd /path/to/em68030-guest-linux/drivers/em68030input
make ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- -C /path/to/linux-6.12.17 M=$(pwd) modules
```

Copy the resulting `em68030input.ko` to the guest disk image.

**Load on the guest:**
```bash
insmod /path/to/em68030input.ko
```

**Verify:**
```bash
# Should show "Em68030 Virtual Keyboard" and "Em68030 Virtual Mouse"
cat /proc/bus/input/devices
```

**Install for auto-load:**
```bash
mkdir -p /lib/modules/$(uname -r)/extra
cp em68030input.ko /lib/modules/$(uname -r)/extra/
depmod -a

# systemd (Debian, Gentoo with systemd):
echo em68030input > /etc/modules-load.d/em68030input.conf

# OpenRC (Gentoo with OpenRC):
# Add modules="em68030fb em68030input" to /etc/conf.d/modules
```

### patches/

Kernel source patches for Em68030-specific MVME147 support.

#### `mvme147-uart-16550.patch`

Registers a virtual 16550A UART platform device at `$FFFE2000` in the MVME147 board
configuration (`arch/m68k/mvme147/config.c`). The real MVME147 uses a Z8530 SCC for
serial I/O, but the upstream Linux kernel lacks a Z8530-based tty driver for MVME147.
This patch enables the well-supported `8250/16550` serial driver to provide the
userspace console (`/dev/ttyS0`).

Without this patch, the kernel boots and `earlyprintk` works, but userspace has no
console device (`Warning: unable to open an initial console`).

**Apply:**
```bash
cd /path/to/linux-6.12.17
patch -p1 < /path/to/em68030-guest-linux/patches/mvme147-uart-16550.patch
```

**Required kernel config:**
- `CONFIG_SERIAL_8250=y`
- `CONFIG_SERIAL_8250_CONSOLE=y`

See also the getting started guides (`getting_started_debian.md` / `getting_started_gentoo.md`)
in the emulator repository for the full kernel build procedure.

### configs/

Saved `.config` files for the Linux 6.12.17 kernel, based on `mvme16x_defconfig`
with Em68030-specific options enabled. These can be copied to the kernel source
tree as `.config` instead of running `menuconfig` manually.

| File | Description |
|------|-------------|
| `.config.20260308_BASIC_CONFIG` | Minimal config: UART + network (`CONFIG_MVME147_NET=y`), no framebuffer |
| `.config.20260312_FB_SIMPLE` | Full config: UART + framebuffer + network |
| `.config.20260314_FB_CONSOLE_with_INPUT_EVDEV` | Full config + fbcon + evdev input (for keyboard/mouse) |

**Key options enabled (common to all configs):**
- `CONFIG_M68030=y`, `CONFIG_MVME147=y` — CPU and board support
- `CONFIG_SERIAL_8250=y`, `CONFIG_SERIAL_8250_CONSOLE=y` — virtual UART console
- `CONFIG_MVME147_SCSI=y`, `CONFIG_BLK_DEV_SD=y` — SCSI disk support
- `CONFIG_EXT4_FS=y` — root filesystem

**Usage:**
```bash
cp /path/to/em68030-guest-linux/configs/.config.20260312_FB_SIMPLE \
   /path/to/linux-6.12.17/.config
make ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- olddefconfig
make ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- vmlinux -j$(nproc)
```

## Acknowledgments

This project depends on the continued effort of the Linux kernel community and
downstream distribution communities to keep Linux/m68k functional on modern
systems. We would like to express our sincere thanks to:

- **The Linux kernel community** — and especially the m68k subsystem maintainers
  and contributors on the `linux-m68k` mailing list — for keeping the m68k
  architecture functional in modern Linux kernels. The out-of-tree modules and
  in-tree patches in this repository plug directly into subsystems (fbdev, evdev,
  serial, SCSI) maintained by the broader kernel community. See
  http://www.linux-m68k.org/

- **The Debian Project and the Debian Ports team** — for maintaining the
  Debian/m68k unofficial port, which consumes the kernel modules and patches from
  this repository. The broader Debian community's policy of welcoming
  non-release architectures is what gives this work a practical use case. See
  https://www.ports.debian.org/

- **The Gentoo Project and the Gentoo m68k team** — for publishing regular stage3
  tarballs and keeping the portage tree working for m68k, providing another
  downstream target for the drivers developed here. See
  https://wiki.gentoo.org/wiki/M68k

Their continuous effort is what makes this project possible.

## Related Projects

- [Em68030_WinUI3Cpp](https://github.com/hha0x617/Em68030_WinUI3Cpp) — Emulator (C++/WinUI3)
- [Em68030_CsWPF](https://github.com/hha0x617/Em68030_CsWPF) — Emulator (C#/WPF)
- [Em68030-Guest-NetBSD](https://github.com/hha0x617/Em68030-Guest-NetBSD) — NetBSD guest components

## Contributing and Policies

- Contribution workflow: [`CONTRIBUTING.md`](CONTRIBUTING.md)
- Code of Conduct: [`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md) (Contributor Covenant 2.1)
- Security: [`SECURITY.md`](SECURITY.md)
