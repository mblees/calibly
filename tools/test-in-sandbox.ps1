# Opens Windows Sandbox (a clean Windows without Qt) with the installer of the
# last release build on its desktop, to test the installer and the app.
# Optional: a folder of test books (EPUB, PDF, M4B, MP3 folder) to try out.
# Run from the project folder after tools/make-release.ps1:
#   powershell -ExecutionPolicy Bypass -File tools/test-in-sandbox.ps1 [-Books <folder>]
#
# Windows Sandbox must be switched on once (as administrator, then restart):
#   Enable-WindowsOptionalFeature -Online -FeatureName Containers-DisposableClientVM

param([string]$Books)

$ErrorActionPreference = "Stop"
$project = Resolve-Path (Join-Path $PSScriptRoot "..")
$installerDir = Join-Path $project "build\mingw-release\installer"
if (-not (Get-ChildItem $installerDir -Filter "calibly-*-setup.exe" -ErrorAction SilentlyContinue)) {
    throw "No installer in $installerDir. Run tools/make-release.ps1 first."
}
if (-not (Test-Path (Join-Path $env:windir "System32\WindowsSandbox.exe"))) {
    throw "Windows Sandbox is not switched on, see the comment at the top of this script."
}

# Read-only folders on the desktop of the sandbox.
$folders = "    <MappedFolder><HostFolder>$installerDir</HostFolder><SandboxFolder>C:\Users\WDAGUtilityAccount\Desktop\calibly-Installer</SandboxFolder><ReadOnly>true</ReadOnly></MappedFolder>`n"
if ($Books) {
    $booksDir = Resolve-Path $Books
    $folders += "    <MappedFolder><HostFolder>$booksDir</HostFolder><SandboxFolder>C:\Users\WDAGUtilityAccount\Desktop\Testbuecher</SandboxFolder><ReadOnly>true</ReadOnly></MappedFolder>`n"
}
$config = @"
<Configuration>
  <VGpu>Disable</VGpu>
  <Networking>Enable</Networking>
  <MappedFolders>
$folders  </MappedFolders>
</Configuration>
"@
$wsb = Join-Path $env:TEMP "calibly-sandbox.wsb"
Set-Content -Path $wsb -Value $config -Encoding UTF8
Start-Process $wsb
