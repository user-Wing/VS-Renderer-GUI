param(
    [string]$BuildDirectory = "build/mingw-release",
    [string]$PreviousDirectory = "dist/VS-Renderer-GUI-1.0.1-windows-x64",
    [string]$ShaderDirectory = "C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/libplacebo",
    [string]$LavDirectory = "C:/Program Files/PM Players/LAVFilters64",
    [string]$MadvrDirectory = "C:/PortableSoft/PotPlayer/madVR09217"
)
$ErrorActionPreference = "Stop"
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$output = Join-Path $projectRoot "dist/VS-Renderer-GUI-1.0.2-windows-x64"
$buildRoot = (Resolve-Path $BuildDirectory).Path
$previousRoot = (Resolve-Path $PreviousDirectory).Path
foreach ($exe in @("VSRenderer.exe", "vs-player.exe")) {
    if (-not (Test-Path (Join-Path $buildRoot $exe))) { throw "Missing build: $exe" }
}
$initialStage = -not (Test-Path -LiteralPath $output)
New-Item -ItemType Directory -Path $output -Force | Out-Null
if ($initialStage) {
    Get-ChildItem -LiteralPath $previousRoot | Copy-Item -Destination $output -Recurse -Force
}
foreach ($exe in @("VSRenderer.exe", "vs-player.exe", "FFF.Native.dll")) {
    Copy-Item -LiteralPath (Join-Path $buildRoot $exe) -Destination $output -Force
}
Copy-Item -LiteralPath (Join-Path $buildRoot "runtime/python") -Destination (Join-Path $output "runtime") -Recurse -Force
New-Item -ItemType Directory -Path (Join-Path $output "shaders"), (Join-Path $output "vpy") -Force | Out-Null
Get-ChildItem -LiteralPath $ShaderDirectory -Filter "*Anime4K*.glsl" -File | Copy-Item -Destination (Join-Path $output "shaders") -Force
Copy-Item -LiteralPath (Join-Path $projectRoot "assets/anime4k-a-fast.glsl") -Destination (Join-Path $output "shaders") -Force
& (Join-Path $PSScriptRoot 'stage-update-tools.ps1') -OutputDirectory (Join-Path $output 'runtime/tools')
@{version='1.0.2';schema=1;platform='windows-x64'} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'release.json') -Encoding UTF8
New-Item -ItemType Directory -Path (Join-Path $output "languages") -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $projectRoot "assets/languages/zh_CN.json"), (Join-Path $projectRoot "assets/languages/en_US.json") -Destination (Join-Path $output "languages") -Force
New-Item -ItemType Directory -Path (Join-Path $output "LAVFilters64"), (Join-Path $output "madVR09217") -Force | Out-Null
Get-ChildItem -LiteralPath $LavDirectory | Copy-Item -Destination (Join-Path $output "LAVFilters64") -Recurse -Force
Get-ChildItem -LiteralPath $MadvrDirectory | Copy-Item -Destination (Join-Path $output "madVR09217") -Recurse -Force
Copy-Item -LiteralPath (Join-Path $projectRoot "README.md"), (Join-Path $projectRoot "project.md"), (Join-Path $projectRoot "changelog.md"), (Join-Path $projectRoot "THIRD_PARTY_NOTICES.md") -Destination $output -Force
Copy-Item -LiteralPath (Join-Path $projectRoot "docs/dependencies.md") -Destination (Join-Path $output "DEPENDENCIES.md") -Force
New-Item -ItemType Directory -Path (Join-Path $output "docs") -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $projectRoot "docs/player-performance-1.0.2.md"), (Join-Path $projectRoot "docs/anime4k-fast-source.md"), (Join-Path $projectRoot "docs/portable-updates.md") -Destination (Join-Path $output "docs") -Force
Write-Host "Local 1.0.2 program directory: $output"
