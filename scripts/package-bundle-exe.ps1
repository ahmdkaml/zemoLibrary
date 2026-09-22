$ErrorActionPreference = "Stop"

$RootDir = Resolve-Path "$PSScriptRoot\.."
$LauncherSrc = "$RootDir\scripts\launcher.c"
$LauncherBase = "$RootDir\dist\launcher_base.exe"
$BundleTar = "$RootDir\dist\bundle.tar.gz"
$OutDir = "$RootDir\dist\ZemoServer"
$OutExe = "$OutDir\ZemoServer.exe"

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

Write-Host "==> Compiling launcher C code..."
& gcc -O2 $LauncherSrc -o $LauncherBase
if ($LASTEXITCODE -ne 0) { throw "GCC compilation failed" }

Write-Host "==> Reading components..."
$baseBytes = [System.IO.File]::ReadAllBytes($LauncherBase)
$bundleBytes = [System.IO.File]::ReadAllBytes($BundleTar)
$payloadLen = [UInt64]$bundleBytes.Length

Write-Host "    Launcher size: $($baseBytes.Length) bytes"
Write-Host "    Bundle size:   $($bundleBytes.Length) bytes"

$lenBytes = [System.BitConverter]::GetBytes($payloadLen)
if (![System.BitConverter]::IsLittleEndian) {
    [System.Array]::Reverse($lenBytes)
}

$magicBytes = [System.Text.Encoding]::ASCII.GetBytes("ZEMOBNDL")

Write-Host "==> Creating self-contained binary: $OutExe..."
$fs = [System.IO.File]::Create($OutExe)
try {
    $fs.Write($baseBytes, 0, $baseBytes.Length)
    $fs.Write($bundleBytes, 0, $bundleBytes.Length)
    $fs.Write($lenBytes, 0, $lenBytes.Length)
    $fs.Write($magicBytes, 0, $magicBytes.Length)
} finally {
    $fs.Close()
}

$finalSize = (Get-Item $OutExe).Length / 1MB
$sha256 = (Get-FileHash $OutExe -Algorithm SHA256).Hash.ToLower()

Write-Host "==> Standalone ZemoServer.exe generated successfully!"
Write-Host "    Size:   $("{0:N2} MB" -f $finalSize)"
Write-Host "    SHA256: $sha256"
