param(
    [Parameter(Mandatory=$true)][string]$NativeRoot,
    [Parameter(Mandatory=$true)][string]$DependenciesRoot,
    [Parameter(Mandatory=$true)][string]$LibiamfRoot,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [string]$MSBuild=''
)
$ErrorActionPreference='Stop'
$NativeRoot=(Resolve-Path -LiteralPath $NativeRoot).Path
$DependenciesRoot=(Resolve-Path -LiteralPath $DependenciesRoot).Path
$LibiamfRoot=(Resolve-Path -LiteralPath $LibiamfRoot).Path
$checkout=(& git -C $NativeRoot rev-parse --show-toplevel).Trim()
if($LASTEXITCODE -ne 0 -or (Resolve-Path -LiteralPath $checkout).Path -ne $NativeRoot){throw 'NativeRoot must be a separate FFF_Project Git checkout.'}
$commit=(& git -C $NativeRoot rev-parse HEAD).Trim()
if($commit -ne '72af915f8bf2567155c1f87851a6fa263f73b14f'){throw 'Checkout the pinned upstream commit 72af915f8bf2567155c1f87851a6fa263f73b14f first.'}
$patch=Join-Path (Split-Path $PSScriptRoot -Parent) 'patches/3fp-vsp-1.1.0.patch'
$ErrorActionPreference='Continue'
& git -C $NativeRoot apply --reverse --check $patch 2>$null
$applied=$LASTEXITCODE -eq 0
$ErrorActionPreference='Stop'
if(!$applied){
    & git -C $NativeRoot apply --check $patch
    if($LASTEXITCODE -ne 0){throw 'VSP 1.1.0 patch conflicts with the native checkout.'}
    & git -C $NativeRoot apply $patch
    if($LASTEXITCODE -ne 0){throw 'VSP 1.1.0 patch failed.'}
}
$networkPatch=Join-Path (Split-Path $PSScriptRoot -Parent) 'patches/3fp-vsp-1.1.0-network-proxy.patch'
$ErrorActionPreference='Continue'
& git -C $NativeRoot apply --reverse --check $networkPatch 2>$null
$networkApplied=$LASTEXITCODE -eq 0
$ErrorActionPreference='Stop'
if(!$networkApplied){
    & git -C $NativeRoot apply --check $networkPatch
    if($LASTEXITCODE -ne 0){throw 'Loopback proxy patch conflicts with the native checkout.'}
    & git -C $NativeRoot apply $networkPatch
    if($LASTEXITCODE -ne 0){throw 'Loopback proxy patch failed.'}
}
$projectRoot=Split-Path $PSScriptRoot -Parent
Copy-Item -LiteralPath (Join-Path $projectRoot 'src/backend/SourcePixelProbe.h') -Destination (Join-Path $NativeRoot 'FFF.Native/3FP/Core/SourcePixelProbe.h') -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'src/color/ColorBridge.h'), (Join-Path $projectRoot 'src/color/NativeColorEngine.h') -Destination (Join-Path $NativeRoot 'FFF.Native/3FP/Render') -Force
New-Item -ItemType Directory -Path (Join-Path $NativeRoot 'third_party') -Force | Out-Null
foreach($name in @('ffmpeg','vcpkg_installed')){
    $source=Join-Path $DependenciesRoot $name
    if(!(Test-Path -LiteralPath $source -PathType Container)){throw "Missing dependency: $source"}
    $target=Join-Path $NativeRoot "third_party\$name"
    if(!(Test-Path -LiteralPath $target)){New-Item -ItemType Junction -Path $target -Target $source | Out-Null}
}
foreach($name in @('iamf','oar','obr','obr_capi','pffft','FLAC','opus','fdk-aac')){
    if(!(Test-Path -LiteralPath (Join-Path $LibiamfRoot "lib\x64\$name.lib"))){throw "Missing IAMF dependency: $name.lib"}
}
if(!$MSBuild){
    $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $MSBuild=(& $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1)
}
if(!$MSBuild -or !(Test-Path -LiteralPath $MSBuild)){throw 'MSBuild with the v145 C++ toolset is required.'}
& (Join-Path $PSScriptRoot 'build-native-anime4k-fast.ps1') -NativeRoot $NativeRoot -LibiamfRoot $LibiamfRoot -OutputDirectory $OutputDirectory -MSBuild $MSBuild
