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
$backupRoot=Join-Path $planRoot ('backup-'+[guid]::NewGuid().ToString('N'))
try {
    if ($installRoot -eq [IO.Path]::GetPathRoot($installRoot).TrimEnd('\') -or !(Test-Path -LiteralPath (Join-Path $installRoot $Launch))) {throw 'Invalid portable installation'}
    $entries=@(Get-Content -LiteralPath $Plan -Raw | ConvertFrom-Json)
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
    $deadline=(Get-Date).AddSeconds(120)
    while($ParentPid -gt 0 -and (Get-Process -Id $ParentPid -ErrorAction SilentlyContinue)){if((Get-Date)-gt $deadline){throw 'Player did not exit'};Start-Sleep -Milliseconds 250}
    if(Get-Process -Name 'vs-player','VSRenderer' -ErrorAction SilentlyContinue | Where-Object {$_.Path -and (Split-Path -Parent $_.Path) -eq $installRoot}) {throw 'Close the other Renderer/Player window before installing'}
    foreach($entry in $entries){
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
    'Components installed; previous files: '+$backupRoot | Set-Content -LiteralPath $log -Encoding utf8
    if(!$NoLaunch){Start-Process -FilePath (Join-Path $installRoot $Launch) -WorkingDirectory $installRoot}
    exit 0
}catch{
    for($index=$changes.Count-1;$index -ge 0;$index--){$change=$changes[$index];if($change.Existed){Copy-Item -LiteralPath $change.Backup -Destination $change.Destination -Force}else{if(Test-Path -LiteralPath $change.Destination){Remove-Item -LiteralPath $change.Destination -Force}}}
    $_ | Out-String | Set-Content -LiteralPath $log -Encoding utf8
    exit 1
}
