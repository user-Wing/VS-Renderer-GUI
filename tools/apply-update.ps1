param(
    [Parameter(Mandatory=$true)][int]$ParentPid,
    [Parameter(Mandatory=$true)][string]$Source,
    [Parameter(Mandatory=$true)][string]$Target,
    [ValidateSet('vs-player.exe','VSRenderer.exe')][string]$Launch = 'vs-player.exe',
    [int]$WaitSeconds = 120,
    [switch]$NoLaunch
)
$ErrorActionPreference = 'Stop'
$targetRoot = (Resolve-Path -LiteralPath $Target).Path.TrimEnd('\')
$sourceRoot = (Resolve-Path -LiteralPath $Source).Path.TrimEnd('\')
$parentRoot = Split-Path -Parent $targetRoot
$operationId = [guid]::NewGuid().ToString('N')
$incoming = "$targetRoot.update-$operationId"
$backup = "$targetRoot.previous-$operationId"
$log = Join-Path (Split-Path -Parent $sourceRoot) 'update-result.txt'
$renamed = $false
$installed = $false
try {
    if (-not $parentRoot -or $targetRoot -eq [IO.Path]::GetPathRoot($targetRoot) -or
        $sourceRoot.StartsWith("$targetRoot\", [StringComparison]::OrdinalIgnoreCase) -or
        $targetRoot.StartsWith("$sourceRoot\", [StringComparison]::OrdinalIgnoreCase) -or
        $sourceRoot -eq $targetRoot) { throw 'Invalid update directories' }
    foreach ($path in @($incoming,$backup)) {
        if ((Split-Path -Parent ([IO.Path]::GetFullPath($path))) -ne $parentRoot -or (Test-Path -LiteralPath $path)) { throw 'Invalid update sibling path' }
    }
    foreach ($name in @('VSRenderer.exe','vs-player.exe','FFF.Native.dll','runtime\python\python.exe','release.json')) {
        if (-not (Test-Path -LiteralPath (Join-Path $sourceRoot $name) -PathType Leaf)) { throw "Incomplete update: $name" }
    }
    if (-not (Test-Path -LiteralPath (Join-Path $targetRoot $Launch) -PathType Leaf)) { throw 'Target is not the running portable installation' }
    foreach ($root in @($sourceRoot,$targetRoot)) {
        if ((Get-Item -LiteralPath $root).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Reparse point directory rejected' }
        if (Get-ChildItem -LiteralPath $root -Recurse -Force | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint } | Select-Object -First 1) { throw 'Reparse point entry rejected' }
    }
    $deadline = (Get-Date).AddSeconds($WaitSeconds)
    do {
        $busy = Get-Process -ErrorAction SilentlyContinue | Where-Object {
            ($ParentPid -gt 0 -and $_.Id -eq $ParentPid) -or ($_.Path -and $_.Path.StartsWith("$targetRoot\", [StringComparison]::OrdinalIgnoreCase))
        }
        if (-not $busy) { break }
        if ((Get-Date) -ge $deadline) { throw 'Close Renderer and Player before installing. Existing installation unchanged.' }
        Start-Sleep -Milliseconds 200
    } while ($true)
    New-Item -ItemType Directory -Path $incoming | Out-Null
    & robocopy.exe $sourceRoot $incoming /E /COPY:DAT /DCOPY:DAT /R:2 /W:1 /NFL /NDL /NJH /NJS /NP | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "Payload copy failed: $LASTEXITCODE" }
    Get-ChildItem -LiteralPath $targetRoot -File -Filter '*.ini' | Copy-Item -Destination $incoming -Force
    $oldPresets = Join-Path $targetRoot 'vpy'
    $newPresets = Join-Path $incoming 'vpy'
    if (Test-Path -LiteralPath $oldPresets) {
        New-Item -ItemType Directory -Path $newPresets -Force | Out-Null
        # New built-ins replace old built-ins. The complete old directory remains in the backup.
        Get-ChildItem -LiteralPath $oldPresets -Force | Where-Object { $_.Name -ne 'builtin' } | Copy-Item -Destination $newPresets -Recurse -Force
    }
    foreach ($name in @('screenshots','projects','presets')) {
        $userPath = Join-Path $targetRoot $name
        if (Test-Path -LiteralPath $userPath) { Copy-Item -LiteralPath $userPath -Destination $incoming -Recurse -Force }
    }
    Move-Item -LiteralPath $targetRoot -Destination $backup
    $renamed = $true
    Move-Item -LiteralPath $incoming -Destination $targetRoot
    $installed = $true
    "Installed; previous directory: $backup" | Set-Content -LiteralPath $log -Encoding UTF8
    Copy-Item -LiteralPath $log -Destination (Join-Path $targetRoot 'update.log') -Force
    if (-not $NoLaunch) { Start-Process -FilePath (Join-Path $targetRoot $Launch) -WorkingDirectory $targetRoot -WindowStyle Hidden }
} catch {
    if ($renamed -and -not $installed -and -not (Test-Path -LiteralPath $targetRoot)) {
        Move-Item -LiteralPath $backup -Destination $targetRoot
    }
    "Update failed: $($_.Exception.Message)" | Set-Content -LiteralPath $log -Encoding UTF8
    if (Test-Path -LiteralPath $targetRoot) { Copy-Item -LiteralPath $log -Destination (Join-Path $targetRoot 'update.log') -Force }
    if (-not $NoLaunch) {
        Add-Type -AssemblyName System.Windows.Forms
        [System.Windows.Forms.MessageBox]::Show("$($_.Exception.Message)`n$log", 'VS GUI update') | Out-Null
    }
    exit 1
}
