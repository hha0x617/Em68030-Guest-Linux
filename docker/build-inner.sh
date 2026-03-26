#!/bin/bash
set -euo pipefail

: "${KERNEL_VERSION:=6.12.17}"
: "${CONFIG_NAME:=fbconsole+evdev}"
: "${CONFIG_FILE:=configs/.config.20260314_FB_CONSOLE_with_INPUT_EVDEV}"

echo "--- Downloading kernel source ---"
cd /build
curl -sL "https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-${KERNEL_VERSION}.tar.xz" | tar xJ
cd "linux-${KERNEL_VERSION}"

echo "--- Applying patches ---"
for patch in /build/patches/*.patch; do
    echo "Applying $(basename "$patch")..."
    patch -p1 -N < "$patch" || true
done

echo "--- Configuring kernel ---"
cp "/build/${CONFIG_FILE}" .config
make ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- olddefconfig

echo "--- Building kernel ---"
make ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- vmlinux -j$(nproc)
make ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- modules_prepare
cp vmlinux.symvers Module.symvers

echo "--- Building kernel modules ---"
cp -r /build/drivers /tmp/drivers
make ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- \
    -C "/build/linux-${KERNEL_VERSION}" M=/tmp/drivers/em68030fb modules
make ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- \
    -C "/build/linux-${KERNEL_VERSION}" M=/tmp/drivers/em68030input modules

echo "--- Copying output ---"
SHORT_HASH=$(cd /build/patches && md5sum *.patch "/build/${CONFIG_FILE}" | md5sum | cut -c1-7)
SUFFIX="${KERNEL_VERSION}-mvme147-uart16550-${CONFIG_NAME}-${SHORT_HASH}"
cp vmlinux "/build/output/vmlinux-${SUFFIX}"
cp /tmp/drivers/em68030fb/em68030fb.ko "/build/output/em68030fb-${SHORT_HASH}.ko"
cp /tmp/drivers/em68030input/em68030input.ko "/build/output/em68030input-${SHORT_HASH}.ko"
ls -la /build/output/
echo "=== Build successful ==="
