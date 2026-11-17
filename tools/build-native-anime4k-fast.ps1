param(
    [Parameter(Mandatory=$true)][string]$NativeRoot,
    [Parameter(Mandatory=$true)][string]$LibiamfRoot,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [string]$MSBuild='',
    [ValidateRange(1,4)][int]$BuildParallelism=2
)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$NativeRoot=(Resolve-Path -LiteralPath $NativeRoot).Path
$LibiamfRoot=(Resolve-Path -LiteralPath $LibiamfRoot).Path
# NativeRoot is prepared by build-native-api20.ps1.
& python (Join-Path $PSScriptRoot 'convert-anime4k-fast.py')
if($LASTEXITCODE -ne 0){throw 'Anime4K Fast shader conversion failed.'}
Copy-Item -LiteralPath (Join-Path $projectRoot 'src/render/Anime4KFast.h'),(Join-Path $projectRoot 'build/anime4k-native/shaders/Anime4KFastShaders.h') -Destination (Join-Path $NativeRoot 'FFF.Native/3FP/Render') -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'src/render/CustomShader.h'),(Join-Path $projectRoot 'src/render/ImageResample.h') -Destination (Join-Path $NativeRoot 'FFF.Native/3FP/Render') -Force
if(!$MSBuild){
    $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $MSBuild=(& $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild/**/Bin/MSBuild.exe' | Select-Object -First 1)
}
$buildProcess=[System.Diagnostics.Process]::GetCurrentProcess()
$previousPriority=$buildProcess.PriorityClass
try {
    $buildProcess.PriorityClass='BelowNormal'
    & $MSBuild "$NativeRoot/FFF.Native/FFF.Native.vcxproj" /t:Rebuild "/m:$BuildParallelism" /p:MultiProcessorCompilation=false /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v145 "/p:LibiamfRoot=$LibiamfRoot" /p:FFF_FORCE_LIBIAMF=true /p:SecurityRegressionTests=false /v:minimal /nologo
} finally {
    $buildProcess.PriorityClass=$previousPriority
}
if($LASTEXITCODE -ne 0){throw 'Anime4K Fast Native build failed.'}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
Copy-Item -LiteralPath "$NativeRoot/FFF.Native/x64/Release/FFF.Native.dll" -Destination $OutputDirectory -Force
