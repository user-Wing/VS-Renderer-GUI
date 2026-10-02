param(
    [string]$BuildDirectory = "build/mingw-release",
    [string]$PreviousDirectory = "dist/VS-Renderer-GUI-1.0.1-windows-x64",
    [string]$ShaderDirectory = "C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/libplacebo",
    [string]$LavDirectory = (Join-Path $PSScriptRoot "../.deps/lav/0.83"),
    [string]$MadvrDirectory = "C:/PortableSoft/PotPlayer/madVR09217"
)
$ErrorActionPreference = "Stop"
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$output = Join-Path $projectRoot "dist/VS-Renderer-GUI-windows-x64"
$version = [regex]::Match((Get-Content -LiteralPath (Join-Path $projectRoot 'CMakeLists.txt') -Raw), 'project\(VSRenderer VERSION ([0-9.]+)').Groups[1].Value
if (-not $version) { throw 'Project version not found' }
$buildRoot = (Resolve-Path $BuildDirectory).Path
foreach ($exe in @("VSRenderer.exe", "vs-player.exe")) {
    if (-not (Test-Path (Join-Path $buildRoot $exe))) { throw "Missing build: $exe" }
}
$initialStage = -not (Test-Path -LiteralPath $output)
New-Item -ItemType Directory -Path $output -Force | Out-Null
if ($initialStage) {
    $previousRoot = (Resolve-Path $PreviousDirectory).Path
    Get-ChildItem -LiteralPath $previousRoot | Copy-Item -Destination $output -Recurse -Force
}
foreach ($exe in @("VSRenderer.exe", "vs-player.exe", "FFF.Native.dll")) {
    Copy-Item -LiteralPath (Join-Path $buildRoot $exe) -Destination $output -Force
}
Copy-Item -LiteralPath (Join-Path $buildRoot "runtime/python") -Destination (Join-Path $output "runtime") -Recurse -Force
New-Item -ItemType Directory -Path (Join-Path $output "shaders"), (Join-Path $output "vpy") -Force | Out-Null
Get-ChildItem -LiteralPath $ShaderDirectory -Filter "*Anime4K*.glsl" -File | Copy-Item -Destination (Join-Path $output "shaders") -Force
Copy-Item -LiteralPath (Join-Path $projectRoot "assets/anime4k-a-fast.glsl"), (Join-Path $projectRoot "assets/anime4k-no-cnn.glsl") -Destination (Join-Path $output "shaders") -Force
Copy-Item -LiteralPath (Join-Path $projectRoot "assets/mpv-shaders") -Destination (Join-Path $output "shaders") -Recurse -Force
& (Join-Path $PSScriptRoot 'stage-update-tools.ps1') -OutputDirectory (Join-Path $output 'runtime/tools')
@{version=$version;schema=1;platform='windows-x64'} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'release.json') -Encoding UTF8
New-Item -ItemType Directory -Path (Join-Path $output "languages") -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $projectRoot "assets/languages/zh_CN.json"), (Join-Path $projectRoot "assets/languages/en_US.json") -Destination (Join-Path $output "languages") -Force
New-Item -ItemType Directory -Path (Join-Path $output "LAVFilters64"), (Join-Path $output "madVR09217") -Force | Out-Null
foreach ($filter in @('LAVVideo.ax', 'LAVAudio.ax', 'LAVSplitter.ax', 'COPYING')) {
    if (-not (Test-Path -LiteralPath (Join-Path $LavDirectory $filter))) { throw "Missing LAV runtime file: $filter in $LavDirectory" }
}
Get-ChildItem -LiteralPath $LavDirectory | Copy-Item -Destination (Join-Path $output "LAVFilters64") -Recurse -Force
$lavNames = @(Get-ChildItem -LiteralPath $LavDirectory -File | Select-Object -ExpandProperty Name)
Get-ChildItem -LiteralPath (Join-Path $output "LAVFilters64") -Filter '*-lav-*.dll' -File |
    Where-Object { $_.Name -notin $lavNames } | ForEach-Object { Remove-Item -LiteralPath $_.FullName }
Get-ChildItem -LiteralPath $MadvrDirectory | Copy-Item -Destination (Join-Path $output "madVR09217") -Recurse -Force
Copy-Item -LiteralPath (Join-Path $projectRoot "README.md"), (Join-Path $projectRoot "project.md"), (Join-Path $projectRoot "changelog.md"), (Join-Path $projectRoot "THIRD_PARTY_NOTICES.md") -Destination $output -Force
Copy-Item -LiteralPath (Join-Path $projectRoot "docs/dependencies.md") -Destination (Join-Path $output "DEPENDENCIES.md") -Force
New-Item -ItemType Directory -Path (Join-Path $output "docs") -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $projectRoot "docs/player-performance-1.0.2.md"), (Join-Path $projectRoot "docs/anime4k-fast-source.md"), (Join-Path $projectRoot "docs/portable-updates.md"), (Join-Path $projectRoot "docs/player-images-menu-1.0.3.md"), (Join-Path $projectRoot "docs/player-images-audio-1.0.3.md"), (Join-Path $projectRoot "docs/player-six-stage-1.0.3.md"), (Join-Path $projectRoot "docs/player-resolution-threshold-1.0.3.md") -Destination (Join-Path $output "docs") -Force
Write-Host "Local $version program directory: $output"

Copy-Item -LiteralPath (Join-Path $projectRoot "assets/image-runtime-LICENSE.txt") -Destination (Join-Path $output "IMAGE-LICENSE.txt") -Force
Copy-Item -LiteralPath (Join-Path $projectRoot "docs/player-apply-avif-1.0.3.md") -Destination (Join-Path $output "docs") -Force
Copy-Item -LiteralPath (Join-Path $projectRoot "docs/player-manual-presets-1.0.3.md") -Destination (Join-Path $output "docs") -Force
Copy-Item -LiteralPath (Join-Path $projectRoot "docs/release-1.0.3.md"), (Join-Path $projectRoot "docs/avif-large-yuv444-analysis.md"), (Join-Path $projectRoot "docs/renderer-panels-1.0.3.md"), (Join-Path $projectRoot "docs/renderer-ui-export-1.0.3.md"), (Join-Path $projectRoot "docs/icons-integration.md") -Destination (Join-Path $output "docs") -Force
Copy-Item -LiteralPath (Join-Path $projectRoot "docs/mpv-shaders-interpolation-1.0.3.md") -Destination (Join-Path $output "docs") -Force
