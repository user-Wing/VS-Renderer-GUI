param([switch]$Preview)
$ErrorActionPreference = 'Stop'
$workspace = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..')).TrimEnd('\')
foreach ($marker in @('CMakeLists.txt', 'src/player/PlayerWindow.cpp', 'tools/stage-portable.ps1')) {
    if (-not (Test-Path -LiteralPath (Join-Path $workspace $marker))) { throw "Wrong workspace: $workspace" }
}
$relativeTargets = @(
    'build/release-stage-1.0.3-before-mpv-interpolation',
    'build/release-stage-1.0.3',
    'build/package-verify-1.0.3',
    'build/release-stage',
    'build/release-archive-extracted',
    'build/ffmpeg-backups',
    'build/shader-backups',
    'build/portable-trim-backups',
    'build/lean-shaders',
    'build/mingw-interpolation',
    'build/mingw-debug',
    'build/release-smoke-5e6f1c88d2c744bcad78a66c52d5e8fd',
    'build/renderer-ui-1.0.3',
    'build/shader-audit-runtime',
    'build/legacy-update-check',
    'build/AWJimage-source',
    'build/icons-backup-20261002-203912',
    '.deps/vs-build',
    '.deps/libavif',
    'build/anime4k-upstream.zip',
    'build/supplied-large-yuv444-corrected.avif',
    'build/supplied-large-yuv420.avif',
    'build/supplied-large-grid.avif',
    'build/player-performance.mkv',
    'build/interpolation-testsrc.mkv',
    'build/renderer-panels-previous-VSRenderer.exe',
    'build/renderer-ui-previous-VSRenderer.exe'
)
$targets = @()
$files = @()
foreach ($relative in $relativeTargets) {
    $absolute = [IO.Path]::GetFullPath((Join-Path $workspace $relative))
    if (-not $absolute.StartsWith($workspace + '\', [StringComparison]::OrdinalIgnoreCase)) { throw "Target escapes workspace: $absolute" }
    # Reject junctions in the target, its contents, or any parent before deleting.
    $ancestor = $absolute
    while ($ancestor -and $ancestor -ne [IO.Path]::GetPathRoot($ancestor)) {
        if (Test-Path -LiteralPath $ancestor) {
            if ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Linked path: $ancestor" }
        }
        $ancestor = Split-Path -Parent $ancestor
    }
    if (-not (Test-Path -LiteralPath $absolute)) { continue }
    $item = Get-Item -LiteralPath $absolute -Force
    if ($item.PSIsContainer) {
        $children = @(Get-ChildItem -LiteralPath $absolute -Recurse -Force)
        if ($children | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint }) { throw "Linked content: $absolute" }
        $files += @($children | Where-Object { -not $_.PSIsContainer })
    } else { $files += $item }
    $targets += $absolute
    Write-Host "[cleanup] $relative"
}
$bytes = ($files | Measure-Object Length -Sum).Sum
Write-Host ('Selected: {0} targets, {1:N2} GiB' -f $targets.Count, ($bytes / 1GB))
Write-Host 'Keep: current Release build, portable output, source, configurations and presets.'
if ($Preview) { Write-Host 'Preview only. Nothing deleted.'; exit 0 }

$backup = Join-Path $workspace 'build/retained-user-files'
foreach ($file in $files | Where-Object { $_.Extension -eq '.ini' -or $_.Name -match '\.vpy($|\.)' -or $_.Name -eq 'settings.bin' }) {
    $destination = Join-Path $backup $file.FullName.Substring($workspace.Length + 1)
    if (Test-Path -LiteralPath $destination) {
        if ((Get-FileHash -LiteralPath $file.FullName).Hash -eq (Get-FileHash -LiteralPath $destination).Hash) { continue }
        $destination += '.' + [DateTime]::Now.ToString('yyyyMMdd-HHmmss-fff')
    }
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath $file.FullName -Destination $destination
    if ((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath $destination).Hash) { throw "Backup mismatch: $destination" }
}
foreach ($target in $targets) {
    Remove-Item -LiteralPath $target -Recurse -Force
    if (Test-Path -LiteralPath $target) { throw "Could not remove: $target" }
}
New-Item -ItemType Directory -Path (Join-Path $workspace 'build') -Force | Out-Null
@{ completedAt = [DateTime]::Now.ToString('o'); removedTargets = $targets; removedBytes = $bytes; retainedUserFiles = $backup } |
    ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $workspace 'build/workspace-cleanup-result.json') -Encoding UTF8
Write-Host ('Completed. Removed {0:N2} GiB. Configurations retained in build/retained-user-files.' -f ($bytes / 1GB))
