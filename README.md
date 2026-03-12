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

Kernel patches for MVME147 support enhancements (e.g., 8250/16550 UART).

### configs/

Custom kernel defconfig for MVME147 with framebuffer support enabled.
