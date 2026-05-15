#!/bin/bash
# Build Linux kernel and modules for Em68030 emulator using Docker.
#
# Usage:
#   ./build.sh [--kernel-version VER]
#
# If --kernel-version is omitted, KERNEL_VERSION env is used; otherwise it
# defaults to "6.12.17" (the version current CI artifacts are built against).
# Examples: 6.12.17, 7.0.6.  The major component (6 / 7) selects both the
# kernel.org URL path (vN.x/) and the patches/linux-<major>/ subdirectory.
#
# Output:
#   output/vmlinux-*       - kernel image (load via File > Open ELF in the emulator)
#   output/em68030fb-*.ko  - framebuffer driver module
#   output/em68030input-*.ko - input driver module

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DOCKER_IMAGE="em68030-linux-builder"
OUTPUT_DIR="${SCRIPT_DIR}/output"

KERNEL_VERSION="${KERNEL_VERSION:-6.12.17}"
while [ $# -gt 0 ]; do
    case "$1" in
        --kernel-version)
            KERNEL_VERSION="$2"
            shift 2
            ;;
        --kernel-version=*)
            KERNEL_VERSION="${1#--kernel-version=}"
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [--kernel-version VER]"
            echo ""
            echo "  --kernel-version VER   Linux version (default: 6.12.17)."
            echo "                         Examples: 6.12.17, 7.0.6."
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

echo "=== Em68030 Linux Kernel Builder ==="
echo "Kernel: ${KERNEL_VERSION}"
echo ""

# Build Docker image
echo "--- Building Docker image ---"
docker build -t "${DOCKER_IMAGE}" "${SCRIPT_DIR}/docker"

# Create output directory
mkdir -p "${OUTPUT_DIR}"

# Run build in Docker
echo "--- Starting kernel build ---"
docker run --rm \
    -v "${SCRIPT_DIR}/patches:/build/patches:ro" \
    -v "${SCRIPT_DIR}/drivers:/build/drivers:ro" \
    -v "${SCRIPT_DIR}/configs:/build/configs:ro" \
    -v "${SCRIPT_DIR}/docker:/build/docker:ro" \
    -v "${OUTPUT_DIR}:/build/output" \
    -e KERNEL_VERSION="${KERNEL_VERSION}" \
    "${DOCKER_IMAGE}" \
    bash /build/docker/build-inner.sh

echo ""
echo "Output files:"
ls -la "${OUTPUT_DIR}/"
echo ""
echo "Load kernel in emulator via: File > Open ELF"
echo "Rename .ko files before installing on guest:"
echo "  mv em68030fb-*.ko em68030fb.ko"
echo "  mv em68030input-*.ko em68030input.ko"
