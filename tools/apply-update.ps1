param(
    [Parameter(Mandatory=$true)][int]$ParentPid,
    [Parameter(Mandatory=$true)][string]$Source,
    [Parameter(Mandatory=$true)][string]$Target,
    [ValidateSet('vs-player.exe','VSRenderer.exe')][string]$Launch = 'vs-player.exe',
    [int]$WaitSeconds = 120,
    [string]$DownloadedArchive = "",
    [string]$ArchiveSha256 = "",
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
function Test-DownloadedArchive([string]$Path,[string]$Expected) {
    $archiveStream=[IO.File]::OpenRead($Path)
    $digestProvider=[Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($digestProvider.ComputeHash($archiveStream)).Replace('-','') -eq $Expected }
    finally { $digestProvider.Dispose();$archiveStream.Dispose() }
}
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
    # Only stop processes from this installation, including tray applications.
    $parentProcess = Get-Process -Id $ParentPid -ErrorAction SilentlyContinue
    if ($ParentPid -gt 0 -and $parentProcess -and (!$parentProcess.Path -or !$parentProcess.Path.StartsWith("$targetRoot\", [StringComparison]::OrdinalIgnoreCase))) { throw 'Parent process is outside the installation' }
    $busy = @(Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.Id -ne $PID -and $_.Path -and $_.Path.StartsWith("$targetRoot\", [StringComparison]::OrdinalIgnoreCase) })
    foreach ($process in $busy) { $null = $process.CloseMainWindow() }
    if ($busy.Count) { Start-Sleep -Milliseconds 500 }
    foreach ($process in $busy) { if (!$process.HasExited) { Stop-Process -Id $process.Id -Force -ErrorAction Stop } }
    $deadline = (Get-Date).AddSeconds($WaitSeconds)
    do {
        $busy = @(Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.Id -ne $PID -and $_.Path -and $_.Path.StartsWith("$targetRoot\", [StringComparison]::OrdinalIgnoreCase) })
        if (-not $busy) { break }
        if ((Get-Date) -ge $deadline) { throw 'Could not stop installation processes. Existing installation unchanged.' }
        foreach ($process in $busy) { Stop-Process -Id $process.Id -Force -ErrorAction Stop }
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
    foreach ($name in @('screenshots','projects','presets','PhotoCraftData','data')) {
        $userPath = Join-Path $targetRoot $name
        if (Test-Path -LiteralPath $userPath) { Copy-Item -LiteralPath $userPath -Destination $incoming -Recurse -Force }
    }
    Get-ChildItem -LiteralPath $targetRoot -Recurse -File | Where-Object { $_.Extension -in '.icc','.icm','.cube' -or $_.Name -eq 'settings.bin' -or $_.Name -like '*.vsr.json' } | ForEach-Object {
        $destination = Join-Path $incoming $_.FullName.Substring($targetRoot.Length + 1)
        New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
        Copy-Item -LiteralPath $_.FullName -Destination $destination -Force
    }
    Move-Item -LiteralPath $targetRoot -Destination $backup
    $renamed = $true
    Move-Item -LiteralPath $incoming -Destination $targetRoot
    $installed = $true
    "Installed successfully" | Set-Content -LiteralPath $log -Encoding UTF8
    Copy-Item -LiteralPath $log -Destination (Join-Path $targetRoot 'update.log') -Force
    if (-not $NoLaunch) { Start-Process -FilePath (Join-Path $targetRoot $Launch) -WorkingDirectory $targetRoot -WindowStyle Normal }
    # Only remove validated updater-generated sibling directories after commit.
    $backupPattern = '^' + [regex]::Escape((Split-Path -Leaf $targetRoot)) + '\.previous-[a-f0-9]{32}$'
    foreach ($previous in Get-ChildItem -LiteralPath $parentRoot -Directory -Force | Where-Object { $_.Name -match $backupPattern }) {
        $resolved = [IO.Path]::GetFullPath($previous.FullName)
        if ((Split-Path -Parent $resolved) -ne $parentRoot -or ($previous.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Invalid previous installation path' }
        if (Get-ChildItem -LiteralPath $resolved -Recurse -Force | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint } | Select-Object -First 1) { throw 'Previous installation contains a reparse point' }
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
    if ($DownloadedArchive -and (Test-Path -LiteralPath $DownloadedArchive -PathType Leaf) -and $ArchiveSha256 -match '^[a-fA-F0-9]{64}$') {
        if (Test-DownloadedArchive $DownloadedArchive $ArchiveSha256) { Remove-Item -LiteralPath $DownloadedArchive -Force }
    }
    $temporaryRoot = [IO.Path]::GetFullPath($PSScriptRoot).TrimEnd('\')
    if ((Split-Path -Parent $temporaryRoot) -eq [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') -and
        (Split-Path -Leaf $temporaryRoot) -match '^vs-gui-update-[A-Za-z0-9]+$' -and
        $sourceRoot.StartsWith("$temporaryRoot\", [StringComparison]::OrdinalIgnoreCase)) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
} catch {
    [Console]::Error.WriteLine(($_ | Out-String))
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
