# Build SubtitleForge on Windows (run from this script's directory).
# Configure first with configure-win.ps1, or use a CMake preset.
$ErrorActionPreference = "Stop"
$ProjectRoot = $PSScriptRoot
Set-Location $ProjectRoot

$BuildDir = "build"
$Config = "Release"

# If preset was used, build dir might exist
if (-not (Test-Path $BuildDir)) {
    Write-Host "Build folder not found. Run configure-win.ps1 first."
    exit 1
}

cmake --build $BuildDir --config $Config
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "Done. Run: .\build\Release\SubtitleForge.exe"
