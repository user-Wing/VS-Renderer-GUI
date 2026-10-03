param(
    [string]$OutputDirectory,
    [string]$SevenZip
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$source = Join-Path $projectRoot 'assets/mpv-shaders'
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (-not $output.StartsWith($projectRoot.TrimEnd('\')+'\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Shader deployment escapes workspace' }
if (Test-Path -LiteralPath (Join-Path $output 'mpv-shaders')) {
    $existing = (Resolve-Path -LiteralPath (Join-Path $output 'mpv-shaders')).Path
    if (-not $existing.StartsWith($output.TrimEnd('\')+'\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid shader deployment path' }
    $backup = Join-Path $projectRoot ('build/shader-backups/' + [DateTime]::Now.ToString('yyyyMMdd-HHmmss-fff'))
    New-Item -ItemType Directory -Path $backup -Force | Out-Null
    Move-Item -LiteralPath $existing -Destination $backup
}
New-Item -ItemType Directory -Path (Join-Path $output 'mpv-shaders') -Force | Out-Null
$archive = Join-Path $output 'mpv-shaders-new.7z'
Push-Location (Join-Path $projectRoot 'assets')
try {
    & $SevenZip a $archive mpv-shaders -t7z -m0=lzma2 -mx=9 -ms=64m -md=32m -mmt=2 -bso0 -bsp0
    if ($LASTEXITCODE -ne 0) { throw 'Shader archive creation failed' }
    & $SevenZip t $archive -bso0 -bsp0
    if ($LASTEXITCODE -ne 0) { throw 'Shader archive verification failed' }
} finally { Pop-Location }
foreach ($file in Get-ChildItem -LiteralPath $source -Recurse -File | Where-Object { $_.Extension -notin @('.glsl', '.hook') }) {
    $destination = Join-Path (Join-Path $output 'mpv-shaders') $file.FullName.Substring($source.Length+1)
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath $file.FullName -Destination $destination -Force
}
Move-Item -LiteralPath $archive -Destination (Join-Path $output 'mpv-shaders.7z') -Force
