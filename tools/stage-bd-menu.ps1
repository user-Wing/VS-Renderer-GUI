param(
    [Parameter(Mandatory = $true)][string]$SourceDirectory,
    [string]$OutputDirectory = 'build/mingw-release/runtime/vlc'
)
$ErrorActionPreference = 'Stop'
$source = (Resolve-Path -LiteralPath $SourceDirectory).Path
foreach ($file in @('libvlc.dll', 'libvlccore.dll', 'plugins', 'COPYING.txt', 'AUTHORS.txt')) {
    if (-not (Test-Path -LiteralPath (Join-Path $source $file))) { throw "Incomplete VLC runtime: $file" }
}
$output = [System.IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
foreach ($file in @('libvlc.dll', 'libvlccore.dll', 'plugins', 'COPYING.txt', 'AUTHORS.txt')) {
    Copy-Item -LiteralPath (Join-Path $source $file) -Destination $output -Recurse -Force
}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot '../docs/libvlc-NOTICE.txt') -Destination $output -Force
Write-Host "BD menu runtime staged: $output"
