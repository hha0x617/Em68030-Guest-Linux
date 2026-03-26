#!/bin/bash
# Build Linux kernel and modules for Em68030 emulator using Docker.
#
# Usage:
#   ./build.sh
#
# Output:
#   output/vmlinux-*       - kernel image (load via File > Open ELF in the emulator)
#   output/em68030fb-*.ko  - framebuffer driver module
#   output/em68030input-*.ko - input driver module

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DOCKER_IMAGE="em68030-linux-builder"
OUTPUT_DIR="${SCRIPT_DIR}/output"

echo "=== Em68030 Linux Kernel Builder ==="

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
