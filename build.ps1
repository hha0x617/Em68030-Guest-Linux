# Build Linux kernel and modules for Em68030 emulator using Docker.
#
# Usage:
#   .\build.ps1
#
# Output:
#   output\vmlinux-*         - kernel image (load via File > Open ELF in the emulator)
#   output\em68030fb-*.ko    - framebuffer driver module
#   output\em68030input-*.ko - input driver module

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$DockerImage = "em68030-linux-builder"
$OutputDir = Join-Path $ScriptDir "output"

Write-Host "=== Em68030 Linux Kernel Builder ==="

# Build Docker image
Write-Host "--- Building Docker image ---"
docker build -t $DockerImage (Join-Path $ScriptDir "docker")

# Create output directory
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$PatchesDir = Join-Path $ScriptDir "patches"
$DriversDir = Join-Path $ScriptDir "drivers"
$ConfigsDir = Join-Path $ScriptDir "configs"
$DockerDir = Join-Path $ScriptDir "docker"

Write-Host "--- Starting kernel build ---"
docker run --rm `
    -v "${PatchesDir}:/build/patches:ro" `
    -v "${DriversDir}:/build/drivers:ro" `
    -v "${ConfigsDir}:/build/configs:ro" `
    -v "${DockerDir}:/build/docker:ro" `
    -v "${OutputDir}:/build/output" `
    $DockerImage `
    bash /build/docker/build-inner.sh

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed"
    exit 1
}

Write-Host ""
Write-Host "Output files:"
Get-ChildItem $OutputDir
Write-Host ""
Write-Host "Load kernel in emulator via: File > Open ELF"
Write-Host "Rename .ko files before installing on guest:"
Write-Host "  mv em68030fb-*.ko em68030fb.ko"
Write-Host "  mv em68030input-*.ko em68030input.ko"
