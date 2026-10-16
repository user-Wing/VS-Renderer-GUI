param([string]$Python='C:\Program Files\Python312\python.exe')
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$targets=@('vs-player.exe','vs-cap.exe','VSRenderer.exe')
$running=@(Get-CimInstance Win32_Process | Where-Object {
    $_.ExecutablePath -and $targets -contains $_.Name -and $_.ExecutablePath.StartsWith((Join-Path $root 'dist'),[StringComparison]::OrdinalIgnoreCase)
})
if($running.Count){throw 'Close the runtime normally before deployment; pinned images and settings must be preserved.'}
# Use current build artifacts, never reactivate the older *-next.exe snapshots.
& $Python (Join-Path $PSScriptRoot 'deploy-local-runtime.py')
if($LASTEXITCODE -ne 0){throw 'Local runtime deployment failed.'}
