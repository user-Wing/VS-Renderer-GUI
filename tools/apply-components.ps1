param(
    [Parameter(Mandatory=$true)][int]$ParentPid,
    [Parameter(Mandatory=$true)][string]$Plan,
    [Parameter(Mandatory=$true)][string]$Target,
    [ValidateSet('vs-player.exe','VSRenderer.exe')][string]$Launch='vs-player.exe',
    [switch]$NoLaunch
)
$ErrorActionPreference='Stop'
$installRoot=(Resolve-Path -LiteralPath $Target).Path.TrimEnd('\')
$planRoot=Split-Path -Parent (Resolve-Path -LiteralPath $Plan).Path
$log=Join-Path $planRoot 'component-install.log'
$changes=@()
$packedRoots=@()
$committed=$false
$backupRoot=Join-Path $planRoot ('backup-'+[guid]::NewGuid().ToString('N'))
function Test-DownloadedArchive([string]$Path,[string]$Expected) {
    $archiveStream=[IO.File]::OpenRead($Path)
    $digestProvider=[Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($digestProvider.ComputeHash($archiveStream)).Replace('-','') -eq $Expected }
    finally { $digestProvider.Dispose();$archiveStream.Dispose() }
}
try {
    if ($installRoot -eq [IO.Path]::GetPathRoot($installRoot).TrimEnd('\') -or !(Test-Path -LiteralPath (Join-Path $installRoot $Launch))) {throw 'Invalid portable installation'}
    $entries=Get-Content -LiteralPath $Plan -Raw | ConvertFrom-Json
    $destinations=@{'ffmpeg'='.';'mkvtoolnix'='runtime\mkvtoolnix';'MadVR'='madVR09217';'LAV-Filters'='LAVFilters64';'aria2-next'='runtime\tools'}
    $programs=@{'ffmpeg'='ffmpeg.exe';'mkvtoolnix'='mkvmerge.exe';'MadVR'='madVR64.ax';'LAV-Filters'='LAVVideo.ax';'aria2-next'='aria2-next.exe'}
    if (!$entries.Count) {throw 'Empty component plan'}
    foreach($entry in $entries){
        if (!$destinations.ContainsKey($entry.id) -or $entry.version -notmatch '^\d+(\.\d+){0,3}$') {throw 'Invalid component identity/version'}
        $sourceRoot=(Resolve-Path -LiteralPath $entry.source).Path.TrimEnd('\')
        if (!$sourceRoot.StartsWith($planRoot+'\',[StringComparison]::OrdinalIgnoreCase) -or !(Test-Path -LiteralPath (Join-Path $sourceRoot $programs[$entry.id]))) {throw 'Invalid staged component'}
        if ((Get-Item -LiteralPath $sourceRoot).Attributes -band [IO.FileAttributes]::ReparsePoint -or (Get-ChildItem -LiteralPath $sourceRoot -Recurse -Force | Where-Object {$_.Attributes -band [IO.FileAttributes]::ReparsePoint} | Select-Object -First 1)) {throw 'Reparse point rejected'}
        $entry | Add-Member -NotePropertyName ResolvedSource -NotePropertyValue $sourceRoot
    }
    $busy=@(Get-Process -ErrorAction SilentlyContinue | Where-Object {$_.Id -ne $PID -and $_.Path -and $_.Path.StartsWith($installRoot+'\',[StringComparison]::OrdinalIgnoreCase)})
    foreach($process in $busy){$null=$process.CloseMainWindow()}
    if($busy.Count){Start-Sleep -Milliseconds 500}
    foreach($process in $busy){if(!$process.HasExited){Stop-Process -Id $process.Id -Force -ErrorAction Stop};if(!$process.WaitForExit(5000)){throw 'Could not stop installation process'}}
    foreach($entry in $entries){
        if($entry.id -eq 'mkvtoolnix' -and (Test-Path -LiteralPath (Join-Path $installRoot 'runtime/mkvtoolnix.zip'))){
            $packed=Join-Path $planRoot ('packed-'+[guid]::NewGuid().ToString('N'));$packedRoots+=,$packed
            $files=Join-Path $packed 'files';$ready=Join-Path $packed 'install'
            New-Item -ItemType Directory -Path $files,$ready -Force | Out-Null
            foreach($name in @('mkvmerge.exe','mkvextract.exe','mkvinfo.exe','mkvpropedit.exe')){Copy-Item -LiteralPath (Join-Path $entry.ResolvedSource $name) -Destination $files}
            New-Item -ItemType Directory -Path (Join-Path $files 'doc') -Force | Out-Null
            foreach($name in @('licenses','en')){Copy-Item -LiteralPath (Join-Path $entry.ResolvedSource "doc/$name") -Destination (Join-Path $files 'doc') -Recurse}
            Compress-Archive -Path (Join-Path $files '*') -DestinationPath (Join-Path $ready 'mkvtoolnix.zip') -CompressionLevel Optimal
            [IO.File]::WriteAllText((Join-Path $ready 'mkvtoolnix.version'),$entry.version)
            $entry.ResolvedSource=$ready;$destinations['mkvtoolnix']='runtime'
        }
        $componentRoot=[IO.Path]::GetFullPath((Join-Path $installRoot $destinations[$entry.id])).TrimEnd('\')
        if($componentRoot -ne $installRoot -and !$componentRoot.StartsWith($installRoot+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Component target escapes installation'}
        foreach($file in Get-ChildItem -LiteralPath $entry.ResolvedSource -Recurse -File){
            if($file.Extension -in '.ini','.vpy' -or $file.Name -eq 'settings.bin'){continue}
            if($entry.id -eq 'ffmpeg' -and ($file.DirectoryName -ne $entry.ResolvedSource -or $file.Extension -notin '.exe','.dll')){continue}
            if($entry.id -eq 'aria2-next' -and $file.Name -ne 'aria2-next.exe'){continue}
            $relative=$file.FullName.Substring($entry.ResolvedSource.Length+1)
            if($relative.Split('\') -contains 'cache'){continue}
            $destination=[IO.Path]::GetFullPath((Join-Path $componentRoot $relative))
            if(!$destination.StartsWith($installRoot+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'File target escapes installation'}
            $ancestor=Split-Path -Parent $destination
            while($ancestor -and $ancestor.StartsWith($installRoot,[StringComparison]::OrdinalIgnoreCase)){
                if((Test-Path -LiteralPath $ancestor) -and ((Get-Item -LiteralPath $ancestor).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Target junction rejected'}
                if($ancestor -eq $installRoot){break};$ancestor=Split-Path -Parent $ancestor
            }
            $backup=Join-Path $backupRoot $destination.Substring($installRoot.Length+1)
            $existed=Test-Path -LiteralPath $destination
            if($existed){New-Item -ItemType Directory -Path (Split-Path -Parent $backup) -Force | Out-Null;Copy-Item -LiteralPath $destination -Destination $backup -Force}
            $changes+=@{Destination=$destination;Backup=$backup;Existed=$existed}
            New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
            Copy-Item -LiteralPath $file.FullName -Destination $destination -Force
        }
    }
    $receiptPath=Join-Path $installRoot 'components.json'
    $receipt=if(Test-Path -LiteralPath $receiptPath){Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json}else{[pscustomobject]@{}}
    foreach($entry in $entries){$receipt | Add-Member -Force -NotePropertyName $entry.id -NotePropertyValue $entry.version}
    $receipt | ConvertTo-Json | Set-Content -LiteralPath $receiptPath -Encoding utf8
    $committed=$true
    'Components installed successfully' | Set-Content -LiteralPath $log -Encoding utf8
    if(!$NoLaunch){Start-Process -FilePath (Join-Path $installRoot $Launch) -WorkingDirectory $installRoot -WindowStyle Normal}
    if(Test-Path -LiteralPath $backupRoot){
        $resolved=[IO.Path]::GetFullPath($backupRoot)
        if((Split-Path -Parent $resolved) -ne $planRoot -or (Get-Item -LiteralPath $resolved).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Invalid component backup path'}
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
    foreach($entry in $entries){
        if($entry.archive -and $entry.sha256 -match '^[a-fA-F0-9]{64}$' -and (Test-Path -LiteralPath $entry.archive -PathType Leaf)){
            $download=[IO.Path]::GetFullPath($entry.archive)
            if($download.StartsWith($planRoot+'\',[StringComparison]::OrdinalIgnoreCase) -and (Test-DownloadedArchive $download $entry.sha256)){Remove-Item -LiteralPath $download -Force}
        }
    }
    foreach($packed in $packedRoots){
        $resolved=[IO.Path]::GetFullPath($packed)
        if((Split-Path -Parent $resolved) -ne $planRoot -or (Split-Path -Leaf $resolved) -notmatch '^packed-[a-f0-9]{32}$'){throw 'Invalid component staging path'}
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
    exit 0
}catch{
    [Console]::Error.WriteLine(($_ | Out-String))
    if(!$committed){for($index=$changes.Count-1;$index -ge 0;$index--){$change=$changes[$index];if($change.Existed){Copy-Item -LiteralPath $change.Backup -Destination $change.Destination -Force}else{if(Test-Path -LiteralPath $change.Destination){Remove-Item -LiteralPath $change.Destination -Force}}}}
    $_ | Out-String | Set-Content -LiteralPath $log -Encoding utf8
    exit 1
}
