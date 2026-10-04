$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$media = 'F:\Media\4K电影+电视剧\Chou.Kaguya-hime.the.Movie 超时空辉夜姬'
$cases = @(
    @('4k8', 'Chou.Kaguya-hime.the.Movie.2160p.48F.AV1.HDR-Fixed.mkv'),
    @('4k10', 'Chou-Kaguya 2160p48F AV1 Real-ESRGAN Ver.mkv'),
    @('8k10', 'Chou Kaguya 4320p48F-AV1 RealESRGAN 8k收藏版.mkv')
)
$env:VSR_PERF_SECONDS = '65'
foreach ($case in $cases) {
    $prefix = Join-Path $root "build/performance/final-mpv-$($case[0])-r1"
    "mpv $($case[0]) reference" | Set-Content (Join-Path $root 'build/performance/run-progress.txt')
    $env:VSR_PERF_CSV = "$prefix.csv"
    $arguments = @('"C:\PortableSoft\mpv\mpv.exe"', ('"'+(Join-Path $media $case[1])+'"'), 3840, 2160)
    $process = Start-Process -FilePath (Join-Path $root 'build/performance/perf-mpv-host.exe') -ArgumentList $arguments -WindowStyle Hidden -RedirectStandardOutput "$prefix.log" -RedirectStandardError "$prefix-pid.log" -PassThru
    $childId = 0
    for ($attempt = 0; $attempt -lt 40; $attempt++) {
        $line = Get-Content "$prefix-pid.log" -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($line -match '^MPV_PID=(\d+)$') { $childId = [int]$Matches[1]; break }
        Start-Sleep -Milliseconds 100
    }
    if (-not $childId) { throw 'mpv child process did not start.' }
    $job = Start-Job -ScriptBlock { param($script, $processId, $output) & $script -TargetProcessId $processId -OutputCsv $output -Seconds 140 } -ArgumentList (Join-Path $root 'tools/perf-system.ps1'), $childId, "$prefix-system.csv"
    $process.WaitForExit()
    $process.Refresh()
    Receive-Job -Job $job -Wait | Out-Null
    Remove-Job -Job $job
    if ($process.ExitCode -ne 0) { throw "mpv reference failed: $prefix" }
    Write-Output "Completed mpv $($case[0]) reference"
}
'mpv reference complete' | Set-Content (Join-Path $root 'build/performance/run-progress.txt')
