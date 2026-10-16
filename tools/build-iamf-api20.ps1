param(
    [Parameter(Mandatory=$true)][string]$SourceRoot,
    [Parameter(Mandatory=$true)][string]$EigenRoot,
    [Parameter(Mandatory=$true)][string]$PffftRoot,
    [Parameter(Mandatory=$true)][string]$BuildDirectory,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [Parameter(Mandatory=$true)][string]$VcVars,
    [string]$CMake='cmake',
    [string]$Ninja='ninja'
)
$ErrorActionPreference='Stop'
$SourceRoot=(Resolve-Path -LiteralPath $SourceRoot).Path
$EigenRoot=(Resolve-Path -LiteralPath $EigenRoot).Path
$PffftRoot=(Resolve-Path -LiteralPath $PffftRoot).Path
$VcVars=(Resolve-Path -LiteralPath $VcVars).Path
$CMake=(Get-Command $CMake -ErrorAction Stop).Source
$Ninja=(Get-Command $Ninja -ErrorAction Stop).Source
$commit=(& git -C $SourceRoot rev-parse HEAD).Trim()
if($commit -ne 'b276f4371a8ae4f370a656e914f7b07e4cd5a2fa'){throw 'Use the pinned libiamf source revision from docs/native-api20-build.md.'}
$oar=Join-Path $SourceRoot 'code\dep_external\src\oar'
if((& git -C $oar rev-parse HEAD).Trim() -ne 'ca14b54381b6e8482a72dcc65c3adb17d6751db0'){throw 'Initialize the pinned OAR submodule first.'}
New-Item -ItemType Directory -Path $BuildDirectory -Force | Out-Null
$BuildDirectory=(Resolve-Path -LiteralPath $BuildDirectory).Path
$command=Join-Path $BuildDirectory 'build-iamf.cmd'
$configure='"{0}" -S "{1}\code" -B "{2}" -G Ninja "-DCMAKE_MAKE_PROGRAM={3}" -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL -DIAMF_BUILD_SHARED_LIB=OFF -DIAMF_ENABLE_BINAURALIZER=ON -DIAMF_TEST_TOOL=OFF -DCMAKE_POLICY_VERSION_MINIMUM=3.5 "-DFETCHCONTENT_SOURCE_DIR_EIGEN={4}" "-DFETCHCONTENT_SOURCE_DIR_PFFFT={5}"' -f $CMake,$SourceRoot,$BuildDirectory,$Ninja,$EigenRoot,$PffftRoot
@('@echo off',('call "{0}" x64 >nul' -f $VcVars),'if errorlevel 1 exit /b 1',$configure,'if errorlevel 1 exit /b 1',('"{0}" --build "{1}" --target iamf --parallel 6' -f $CMake,$BuildDirectory)) | Set-Content -LiteralPath $command -Encoding Default
& cmd.exe /d /c $command
if($LASTEXITCODE -ne 0){throw 'libiamf/OBR build failed.'}
$include=Join-Path $OutputDirectory 'include'
$oarInclude=Join-Path $OutputDirectory 'oar\include'
$lib=Join-Path $OutputDirectory 'lib\x64'
$absl=Join-Path $lib 'absl'
New-Item -ItemType Directory -Path $include,$oarInclude,$lib,$absl -Force | Out-Null
Get-ChildItem -LiteralPath "$SourceRoot\code\include" -File -Filter '*.h' | Copy-Item -Destination $include -Force
Get-ChildItem -LiteralPath "$oar\include" -File -Filter '*.h' | Copy-Item -Destination $oarInclude -Force
foreach($name in @('iamf','oar','obr','obr_capi','pffft')){
    $built=Get-ChildItem -LiteralPath $BuildDirectory -Recurse -File -Filter "$name.lib" | Select-Object -First 1
    if(!$built){throw "Missing build output: $name.lib"}
    Copy-Item -LiteralPath $built.FullName -Destination $lib -Force
}
foreach($name in @('FLAC','opus','fdk-aac')){Copy-Item -LiteralPath "$SourceRoot\code\dep_codecs\lib\$name.lib" -Destination $lib -Force}
Get-ChildItem -LiteralPath "$BuildDirectory\_deps\absl-build" -Recurse -File -Filter '*.lib' | Copy-Item -Destination $absl -Force
