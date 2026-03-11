# Configure SubtitleForge build for Windows (run from this script's directory).
# Uses vcpkg toolchain if VCPKG_ROOT is set.
$ErrorActionPreference = "Stop"
$ProjectRoot = $PSScriptRoot
Set-Location $ProjectRoot

$BuildDir = "build"
$Preset = "windows"

if ($env:VCPKG_ROOT) {
    $Preset = "windows-vcpkg"
    Write-Host "Using vcpkg at: $env:VCPKG_ROOT"
} else {
    Write-Host "VCPKG_ROOT not set. Using preset: $Preset (set VCPKG_ROOT for vcpkg Qt/FFmpeg)."
}

Write-Host "Configuring with preset: $Preset"
cmake --preset $Preset
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "Configure done. Build with: cmake --build $BuildDir --config Release"
