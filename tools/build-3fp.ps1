param(
    [Parameter(Mandatory = $true)]
    [string]$FffProject,
    [string]$OutputDirectory = "",
    [ValidateSet('', 'v143', 'v145')]
    [string]$PlatformToolset = ''
)

$ErrorActionPreference = "Stop"
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$source = (Resolve-Path $FffProject).Path
$nativeProject = Join-Path $source "FFF.Native\FFF.Native.vcxproj"
if (-not (Test-Path $nativeProject)) { throw "FFF.Native.vcxproj not found under $source" }

function Test-AppliedPatch([string]$Path) {
    # Windows PowerShell turns git's expected negative check into an error record.
    $ErrorActionPreference = 'Continue'
    & git -C $source apply --reverse --check $Path 2>$null
    return $LASTEXITCODE -eq 0
}
$performancePatch = Join-Path $projectRoot 'patches/3fp-present-performance.patch'
$jincPatch = Join-Path $projectRoot 'patches/3fp-jinc-upstream-1.0.5.patch'
$softwareFramePatch = Join-Path $projectRoot 'patches/3fp-software-frame-upload.patch'
$softwareFrameApplied = (Test-Path $softwareFramePatch) -and (Test-AppliedPatch $softwareFramePatch)
$softwareDecodePatch = Join-Path $projectRoot 'patches/3fp-software-decode-pipeline.patch'
# The pipeline patch overlaps the earlier session/queue patch contexts.
$softwareDecodeApplied = (Test-Path $softwareDecodePatch) -and (Test-AppliedPatch $softwareDecodePatch)
$jincApplied = $softwareFrameApplied -or ((Test-Path $jincPatch) -and (Test-AppliedPatch $jincPatch))
$performanceApplied = $jincApplied -or ((Test-Path $performancePatch) -and (Test-AppliedPatch $performancePatch))
if (-not $performanceApplied) {

# The final audio patch overlaps older rate hunks. Its reverse check identifies
# an already fully patched native checkout without reapplying those hunks.
$audioPatch = Join-Path $projectRoot "patches\3fp-player-audio-effects.patch"
if (-not (Test-AppliedPatch $audioPatch)) {
    foreach ($patchName in @("3fp-vsrenderer-extensions.patch", "3fp-resize-flags.patch", "3fp-performance-chroma.patch", "3fp-player-rate.patch", "3fp-player-output.patch", "3fp-network-subtitles.patch", "3fp-native-scaling.patch", "3fp-player-audio-effects.patch")) {
        $patch = Join-Path $projectRoot "patches\$patchName"
        if (-not (Test-AppliedPatch $patch)) {
            & git -C $source apply --check $patch
            if ($LASTEXITCODE -ne 0) { throw "3FP patch does not apply cleanly: $patchName" }
            & git -C $source apply $patch
            if ($LASTEXITCODE -ne 0) { throw "3FP patch failed: $patchName" }
        }
    }
}

$lanczosPatch = Join-Path $projectRoot 'patches/3fp-lanczos4.patch'
$colorPatch = Join-Path $projectRoot 'patches/3fp-color-management.patch'
$colorApplied = Test-AppliedPatch $colorPatch
if (-not $colorApplied) {
    if (-not (Test-AppliedPatch $lanczosPatch)) {
        & git -C $source apply --check $lanczosPatch
        if ($LASTEXITCODE -ne 0) { throw 'Lanczos4 patch does not apply cleanly.' }
        & git -C $source apply $lanczosPatch
        if ($LASTEXITCODE -ne 0) { throw 'Lanczos4 patch failed.' }
    }
    & git -C $source apply --check $colorPatch
    if ($LASTEXITCODE -ne 0) { throw 'Color management patch does not apply cleanly.' }
    & git -C $source apply $colorPatch
    if ($LASTEXITCODE -ne 0) { throw 'Color management patch failed.' }
}
$subtitlePatch = Join-Path $projectRoot 'patches/3fp-streaming-text-subtitles.patch'
if (-not (Test-AppliedPatch $subtitlePatch)) {
    & git -C $source apply --check $subtitlePatch
    if ($LASTEXITCODE -ne 0) { throw 'Streaming subtitle patch does not apply cleanly.' }
    & git -C $source apply $subtitlePatch
    if ($LASTEXITCODE -ne 0) { throw 'Streaming subtitle patch failed.' }
}
if (Test-Path $performancePatch) {
    & git -C $source apply --check $performancePatch
    if ($LASTEXITCODE -ne 0) { throw 'Present performance patch does not apply cleanly.' }
    & git -C $source apply $performancePatch
    if ($LASTEXITCODE -ne 0) { throw 'Present performance patch failed.' }
}
}
if ((Test-Path $jincPatch) -and -not $jincApplied) {
    & git -C $source apply --check $jincPatch
    if ($LASTEXITCODE -ne 0) { throw 'Jinc/upstream compatibility patch does not apply cleanly.' }
    & git -C $source apply $jincPatch
    if ($LASTEXITCODE -ne 0) { throw 'Jinc/upstream compatibility patch failed.' }
}
$clockPatch = Join-Path $projectRoot 'patches/3fp-vs-audio-clock.patch'
if (-not $softwareDecodeApplied -and -not (Test-AppliedPatch $clockPatch)) {
    & git -C $source apply --check $clockPatch
    if ($LASTEXITCODE -ne 0) { throw 'VS audio-clock patch does not apply cleanly.' }
    & git -C $source apply $clockPatch
    if ($LASTEXITCODE -ne 0) { throw 'VS audio-clock patch failed.' }
}
$blurayPatch = Join-Path $projectRoot 'patches/3fp-bluray-input.patch'
$bdPlaybackPatch = Join-Path $projectRoot 'patches/3fp-bd-subtitles-probe.patch'
# The BD probe hunk overlaps the older input patch's context.
if (-not $softwareDecodeApplied -and -not (Test-AppliedPatch $bdPlaybackPatch) -and -not (Test-AppliedPatch $blurayPatch)) {
    & git -C $source apply --check $blurayPatch
    if ($LASTEXITCODE -ne 0) { throw 'Blu-ray input patch does not apply cleanly.' }
    & git -C $source apply $blurayPatch
    if ($LASTEXITCODE -ne 0) { throw 'Blu-ray input patch failed.' }
}
$av1ThreadsPatch = Join-Path $projectRoot 'patches/3fp-av1-software-threads.patch'
if (-not $softwareDecodeApplied -and -not (Test-AppliedPatch $av1ThreadsPatch)) {
    & git -C $source apply --check $av1ThreadsPatch
    if ($LASTEXITCODE -ne 0) { throw 'AV1 software threads patch does not apply cleanly.' }
    & git -C $source apply $av1ThreadsPatch
    if ($LASTEXITCODE -ne 0) { throw 'AV1 software threads patch failed.' }
}
if (-not $softwareFrameApplied) {
    & git -C $source apply --check $softwareFramePatch
    if ($LASTEXITCODE -ne 0) { throw 'Software frame upload patch does not apply cleanly.' }
    & git -C $source apply $softwareFramePatch
    if ($LASTEXITCODE -ne 0) { throw 'Software frame upload patch failed.' }
}
if (-not $softwareDecodeApplied -and -not (Test-AppliedPatch $bdPlaybackPatch)) {
    & git -C $source apply --check $bdPlaybackPatch
    if ($LASTEXITCODE -ne 0) { throw 'BD subtitle/probe patch does not apply cleanly.' }
    & git -C $source apply $bdPlaybackPatch
    if ($LASTEXITCODE -ne 0) { throw 'BD subtitle/probe patch failed.' }
}
$concatSeekPatch = Join-Path $projectRoot 'patches/3fp-concat-seek.patch'
if (-not $softwareDecodeApplied -and -not (Test-AppliedPatch $concatSeekPatch)) {
    & git -C $source apply --check $concatSeekPatch
    if ($LASTEXITCODE -ne 0) { throw 'Concat seek patch does not apply cleanly.' }
    & git -C $source apply $concatSeekPatch
    if ($LASTEXITCODE -ne 0) { throw 'Concat seek patch failed.' }
}
$hardwareQueuePatch = Join-Path $projectRoot 'patches/3fp-hardware-video-queue.patch'
if (-not $softwareDecodeApplied -and -not (Test-AppliedPatch $hardwareQueuePatch)) {
    & git -C $source apply --check $hardwareQueuePatch
    if ($LASTEXITCODE -ne 0) { throw 'Hardware video queue patch does not apply cleanly.' }
    & git -C $source apply $hardwareQueuePatch
    if ($LASTEXITCODE -ne 0) { throw 'Hardware video queue patch failed.' }
}
if (-not $softwareDecodeApplied) {
    & git -C $source apply --check $softwareDecodePatch
    if ($LASTEXITCODE -ne 0) { throw 'Software decode pipeline patch does not apply cleanly.' }
    & git -C $source apply $softwareDecodePatch
    if ($LASTEXITCODE -ne 0) { throw 'Software decode pipeline patch failed.' }
}
$audioMetersPatch = Join-Path $projectRoot 'patches/3fp-audio-input-meters.patch'
if (-not (Test-AppliedPatch $audioMetersPatch)) {
    & git -C $source apply --check $audioMetersPatch
    if ($LASTEXITCODE -ne 0) { throw 'Audio input meters patch does not apply cleanly.' }
    & git -C $source apply $audioMetersPatch
    if ($LASTEXITCODE -ne 0) { throw 'Audio input meters patch failed.' }
}
$hdrReadbackPatch = Join-Path $projectRoot 'patches/3fp-hdr-readback.patch'
if (-not (Test-AppliedPatch $hdrReadbackPatch)) {
    & git -C $source apply --check $hdrReadbackPatch
    if ($LASTEXITCODE -ne 0) { throw 'HDR readback patch does not apply cleanly.' }
    & git -C $source apply $hdrReadbackPatch
    if ($LASTEXITCODE -ne 0) { throw 'HDR readback patch failed.' }
}
Copy-Item -LiteralPath (Join-Path $projectRoot 'src/color/ColorBridge.h'), (Join-Path $projectRoot 'src/color/NativeColorEngine.h') -Destination (Join-Path $source 'FFF.Native/3FP/Render') -Force
$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "vswhere.exe not found." }
$msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find "MSBuild\Current\Bin\MSBuild.exe" | Select-Object -First 1
if (-not $msbuild) { throw "MSBuild.exe not found." }

$buildArguments = @($nativeProject, '/m', '/p:Configuration=Release', '/p:Platform=x64', '/v:minimal')
if ($PlatformToolset) { $buildArguments += "/p:PlatformToolset=$PlatformToolset" }
& $msbuild @buildArguments
if ($LASTEXITCODE -ne 0) { throw "FFF.Native build failed." }

$binary = Join-Path $source "FFF.Native\x64\Release\FFF.Native.dll"
if (-not (Test-Path $binary)) { throw "Built FFF.Native.dll not found." }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $projectRoot "build\mingw-debug" }
$resolvedOutput = [System.IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $resolvedOutput | Out-Null
Copy-Item $binary -Destination (Join-Path $resolvedOutput "FFF.Native.dll") -Force
Write-Host "Patched 3FP ready: $(Join-Path $resolvedOutput 'FFF.Native.dll')"
