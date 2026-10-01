param(
    [Parameter(Mandatory = $true)]
    [string]$FffProject,
    [string]$OutputDirectory = ""
)

$ErrorActionPreference = "Stop"
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$source = (Resolve-Path $FffProject).Path
$nativeProject = Join-Path $source "FFF.Native\FFF.Native.vcxproj"
if (-not (Test-Path $nativeProject)) { throw "FFF.Native.vcxproj not found under $source" }

foreach ($patchName in @("3fp-vsrenderer-extensions.patch", "3fp-resize-flags.patch", "3fp-performance-chroma.patch", "3fp-player-rate.patch", "3fp-player-output.patch", "3fp-network-subtitles.patch", "3fp-native-scaling.patch")) {
    $patch = Join-Path $projectRoot "patches\$patchName"
    & git -C $source apply --reverse --check $patch 2>$null
    if ($LASTEXITCODE -ne 0) {
        & git -C $source apply --check $patch
        if ($LASTEXITCODE -ne 0) { throw "3FP patch does not apply cleanly." }
        & git -C $source apply $patch
        if ($LASTEXITCODE -ne 0) { throw "3FP patch failed." }
    }
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "vswhere.exe not found." }
$msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find "MSBuild\Current\Bin\MSBuild.exe" | Select-Object -First 1
if (-not $msbuild) { throw "MSBuild.exe not found." }

& $msbuild $nativeProject /m /p:Configuration=Release /p:Platform=x64 /v:minimal
if ($LASTEXITCODE -ne 0) { throw "FFF.Native build failed." }

$binary = Join-Path $source "FFF.Native\x64\Release\FFF.Native.dll"
if (-not (Test-Path $binary)) { throw "Built FFF.Native.dll not found." }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $projectRoot "build\mingw-debug" }
$resolvedOutput = [System.IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $resolvedOutput | Out-Null
Copy-Item $binary -Destination (Join-Path $resolvedOutput "FFF.Native.dll") -Force
Write-Host "Patched 3FP ready: $(Join-Path $resolvedOutput 'FFF.Native.dll')"
