param(
    [Parameter(Mandatory=$true)][string]$NativeRoot,
    [Parameter(Mandatory=$true)][string]$LibiamfRoot,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [string]$MSBuild=''
)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$NativeRoot=(Resolve-Path -LiteralPath $NativeRoot).Path
$LibiamfRoot=(Resolve-Path -LiteralPath $LibiamfRoot).Path
# NativeRoot must already have the pinned API20 VS Renderer patches/dependencies.
$patch=Join-Path $projectRoot 'patches/3fp-api20-anime4k-fast.patch'
$savedPreference=$ErrorActionPreference
$ErrorActionPreference='Continue'
try { & git -C $NativeRoot apply --reverse --check $patch 2>$null; $applied=$LASTEXITCODE -eq 0 }
finally { $ErrorActionPreference=$savedPreference }
if(!$applied){
    & git -C $NativeRoot apply --check $patch
    if($LASTEXITCODE -ne 0){throw 'Prepare the patched API20 Native checkout before adding Anime4K Fast.'}
    & git -C $NativeRoot apply $patch
    if($LASTEXITCODE -ne 0){throw 'Anime4K Fast patch failed.'}
}
& python (Join-Path $PSScriptRoot 'convert-anime4k-fast.py')
if($LASTEXITCODE -ne 0){throw 'Anime4K Fast shader conversion failed.'}
Copy-Item -LiteralPath (Join-Path $projectRoot 'src/render/Anime4KFast.h'),(Join-Path $projectRoot 'build/anime4k-native/shaders/Anime4KFastShaders.h') -Destination (Join-Path $NativeRoot 'FFF.Native/3FP/Render') -Force
if(!$MSBuild){
    $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $MSBuild=(& $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild/**/Bin/MSBuild.exe' | Select-Object -First 1)
}
& $MSBuild "$NativeRoot/FFF.Native/FFF.Native.vcxproj" /t:Build /m:6 /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v145 "/p:LibiamfRoot=$LibiamfRoot" /p:FFF_FORCE_LIBIAMF=true /v:minimal /nologo
if($LASTEXITCODE -ne 0){throw 'Anime4K Fast Native build failed.'}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
Copy-Item -LiteralPath "$NativeRoot/FFF.Native/x64/Release/FFF.Native.dll" -Destination $OutputDirectory -Force
