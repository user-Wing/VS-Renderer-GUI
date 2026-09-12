param(
    [string]$Venv = "",
    [string]$OutputDirectory = ""
)

$ErrorActionPreference = "Stop"
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if (-not $Venv) { $Venv = Join-Path $projectRoot ".deps\vs-python" }
$Venv = (Resolve-Path $Venv).Path
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $projectRoot "build\mingw-debug\runtime\python"
}
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)

$python = Join-Path $Venv "Scripts\python.exe"
$package = Join-Path $Venv "Lib\site-packages\vapoursynth"
if (-not (Test-Path $python) -or -not (Test-Path (Join-Path $package "vsscript.dll"))) {
    throw "Built VapourSynth environment not found under $Venv"
}

$versionDirectory = & $python -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}')"
$versionTag = & $python -c "import sys; v=sys.version_info; s=f'{v.major}.{v.minor}.{v.micro}'; print(s if v.releaselevel == 'final' else s + {'alpha':'a','beta':'b','candidate':'rc'}[v.releaselevel] + str(v.serial))"
$archive = Join-Path (Join-Path $projectRoot ".deps") "python-$versionTag-embed-amd64.zip"
if (-not (Test-Path $archive)) {
    $url = "https://www.python.org/ftp/python/$versionDirectory/python-$versionTag-embed-amd64.zip"
    Write-Host "Downloading embedded Python $versionTag"
    Invoke-WebRequest -Uri $url -OutFile $archive
}

if (Test-Path $OutputDirectory) { Remove-Item -LiteralPath $OutputDirectory -Recurse -Force }
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
Expand-Archive -LiteralPath $archive -DestinationPath $OutputDirectory -Force
New-Item -ItemType Directory -Force -Path (Join-Path $OutputDirectory "DLLs") | Out-Null

$sitePackage = Join-Path $OutputDirectory "Lib\site-packages\vapoursynth"
$plugins = Join-Path $sitePackage "plugins"
New-Item -ItemType Directory -Force -Path $plugins | Out-Null
Copy-Item (Join-Path $package "*.py"),
              (Join-Path $package "*.pyi"),
              (Join-Path $package "*.pyd"),
              (Join-Path $package "*.dll"),
              (Join-Path $package "*.exe") -Destination $sitePackage -Force

$pluginFiles = @(
    "plugins\cas.dll",
    "plugins\bwdif.dll",
    "plugins\deblock.dll",
    "plugins\eedi3m.dll",
    "plugins\nnedi3_weights.bin",
    "plugins\sangnom.dll",
    "plugins\vivtc.dll",
    "plugins\vsnlm_ispc.dll",
    "plugins\znedi3.dll",
    "plugins\vsrepo\AddGrain.dll",
    "plugins\vsrepo\ffms2.dll",
    "plugins\vsrepo\fmtconv.dll",
    "plugins\vsrepo\libfftw3-3.dll",
    "plugins\vsrepo\libfftw3f-3.dll",
    "plugins\vsrepo\LSMASHSource.dll",
    "plugins\vsrepo\RemoveGrainVS.dll"
)
foreach ($relative in $pluginFiles) {
    $source = Join-Path $package $relative
    if (-not (Test-Path $source)) { throw "Required VapourSynth plugin not found: $relative" }
    Copy-Item -LiteralPath $source -Destination $plugins -Force
}

$vszipSource = Join-Path $package "plugins\vszip"
if (-not (Test-Path (Join-Path $vszipSource "vszip.dll"))) {
    throw "Required VapourSynth plugin not found: plugins\vszip\vszip.dll"
}
Copy-Item -LiteralPath $vszipSource -Destination $plugins -Recurse -Force

$zsmoothSource = Join-Path $package "plugins\zsmooth"
if (-not (Test-Path (Join-Path $zsmoothSource "zsmooth.dll"))) {
    throw "Required VapourSynth plugin not found: plugins\zsmooth\zsmooth.dll"
}
Copy-Item -LiteralPath $zsmoothSource -Destination $plugins -Recurse -Force

$pth = Get-ChildItem $OutputDirectory -Filter "python*._pth" | Select-Object -First 1
if (-not $pth) { throw "Embedded Python path file was not found." }
$paths = Get-Content -LiteralPath $pth.FullName
if ($paths -notcontains "Lib\site-packages") {
    $paths += "Lib\site-packages"
    Set-Content -LiteralPath $pth.FullName -Value $paths -Encoding Ascii
}

Copy-Item (Join-Path $projectRoot "third_party\vapoursynth\COPYING.LESSER") `
    -Destination (Join-Path $OutputDirectory "VapourSynth-COPYING.LESSER.txt") -Force
Copy-Item (Join-Path $OutputDirectory "LICENSE.txt") `
    -Destination (Join-Path $OutputDirectory "Python-LICENSE.txt") -Force

Write-Host "Bundled VapourSynth runtime ready: $OutputDirectory"
