param([int]$Seconds = 60)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$media = 'F:\Media\4K电影+电视剧\Chou.Kaguya-hime.the.Movie 超时空辉夜姬'
$env:VSR_3FP_DLL = Join-Path $root 'build/perf-player/FFF.Native.dll'
$env:VSR_VSSCRIPT_DLL = Join-Path $root 'build/perf-player/runtime/python/vsscript.dll'
$env:VSR_PERF_SECONDS = $Seconds
$cases = @(
    @('4k8', 'Chou.Kaguya-hime.the.Movie.2160p.48F.AV1.HDR-Fixed.mkv'),
    @('4k10', 'Chou-Kaguya 2160p48F AV1 Real-ESRGAN Ver.mkv'),
    @('8k10', 'Chou Kaguya 4320p48F-AV1 RealESRGAN 8k收藏版.mkv')
)
foreach ($case in $cases) {
    foreach ($trial in 1..3) {
        $prefix = Join-Path $root "build/performance/final-gui-$($case[0])-r$trial"
        "GUI $($case[0]) trial $trial" | Set-Content (Join-Path $root 'build/performance/run-progress.txt')
        $env:VSR_PERF_SOURCE = Join-Path $media $case[1]
        $env:VSR_PERF_CSV = "$prefix.csv"
        $process = Start-Process -FilePath (Join-Path $root 'build/perf-player/vsr_player_tests.exe') -ArgumentList @('directNativePerformance', '-o', "$prefix-tests.txt,txt") -WindowStyle Hidden -PassThru
        $job = Start-Job -ScriptBlock { param($script, $processId, $output) & $script -TargetProcessId $processId -OutputCsv $output -Seconds 140 } -ArgumentList (Join-Path $root 'tools/perf-system.ps1'), $process.Id, "$prefix-system.csv"
        $process.WaitForExit()
        $process.Refresh()
        Receive-Job -Job $job -Wait | Out-Null
        Remove-Job -Job $job
        if ($process.ExitCode -ne 0) { throw "GUI playback failed: $prefix" }
        Write-Output "Completed GUI $($case[0]) trial $trial"
    }
}
'GUI matrix complete' | Set-Content (Join-Path $root 'build/performance/run-progress.txt')
