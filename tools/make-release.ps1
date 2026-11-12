# Builds a release of calibly: release build, unit tests, the deployed folder
# (build\mingw-release\deploy) and the installer
# (build\mingw-release\installer\calibly-<version>-setup.exe).
# Run from the project folder:
#   powershell -ExecutionPolicy Bypass -File tools/make-release.ps1
# Options:
#   -SkipTests    leave out the unit tests
#   -QtRoot       Qt installation, default C:\Qt (as in CMakePresets.json)

param(
    [switch]$SkipTests,
    [string]$QtRoot = "C:\Qt"
)

$ErrorActionPreference = "Stop"
$project = Resolve-Path (Join-Path $PSScriptRoot "..")
$buildDir = Join-Path $project "build\mingw-release"
$deployDir = Join-Path $buildDir "deploy"

# The tools are not on PATH by default. windeployqt finds the MinGW runtime
# DLLs through PATH, so the compiler folder must be on it as well.
$cmake = Join-Path $QtRoot "Tools\CMake_64\bin\cmake.exe"
$ctest = Join-Path $QtRoot "Tools\CMake_64\bin\ctest.exe"
$env:PATH = (Join-Path $QtRoot "6.12.0\mingw_64\bin") + ";" + (Join-Path $QtRoot "Tools\mingw1510_64\bin") + ";" + $env:PATH
$iscc = @(
    (Join-Path $env:LOCALAPPDATA "Programs\Inno Setup 6\ISCC.exe"),
    (Join-Path ${env:ProgramFiles(x86)} "Inno Setup 6\ISCC.exe"),
    (Join-Path $env:ProgramFiles "Inno Setup 6\ISCC.exe")
) | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $iscc) { throw "Inno Setup 6 was not found. Install it, e.g. with: winget install JRSoftware.InnoSetup" }

# Runs a tool; only its exit code counts (tools also write warnings to
# stderr, which PowerShell would otherwise treat as errors).
function Step([string]$name, [scriptblock]$action) {
    Write-Host "`n=== $name ===" -ForegroundColor Cyan
    $ErrorActionPreference = "Continue"
    & $action
    $code = $LASTEXITCODE
    $ErrorActionPreference = "Stop"
    if ($code -ne 0) { throw "$name failed (exit code $code)" }
}

Push-Location $project
try {
    Step "Configure" { & $cmake --preset mingw-release }
    Step "Build" { & $cmake --build --preset mingw-release }
    if (-not $SkipTests) {
        Step "Tests" { & $ctest --preset mingw-release }
    }
    # A fresh folder, so nothing of an earlier release stays in it.
    if (Test-Path $deployDir) { Remove-Item -Recurse -Force $deployDir }
    Step "Deploy" { & $cmake --install $buildDir --prefix $deployDir }
    Step "Installer" { & $iscc "/DBuildDir=$buildDir" (Join-Path $project "installer\calibly.iss") }
    $setup = Get-ChildItem (Join-Path $buildDir "installer") -Filter "calibly-*-setup.exe" |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    Write-Host "`nDone: $($setup.FullName)" -ForegroundColor Green
}
finally {
    Pop-Location
}
