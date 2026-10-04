param([string]$Workspace = '.')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $Workspace).Path
$manifest = Get-Content -LiteralPath (Join-Path $root 'handoff-manifest.json') -Raw | ConvertFrom-Json
foreach ($file in $manifest.files) {
    $path = [IO.Path]::GetFullPath((Join-Path $root $file.path))
    if (-not $path.StartsWith($root.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)) { throw "Unsafe manifest path: $($file.path)" }
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing file: $($file.path)" }
    if ((Get-Item -LiteralPath $path).Length -ne $file.bytes -or (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $file.sha256) { throw "Hash mismatch: $($file.path)" }
}
if ($manifest.files.Count -ne $manifest.fileCount) { throw 'Manifest count mismatch.' }
Write-Output "Verified $($manifest.fileCount) files; baseline $($manifest.baseCommit)"
