$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$media = 'F:\Media\4K电影+电视剧\Chou.Kaguya-hime.the.Movie 超时空辉夜姬'
$cases = @(
    @('1080-native', '辉夜姬 V31080-48-HEVC.mkv', 1920, 1080),
    @('1080-to4k', '辉夜姬 V31080-48-HEVC.mkv', 3840, 2160),
    @('4k8', 'Chou.Kaguya-hime.the.Movie.2160p.48F.AV1.HDR-Fixed.mkv', 3840, 2160),
    @('4k10', 'Chou-Kaguya 2160p48F AV1 Real-ESRGAN Ver.mkv', 3840, 2160),
    @('8k10', 'Chou Kaguya 4320p48F-AV1 RealESRGAN 8k收藏版.mkv', 3840, 2160)
)
foreach ($case in $cases) {
    foreach ($trial in 1..3) {
        $prefix = Join-Path $root "build/performance/final-native-$($case[0])-r$trial"
        "Starting $($case[0]) trial $trial" | Set-Content (Join-Path $root 'build/performance/run-progress.txt')
        $arguments = @(('"'+(Join-Path $root 'build/perf-player/FFF.Native.dll')+'"'), ('"'+(Join-Path $media $case[1])+'"'), $case[2], $case[3], 60, 519)
        $process = Start-Process -FilePath (Join-Path $root 'build/perf-player/perf-3fp.exe') -ArgumentList $arguments -WindowStyle Hidden -RedirectStandardOutput "$prefix.csv" -RedirectStandardError "$prefix.log" -PassThru
        $job = Start-Job -ScriptBlock { param($script, $processId, $output) & $script -TargetProcessId $processId -OutputCsv $output -Seconds 140 } -ArgumentList (Join-Path $root 'tools/perf-system.ps1'), $process.Id, "$prefix-system.csv"
        $process.WaitForExit()
        $process.Refresh()
        Receive-Job -Job $job -Wait | Out-Null
        Remove-Job -Job $job
        if ($process.ExitCode -ne 0) { throw "Playback failed: $prefix, exit $($process.ExitCode)" }
        Write-Output "Completed $($case[0]) trial $trial"
    }
}
'Native matrix complete' | Set-Content (Join-Path $root 'build/performance/run-progress.txt')
