# em68030-guest-linux

Guest-side Linux kernel modules, patches, and configurations for the
[Em68030](https://github.com/hha0x617/Em68030_WinUI3Cpp) MC68030/MVME147 emulator.

## License

This repository is licensed under GPL-2.0-only. See [LICENSE](LICENSE) for details.

The emulator itself is licensed under Apache License 2.0 in separate repositories
([Em68030_WinUI3Cpp](https://github.com/hha0x617/Em68030_WinUI3Cpp),
[Em68030_CsWPF](https://github.com/hha0x617/Em68030_CsWPF)).

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

Supports both absolute mouse mode (tablet-style, for framebuffer use) and
relative mouse mode. Keyboard events use Linux `KEY_*` codes directly.

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
