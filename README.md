# em68030-guest-linux

Guest-side Linux kernel modules, patches, and configurations for the
[Em68030](https://github.com/hha0x617/em68030) MC68030/MVME147 emulator.

## License

This repository is licensed under GPL-2.0-only. See [LICENSE](LICENSE) for details.

The emulator itself (Em68030) is licensed under Apache License 2.0 in separate repositories.

## Contents

### drivers/em68030fb/

Loadable kernel module that registers a `simple-framebuffer` platform device
by reading framebuffer parameters from the EMFB control registers at `$FFFE8000`.

Works with any resolution/BPP configured in the emulator settings — no
hardcoded values.

**Prerequisites:**
- Kernel built with `CONFIG_FB_SIMPLE=y` (or `=m`)
- Verify: `zcat /proc/config.gz | grep FB_SIMPLE`

**Install build dependencies (on the guest):**
```bash
apt install build-essential bc flex bison libelf-dev
```

**Set up kernel headers:**

The module Makefile expects the kernel build tree at `/lib/modules/$(uname -r)/build`.
Since the guest runs a custom-built kernel, you must prepare and install the headers
from the same kernel source used to build the running kernel.

On the **host** (cross-build environment), after building the kernel:
```bash
# In the kernel source directory
make ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- modules_prepare

# Create a tarball of the kernel build tree
KVER=$(make -s kernelrelease)
tar cf /tmp/kheaders.tar \
  --transform "s,^,/usr/src/linux-headers-${KVER}/," \
  .config Module.symvers Makefile include/ scripts/ arch/m68k/include/ arch/m68k/Makefile
```

Copy the tarball to the guest disk image and extract it on the guest:
```bash
tar xf /path/to/kheaders.tar -C /
ln -s /usr/src/linux-headers-$(uname -r) /lib/modules/$(uname -r)/build
```

**Build and load:**
```bash
cd drivers/em68030fb
make
insmod em68030fb.ko
```

**Verify:**
```bash
ls -la /dev/fb0
cat /dev/urandom > /dev/fb0   # noise should appear in emulator's framebuffer window
```

**Install for auto-load:**
```bash
make install
echo em68030fb >> /etc/modules
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
