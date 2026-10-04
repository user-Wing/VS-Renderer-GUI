param(
    [string]$ProcessName = 'perf-3fp',
    [int]$TargetProcessId = 0,
    [Parameter(Mandatory = $true)][string]$OutputCsv,
    [int]$Seconds = 65
)
$ErrorActionPreference = 'Stop'
$previousCpu = 0.0
$timer = [Diagnostics.Stopwatch]::StartNew()
$previousTime = 0.0
$target = if ($TargetProcessId) { Get-Process -Id $TargetProcessId -ErrorAction Stop } else { Get-Process -Name $ProcessName -ErrorAction Stop | Select-Object -First 1 }
$processId = $target.Id
for ($sample = 0; $sample -lt $Seconds; $sample++) {
    $process = Get-Process -Id $processId -ErrorAction SilentlyContinue
    if (-not $process) { break }
    $counters = Get-Counter '\GPU Engine(*)\Utilization Percentage', '\GPU Process Memory(*)\Dedicated Usage', '\GPU Process Memory(*)\Shared Usage' -ErrorAction SilentlyContinue
    $process.Refresh()
    $now = $timer.Elapsed.TotalSeconds
    $cpu = ($process.CPU - $previousCpu) / ($now - $previousTime) * 100.0 / [Environment]::ProcessorCount
    $previousCpu = $process.CPU
    $previousTime = $now
    $io = Get-CimInstance Win32_PerfFormattedData_PerfProc_Process -Filter "IDProcess=$processId"
    foreach ($counter in $counters.CounterSamples) {
        if ($counter.InstanceName -notlike "pid_${processId}_*") { continue }
        if ($counter.Status -ne 0) { continue }
        [pscustomobject]@{
            WallSeconds = $now
            ProcessId = $processId
            CpuPercent = $cpu
            WorkingSetBytes = $process.WorkingSet64
            IoReadBytesPerSecond = $io.IOReadBytesPersec
            Counter = $counter.Path
            Value = $counter.CookedValue
        } | Export-Csv -LiteralPath $OutputCsv -NoTypeInformation -Append -Encoding utf8
    }
}
