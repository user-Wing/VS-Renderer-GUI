param(
    [string]$Version = "1.0.0",
    [string]$BuildDirectory = "",
    [string]$NativeRuntimeDirectory = "",
    [string]$SevenZip = "",
    [string]$WinDeployQt = ""
)

$ErrorActionPreference = "Stop"
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if (-not $BuildDirectory) { $BuildDirectory = Join-Path $projectRoot "build\mingw-release" }
if (-not $NativeRuntimeDirectory) { $NativeRuntimeDirectory = Join-Path $projectRoot "build\mingw-debug" }
$BuildDirectory = [System.IO.Path]::GetFullPath($BuildDirectory)
$NativeRuntimeDirectory = [System.IO.Path]::GetFullPath($NativeRuntimeDirectory)
$distRoot = Join-Path $projectRoot "dist"
$packageName = "VS-Renderer-GUI-$Version-windows-x64"
$staging = Join-Path $distRoot $packageName
$archive = Join-Path $distRoot "$packageName.7z"

$exe = Join-Path $BuildDirectory "VSRenderer.exe"
$pythonRuntime = Join-Path $BuildDirectory "runtime\python"
if (-not (Test-Path -LiteralPath $exe)) { throw "Release executable not found: $exe" }
if (-not (Test-Path -LiteralPath $pythonRuntime)) { throw "Bundled VapourSynth runtime not found: $pythonRuntime" }
if (-not (Test-Path -LiteralPath (Join-Path $NativeRuntimeDirectory "FFF.Native.dll"))) {
    throw "FFF.Native.dll not found under $NativeRuntimeDirectory"
}

if (-not $WinDeployQt -and $env:QT_ROOT) {
    $WinDeployQt = Join-Path $env:QT_ROOT "bin\windeployqt.exe"
}
if (-not (Test-Path -LiteralPath $WinDeployQt)) { throw "windeployqt.exe not found; pass -WinDeployQt." }
if (-not (Test-Path -LiteralPath $SevenZip)) { throw "7z.exe not found; pass -SevenZip." }

New-Item -ItemType Directory -Force -Path $distRoot | Out-Null
if (Test-Path -LiteralPath $staging) { Remove-Item -LiteralPath $staging -Recurse -Force }
if (Test-Path -LiteralPath $archive) { Remove-Item -LiteralPath $archive -Force }
New-Item -ItemType Directory -Force -Path $staging | Out-Null

Copy-Item -LiteralPath $exe -Destination $staging
New-Item -ItemType Directory -Force -Path (Join-Path $staging "runtime") | Out-Null
Copy-Item -LiteralPath $pythonRuntime -Destination (Join-Path $staging "runtime\python") -Recurse
Get-ChildItem -LiteralPath $NativeRuntimeDirectory -Filter "*.dll" -File |
    Copy-Item -Destination $staging -Force
Copy-Item -LiteralPath (Join-Path $projectRoot "README.md"),
                           (Join-Path $projectRoot "LICENSE"),
                           (Join-Path $projectRoot "THIRD_PARTY_NOTICES.md") -Destination $staging
Copy-Item -LiteralPath (Join-Path $projectRoot "docs\dependencies.md") `
    -Destination (Join-Path $staging "DEPENDENCIES.md")

& $WinDeployQt --release --no-translations --no-opengl-sw --no-system-d3d-compiler `
    (Join-Path $staging "VSRenderer.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed." }

Push-Location $distRoot
try {
    & $SevenZip a -t7z $archive $packageName -mx=9 -m0=lzma2 -md=128m -mfb=273 -ms=on -mmt=on
    if ($LASTEXITCODE -ne 0) { throw "7-Zip packaging failed." }
} finally {
    Pop-Location
}

$hash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
Set-Content -LiteralPath "$archive.sha256" -Value "$hash  $packageName.7z" -Encoding Ascii
Write-Host "Release package: $archive"
Write-Host "SHA256: $hash"
