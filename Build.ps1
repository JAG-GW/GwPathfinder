# ===================================================================
# GWPathfinder - Script de compilation complet
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

# Étape 1: Vérifier maps.zip
Write-Host "[1/4] Verification de maps.zip..." -ForegroundColor Yellow
if (-not (Test-Path "maps.zip")) {
    if (Test-Path "maps") {
        Write-Host "  Creation depuis le dossier maps/..." -ForegroundColor Gray
        Compress-Archive -Path "maps\*" -DestinationPath "maps.zip" -Force
        Write-Host "  Cree avec succes" -ForegroundColor Green
    } else {
        Write-Host "  ERREUR: maps/ et maps.zip manquants!" -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "  OK" -ForegroundColor Green
}

# Étape 2: Vérifier et installer les dépendances vcpkg
Write-Host ""
Write-Host "[2/4] Dependencies vcpkg..." -ForegroundColor Yellow

if (-not (Test-Path $VcpkgExe)) {
    Write-Host "  ERREUR: vcpkg introuvable dans $VcpkgRoot" -ForegroundColor Red
    Write-Host "  Installez: https://github.com/microsoft/vcpkg" -ForegroundColor Gray
    exit 1
}

Write-Host "  Installation des packages (x86-windows-static)..." -ForegroundColor Gray
$env:VCPKG_DEFAULT_TRIPLET = "x86-windows-static"
Push-Location $PSScriptRoot
& $VcpkgExe install --triplet x86-windows-static
Pop-Location

if ($LASTEXITCODE -ne 0) {
    Write-Host "  ERREUR lors de l'installation vcpkg" -ForegroundColor Red
    exit 1
}
Write-Host "  OK" -ForegroundColor Green

# Étape 3: Configuration CMake
Write-Host ""
Write-Host "[3/4] Configuration CMake..." -ForegroundColor Yellow

if ($Clean -and (Test-Path "build")) {
    Write-Host "  Nettoyage..." -ForegroundColor Gray
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
    Write-Host "  ERREUR Configuration CMake" -ForegroundColor Red
    Pop-Location
    exit 1
}

Write-Host "  OK" -ForegroundColor Green

# Étape 4: Compilation
Write-Host ""
Write-Host "[4/4] Compilation..." -ForegroundColor Yellow

cmake --build . --config $BuildType --parallel | Out-Host

if ($LASTEXITCODE -ne 0) {
    Write-Host "  ERREUR Compilation" -ForegroundColor Red
    Pop-Location
    exit 1
}

Pop-Location

Write-Host "  OK" -ForegroundColor Green

# Résultat final
Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  COMPILATION REUSSIE!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""

$DllPath = "build\$BuildType\GWPathfinder.dll"
if (Test-Path $DllPath) {
    # Copier maps.zip
    Copy-Item "maps.zip" "build\$BuildType\maps.zip" -Force

    # Afficher les infos
    $DllSize = (Get-Item $DllPath).Length / 1MB
    $ZipSize = (Get-Item "build\$BuildType\maps.zip").Length / 1MB

    Write-Host "Fichiers crees:" -ForegroundColor Cyan
    Write-Host "  DLL: $DllPath ($([math]::Round($DllSize, 1)) MB)" -ForegroundColor Gray
    Write-Host "  ZIP: build\$BuildType\maps.zip ($([math]::Round($ZipSize, 1)) MB)" -ForegroundColor Gray

    Write-Host ""
    Write-Host "Pour tester:" -ForegroundColor Yellow
    Write-Host "  cd build\$BuildType" -ForegroundColor Gray
    Write-Host "  AutoIt3.exe ..\..\TestAutoIt.au3" -ForegroundColor Gray
} else {
    Write-Host "ERREUR: DLL non creee!" -ForegroundColor Red
    exit 1
}

Write-Host ""
