param (
    [string]$JdkPath = "C:\Program Files\Eclipse Adoptium\jdk-21.0.9.10-hotspot",
    [string]$AppName = "zemoLibraryServer"
)

$ErrorActionPreference = "Stop"

$RootDir = Resolve-Path "$PSScriptRoot\.."
$BackendDir = "$RootDir\backend"
$TargetJar = "$BackendDir\target\backend-0.0.1-SNAPSHOT.jar"
$StagingDir = "$BackendDir\target\package-input"
$DistDir = "$RootDir\dist"

$JPackageExe = "$JdkPath\bin\jpackage.exe"
if (-not (Test-Path $JPackageExe)) {
    $JPackageCmd = Get-Command jpackage -ErrorAction SilentlyContinue
    if ($JPackageCmd) {
        $JPackageExe = $JPackageCmd.Source
    } else {
        throw "jpackage.exe not found at '$JPackageExe' or in PATH."
    }
}

Write-Host "==> Verifying backend JAR..."
if (-not (Test-Path $TargetJar)) {
    throw "Backend JAR not found at '$TargetJar'. Please run './mvnw clean package' first."
}

Write-Host "==> Preparing packaging staging directory..."
if (Test-Path $StagingDir) {
    Remove-Item -Path $StagingDir -Recurse -Force
}
New-Item -ItemType Directory -Path $StagingDir -Force | Out-Null
Copy-Item -Path $TargetJar -Destination "$StagingDir\backend-0.0.1-SNAPSHOT.jar" -Force

if (Test-Path "$DistDir\$AppName") {
    Write-Host "==> Removing existing dist\$AppName..."
    Remove-Item -Path "$DistDir\$AppName" -Recurse -Force
}

Write-Host "==> Running jpackage to create application image in dist/..."
& $JPackageExe `
    --type app-image `
    --name $AppName `
    --input $StagingDir `
    --main-jar "backend-0.0.1-SNAPSHOT.jar" `
    --dest $DistDir `
    --win-console

Write-Host "==> Cleaning staging directory..."
Remove-Item -Path $StagingDir -Recurse -Force

Write-Host "==> Packaging complete! Executable located at: $DistDir\$AppName\$AppName.exe"
