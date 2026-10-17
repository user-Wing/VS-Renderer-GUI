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
if($commit -ne 'ce195ebd06e9cbb8352a443f0c874cf569a60733'){throw 'Checkout the pinned API 20 commit ce195ebd06e9cbb8352a443f0c874cf569a60733 first.'}
$patch=Join-Path (Split-Path $PSScriptRoot -Parent) 'patches\3fp-api20-vsrenderer-1.0.10.patch'
$savedPreference=$ErrorActionPreference
$ErrorActionPreference='Continue'
try { & git -C $NativeRoot apply --reverse --check $patch 2>$null; $applied=$LASTEXITCODE -eq 0 }
finally { $ErrorActionPreference=$savedPreference }
if(!$applied){
    & git -C $NativeRoot apply --check $patch
    if($LASTEXITCODE -ne 0){throw 'API 20 patch conflicts with the native checkout.'}
    & git -C $NativeRoot apply $patch
    if($LASTEXITCODE -ne 0){throw 'API 20 patch failed.'}
}
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
& $MSBuild "$NativeRoot\FFF.Native\FFF.Native.vcxproj" /t:Build /m:6 /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v145 "/p:LibiamfRoot=$LibiamfRoot" /p:FFF_FORCE_LIBIAMF=true /v:minimal /nologo
if($LASTEXITCODE -ne 0){throw 'Native API 20 build failed.'}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$target=Join-Path $OutputDirectory 'FFF.Native.dll'
Copy-Item -LiteralPath "$NativeRoot\FFF.Native\x64\Release\FFF.Native.dll" -Destination "$target.new" -Force
Move-Item -LiteralPath "$target.new" -Destination $target -Force
