param(
    [string]$PythonLauncher = "py",
    [string]$PythonVersion = "3.15",
    [string]$SevenZip = ""
)

$ErrorActionPreference = "Stop"
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$source = Join-Path $projectRoot "third_party\vapoursynth"
$dependencyRoot = Join-Path $projectRoot ".deps"
$venv = Join-Path $dependencyRoot "vs-python"
$build = Join-Path $dependencyRoot "vs-build"

if (-not (Test-Path (Join-Path $source "meson.build"))) {
    New-Item -ItemType Directory -Force -Path (Split-Path $source) | Out-Null
    & git clone https://github.com/vapoursynth/vapoursynth.git $source
    if ($LASTEXITCODE -ne 0) { throw "VapourSynth clone failed." }
}

if (-not (Test-Path (Join-Path $venv "Scripts\python.exe"))) {
    if ($PythonLauncher -eq "py") {
        & py "-$PythonVersion" -m venv $venv
    } else {
        & $PythonLauncher -m venv $venv
    }
    if ($LASTEXITCODE -ne 0) { throw "Python virtual environment creation failed." }
}

$python = Join-Path $venv "Scripts\python.exe"
$scripts = Join-Path $venv "Scripts"
& $python -m pip install "Cython>=3.3.0" meson ninja meson-python vsrepo `
    "vapoursynth-cas==3.0" "vapoursynth-nlm-ispc==4.0" "vapoursynth-vszip==22.1.0" `
    "vapoursynth-zsmooth==0.20.0" "vapoursynth-deblock==9.0" "vapoursynth-znedi3==3.3" `
    "vapoursynth-eedi3==10.0" "vapoursynth-sangnom==45.0" "vapoursynth-bwdif==5.1" `
    "vapoursynth-vivtc==2.0" "vs-placebo==2.0.4" "vapoursynth-mvtools==29"
if ($LASTEXITCODE -ne 0) { throw "VapourSynth build dependencies failed to install." }

$env:PATH = "$scripts;$env:PATH"
$meson = Join-Path $scripts "meson.exe"
$configured = Test-Path (Join-Path $build "meson-private\coredata.dat")
$setupArgs = @("setup")
if ($configured) { $setupArgs += "--wipe" }
$setupArgs += @($build, $source, "--vsenv", "-Denable_x86_asm=false", "--prefix=$venv")
& $meson @setupArgs
if ($LASTEXITCODE -ne 0) { throw "VapourSynth Meson setup failed." }

& $meson compile -C $build libvapoursynth vapoursynth vsscript vspipe libvapoursynthfilters
if ($LASTEXITCODE -ne 0) { throw "VapourSynth build failed." }

if (-not $SevenZip) {
    $sevenZipCommand = Get-Command 7z.exe -ErrorAction SilentlyContinue
    if ($sevenZipCommand) { $SevenZip = $sevenZipCommand.Source }
}
if ($SevenZip -and (Test-Path $SevenZip)) {
    $env:PATH = "$(Split-Path $SevenZip);$env:PATH"
    & (Join-Path $scripts "vsrepo.exe") update
    if ($LASTEXITCODE -ne 0) { throw "VSRepo index update failed." }
    & (Join-Path $scripts "vsrepo.exe") install lsmas ffms2 fmtc rgvs grain
    if ($LASTEXITCODE -ne 0) { throw "VapourSynth plugin installation failed." }
} else {
    Write-Warning "7z.exe not found; VSRepo plugin set was not installed. Pass -SevenZip <path>."
}

$runtime = Join-Path $venv "Lib\site-packages\vapoursynth"
New-Item -ItemType Directory -Force -Path $runtime | Out-Null
Copy-Item (Join-Path $build "vapoursynth.pyd"),
              (Join-Path $build "vsscript.dll"),
              (Join-Path $build "libvapoursynth.dll"),
              (Join-Path $build "libvapoursynthfilters.dll"),
              (Join-Path $build "vspipe.exe") -Destination $runtime -Force
Copy-Item (Join-Path $source "src\py\*.py"), (Join-Path $source "src\py\*.pyi") -Destination $runtime -Force

& (Join-Path $runtime "vspipe.exe") --version
if ($LASTEXITCODE -ne 0) { throw "VapourSynth runtime smoke test failed." }

& (Join-Path $PSScriptRoot "install-analysis-plugins.ps1") -Venv $venv -SevenZip $SevenZip
if ($LASTEXITCODE -ne 0) { throw "Analysis plugin installation failed." }

& (Join-Path $PSScriptRoot "stage-vapoursynth-runtime.ps1") -Venv $venv
if ($LASTEXITCODE -ne 0) { throw "VapourSynth runtime staging failed." }

Write-Host "VapourSynth runtime ready: $runtime"
