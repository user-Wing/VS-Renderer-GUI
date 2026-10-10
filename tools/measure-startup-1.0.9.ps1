param([int]$Trials=3, [string]$BaselineDirectory='C:\PortableSoft\VS-Renderer-GUI', [string]$CsvPath='')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$env:PATH="$env:SystemRoot\System32;$env:SystemRoot"
$cases=@(
  @{Name='old-renderer';Exe=(Join-Path $BaselineDirectory 'VSRenderer.exe')},
  @{Name='old-player';Exe=(Join-Path $BaselineDirectory 'vs-player.exe')},
  @{Name='new-renderer';Exe=(Join-Path $root 'dist\Full\VSRenderer.exe')},
  @{Name='new-player';Exe=(Join-Path $root 'dist\Full\vs-player.exe')},
  @{Name='new-lite';Exe=(Join-Path $root 'dist\PlayerLite\vs-player.exe')}
)
$rows=@()
foreach($case in $cases){
  $file=$case.Name
  $exe=$case.Exe
  if(-not(Test-Path $exe)){Write-Output "MISSING $file";continue}
  for($i=1;$i -le $Trials;$i++){
    $watch=[System.Diagnostics.Stopwatch]::StartNew()
    $p=Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe) -PassThru
    $ready=$false
    while($watch.ElapsedMilliseconds -lt 10000){
      $p.Refresh()
      if($p.HasExited){break}
      if($p.MainWindowHandle -ne [IntPtr]::Zero){$ready=$true;break}
      Start-Sleep -Milliseconds 20
    }
    Write-Output ("STARTUP file={0} trial={1} visible_hwnd={2} elapsed_ms={3} pid={4} exited={5}" -f $file,$i,$ready,$watch.ElapsedMilliseconds,$p.Id,$p.HasExited)
    $rows += [pscustomobject]@{program=$file;trial=$i;visibleHwnd=$ready;elapsedMs=$watch.ElapsedMilliseconds;pid=$p.Id;exited=$p.HasExited;exe=$exe}
    if(!$p.HasExited){
      $p.CloseMainWindow()|Out-Null
      if(!$p.WaitForExit(3000)){Stop-Process -Id $p.Id -Force}
    }
    Start-Sleep -Milliseconds 250
  }
}
if($CsvPath){$rows | Export-Csv -LiteralPath $CsvPath -NoTypeInformation -Encoding UTF8}
