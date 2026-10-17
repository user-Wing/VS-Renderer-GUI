param(
    [string]$BuildDirectory = "build/mingw-release",
    [string]$PreviousDirectory = "dist/VS-Renderer-GUI-1.0.1-windows-x64",
    [string]$ShaderDirectory = "C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/libplacebo",
    [string]$LavDirectory = (Join-Path $PSScriptRoot "../.deps/lav/0.83"),
    [string]$MadvrDirectory = "C:/PortableSoft/PotPlayer/madVR09217",
    [string]$MkvtoolnixDirectory = "C:/PortableSoft/Mkvtoolnix"
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
Copy-Item -LiteralPath (Join-Path $buildRoot 'Qt6Concurrent.dll') -Destination $output -Force
New-Item -ItemType Directory -Path (Join-Path $output 'tools') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $projectRoot 'tools/download-bd.py') -Destination (Join-Path $output 'tools') -Force
if (Test-Path -LiteralPath (Join-Path $buildRoot 'color/vsr-color.dll')) {
    Copy-Item -LiteralPath (Join-Path $buildRoot 'color') -Destination $output -Recurse -Force
}
Copy-Item -LiteralPath (Join-Path $buildRoot "runtime/python") -Destination (Join-Path $output "runtime") -Recurse -Force
& (Join-Path $PSScriptRoot 'stage-ffmpeg.ps1') -OutputDirectory $output
if (-not (Test-Path -LiteralPath (Join-Path $buildRoot 'runtime/awj/AWJ.exe'))) { throw 'Missing AWJimage backend; build VSPlayer before staging.' }
Copy-Item -LiteralPath (Join-Path $buildRoot 'runtime/awj') -Destination (Join-Path $output 'runtime') -Recurse -Force
& (Join-Path $PSScriptRoot 'stage-bd-menu.ps1') -SourceDirectory (Join-Path $buildRoot 'runtime/vlc') -OutputDirectory (Join-Path $output 'runtime/vlc')
New-Item -ItemType Directory -Path (Join-Path $output "shaders"), (Join-Path $output "vpy") -Force | Out-Null
Get-ChildItem -LiteralPath $ShaderDirectory -Filter "*Anime4K*.glsl" -File | Copy-Item -Destination (Join-Path $output "shaders") -Force
Copy-Item -LiteralPath (Join-Path $projectRoot "assets/anime4k-a-fast.glsl"), (Join-Path $projectRoot "assets/anime4k-no-cnn.glsl") -Destination (Join-Path $output "shaders") -Force
& (Join-Path $PSScriptRoot 'stage-shaders.ps1') -OutputDirectory (Join-Path $output 'shaders') -SevenZip (Join-Path $output 'runtime/tools/7z.exe')
& (Join-Path $PSScriptRoot 'stage-update-tools.ps1') -OutputDirectory (Join-Path $output 'runtime/tools')
& (Join-Path $PSScriptRoot 'stage-mkvtoolnix.ps1') -SourceDirectory $MkvtoolnixDirectory -OutputDirectory (Join-Path $output 'runtime/mkvtoolnix')
if (!(Test-Path -LiteralPath (Join-Path $output 'components.json'))) { Copy-Item -LiteralPath (Join-Path $projectRoot 'assets/full-components.json') -Destination (Join-Path $output 'components.json') }
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
& (Join-Path $PSScriptRoot 'trim-portable.ps1') -OutputDirectory $output
Copy-Item -LiteralPath (Join-Path $projectRoot "README.md"), (Join-Path $projectRoot "project.md"), (Join-Path $projectRoot "changelog.md"), (Join-Path $projectRoot "THIRD_PARTY_NOTICES.md") -Destination $output -Force
Copy-Item -LiteralPath (Join-Path $projectRoot "docs/dependencies.md") -Destination (Join-Path $output "DEPENDENCIES.md") -Force
New-Item -ItemType Directory -Path (Join-Path $output "docs") -Force | Out-Null
Get-ChildItem -LiteralPath (Join-Path $projectRoot 'docs') -Filter '*.md' -File | Copy-Item -Destination (Join-Path $output 'docs') -Force
Write-Host "Local $version program directory: $output"

Copy-Item -LiteralPath (Join-Path $projectRoot "assets/image-runtime-LICENSE.txt") -Destination (Join-Path $output "IMAGE-LICENSE.txt") -Force
New-Item -ItemType Directory -Path (Join-Path $output 'licenses') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $projectRoot 'third_party/image-zlib/LICENSE') -Destination (Join-Path $output 'licenses/image-zlib-LICENSE.txt') -Force
Get-ChildItem -LiteralPath (Join-Path $projectRoot 'third_party/runtime-notices') -File | Copy-Item -Destination (Join-Path $output 'licenses') -Force
