param(
    [string]$Venv = "",
    [string]$ModelDirectory = "C:\PortableSoft\FFmpegFreeUI ReadyToRun x64\plugin\videoenhancer\models\Frame-Interpolation\RIFE",
    [string]$SevenZip = "C:\PortableSoft\7-Zip-Zstandard\7z.exe"
)
$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if (-not $Venv) { $Venv = Join-Path $root ".deps\vs-python" }
$package = Join-Path $Venv "Lib\site-packages\vapoursynth"
$plugins = Join-Path $package "plugins"
$env:PATH = "$(Split-Path $SevenZip);$env:PATH"
& (Join-Path $Venv "Scripts\vsrepo.exe") install descale
if ($LASTEXITCODE -ne 0) { throw "Descale installation failed" }
$manifest = Get-Content (Join-Path $root "third_party\rife\runtime-sha256.json") -Raw | ConvertFrom-Json
$url = "https://github.com/styler00dollar/VapourSynth-RIFE-ncnn-Vulkan/releases/download/r9_mod_v33/librife_windows_x86-64.dll"
$dll = Join-Path $plugins "librife.dll"
Invoke-WebRequest -Uri $url -OutFile $dll
if ((Get-FileHash $dll -Algorithm SHA256).Hash.ToLowerInvariant() -ne $manifest.'librife.dll') { throw "RIFE DLL checksum mismatch" }
foreach ($model in @("rife-v4.26", "rife-v4.26-heavy")) {
    $target = Join-Path (Join-Path $plugins "models") $model
    New-Item -ItemType Directory -Force -Path $target | Out-Null
    foreach ($name in @("flownet.bin", "flownet.param")) {
        $source = Join-Path (Join-Path $ModelDirectory $model) $name
        $key = "$model\$name"
        if (-not (Test-Path $source)) { throw "Missing converted NCNN model: $source; PKL files cannot be substituted" }
        if ((Get-FileHash $source -Algorithm SHA256).Hash.ToLowerInvariant() -ne $manifest.$key) { throw "Model checksum mismatch: $source" }
        Copy-Item -LiteralPath $source -Destination $target -Force
    }
}
Write-Host "RIFE 4.26/Heavy Vulkan and Descale ready; stage the runtime next."
