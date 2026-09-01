param(
    [string]$IdfPath = "D:\Espressif\frameworks\esp-idf-v6.0.2",
    [string]$BuildDir = ".build\idf6"
)

$ErrorActionPreference = "Stop"
$requiredVersion = (Get-Content "$PSScriptRoot\esp-idf-version.txt" -Raw).Trim()
$projectRoot = (Resolve-Path "$PSScriptRoot\..\").Path
$idfPathResolved = (Resolve-Path -LiteralPath $IdfPath -ErrorAction Stop).Path
$exportBat = Join-Path $idfPathResolved "export.bat"
$idfPy = Join-Path $idfPathResolved "tools\idf.py"

if (!(Test-Path -LiteralPath $exportBat) -or !(Test-Path -LiteralPath $idfPy)) {
    throw "ESP-IDF installation is incomplete: $idfPathResolved"
}

$projectRoot = $projectRoot.TrimEnd('\')
$buildPath = [IO.Path]::GetFullPath((Join-Path $projectRoot $BuildDir))
if (!$buildPath.StartsWith($projectRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw "BuildDir must stay inside the project root: $buildPath"
}

$versionOutput = (& cmd.exe /d /s /c "call `"$exportBat`" >nul 2>nul && python `"$idfPy`" --version 2>nul" | Out-String)
$versionExitCode = $LASTEXITCODE
if ($versionExitCode -ne 0 -or ($versionOutput -notmatch [regex]::Escape($requiredVersion))) {
    throw "ESP-IDF version mismatch. Required $requiredVersion, got: $versionOutput"
}

Write-Host "Using ESP-IDF $requiredVersion"
Write-Host "Removing clean-build directory: $buildPath"
if (Test-Path -LiteralPath $buildPath) {
    Remove-Item -LiteralPath $buildPath -Recurse -Force
}
New-Item -ItemType Directory -Path $buildPath -Force | Out-Null

$buildCommand = "call `"$exportBat`" >nul 2>nul && python `"$idfPy`" -B `"$buildPath`" build"
$buildLog = [IO.Path]::GetTempFileName()
try {
    & cmd.exe /d /s /c $buildCommand > $buildLog 2>&1
    $buildExitCode = $LASTEXITCODE
    Get-Content -LiteralPath $buildLog | ForEach-Object { Write-Host $_ }
}
finally {
    Remove-Item -LiteralPath $buildLog -Force -ErrorAction SilentlyContinue
}
if ($buildExitCode -ne 0) {
    throw "ESP-IDF clean build failed with exit code $buildExitCode"
}

Write-Host "Clean build completed: $buildPath"
