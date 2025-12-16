# ===================================================================
# GWPathfinder - Complete Build Script
# Usage: .\Build.ps1
# ===================================================================

param(
    [switch]$Clean,
    [switch]$Debug
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  GWPathfinder Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Configuration
$BuildType = if ($Debug) { "Debug" } else { "Release" }
$VcpkgRoot = "C:\vcpkg"
$VcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
$VcpkgToolchain = Join-Path $VcpkgRoot "scripts\buildsystems\vcpkg.cmake"

Write-Host "Configuration: $BuildType" -ForegroundColor Gray
Write-Host ""

# Step 1: Check maps.zip
Write-Host "[1/4] Checking maps.zip..." -ForegroundColor Yellow
if (-not (Test-Path "maps.zip")) {
    if (Test-Path "maps") {
        Write-Host "  Creating from maps/ folder..." -ForegroundColor Gray
        Compress-Archive -Path "maps\*" -DestinationPath "maps.zip" -Force
        Write-Host "  Created successfully" -ForegroundColor Green
    } else {
        Write-Host "  ERROR: maps/ and maps.zip are missing!" -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "  OK" -ForegroundColor Green
}

# Step 2: Check and install vcpkg dependencies
Write-Host ""
Write-Host "[2/4] vcpkg dependencies..." -ForegroundColor Yellow

if (-not (Test-Path $VcpkgExe)) {
    Write-Host "  ERROR: vcpkg not found in $VcpkgRoot" -ForegroundColor Red
    Write-Host "  Install from: https://github.com/microsoft/vcpkg" -ForegroundColor Gray
    exit 1
}

Write-Host "  Installing packages (x86-windows-static)..." -ForegroundColor Gray
$env:VCPKG_DEFAULT_TRIPLET = "x86-windows-static"
Push-Location $PSScriptRoot
& $VcpkgExe install --triplet x86-windows-static
Pop-Location

if ($LASTEXITCODE -ne 0) {
    Write-Host "  ERROR: vcpkg installation failed" -ForegroundColor Red
    exit 1
}
Write-Host "  OK" -ForegroundColor Green

# Step 3: CMake Configuration
Write-Host ""
Write-Host "[3/4] CMake configuration..." -ForegroundColor Yellow

if ($Clean -and (Test-Path "build")) {
    Write-Host "  Cleaning..." -ForegroundColor Gray
    Remove-Item "build" -Recurse -Force
}

if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

Push-Location "build"

cmake .. `
    -G "Visual Studio 17 2022" `
    -A Win32 `
    -DCMAKE_BUILD_TYPE=$BuildType `
    -DVCPKG_TARGET_TRIPLET=x86-windows-static `
    -DCMAKE_TOOLCHAIN_FILE="$VcpkgToolchain" | Out-Host

if ($LASTEXITCODE -ne 0) {
    Write-Host "  ERROR: CMake configuration failed" -ForegroundColor Red
    Pop-Location
    exit 1
}

Write-Host "  OK" -ForegroundColor Green

# Step 4: Build
Write-Host ""
Write-Host "[4/4] Building..." -ForegroundColor Yellow

cmake --build . --config $BuildType --parallel | Out-Host

if ($LASTEXITCODE -ne 0) {
    Write-Host "  ERROR: Build failed" -ForegroundColor Red
    Pop-Location
    exit 1
}

Pop-Location

Write-Host "  OK" -ForegroundColor Green

# Final result
Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  BUILD SUCCESSFUL!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""

$DllPath = "build\$BuildType\GWPathfinder.dll"
if (Test-Path $DllPath) {
    # Copy maps.zip
    Copy-Item "maps.zip" "build\$BuildType\maps.zip" -Force

    # Display info
    $DllSize = (Get-Item $DllPath).Length / 1MB
    $ZipSize = (Get-Item "build\$BuildType\maps.zip").Length / 1MB

    Write-Host "Created files:" -ForegroundColor Cyan
    Write-Host "  DLL: $DllPath ($([math]::Round($DllSize, 1)) MB)" -ForegroundColor Gray
    Write-Host "  ZIP: build\$BuildType\maps.zip ($([math]::Round($ZipSize, 1)) MB)" -ForegroundColor Gray

    Write-Host ""
    Write-Host "To test:" -ForegroundColor Yellow
    Write-Host "  cd build\$BuildType" -ForegroundColor Gray
    Write-Host "  AutoIt3.exe ..\..\TestAutoIt.au3" -ForegroundColor Gray
} else {
    Write-Host "ERROR: DLL not created!" -ForegroundColor Red
    exit 1
}

Write-Host ""
