param(
    [Parameter(Mandatory=$true)][string]$NativeRoot,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [string]$MSBuild='C:\BuildTools\2026\MSBuild\Current\Bin\MSBuild.exe'
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$patch=Join-Path $root 'patches\3fp-api18-vsrenderer-1.0.10.patch'
$savedErrorPreference=$ErrorActionPreference
$ErrorActionPreference='Continue'
try {
    & git -C $NativeRoot apply --reverse --check $patch 2>$null
    $alreadyApplied=$LASTEXITCODE -eq 0
} finally {$ErrorActionPreference=$savedErrorPreference}
if(!$alreadyApplied){
    $legacy=Join-Path $root 'patches\3fp-api18-vsrenderer-1.0.9.patch'
    $ErrorActionPreference='Continue'
    try { & git -C $NativeRoot apply --reverse --check $legacy 2>$null; $legacyApplied=$LASTEXITCODE -eq 0 }
    finally { $ErrorActionPreference=$savedErrorPreference }
    if($legacyApplied){$patch=Join-Path $root 'patches\3fp-api18-vsrenderer-1.0.10-update.patch'}
    & git -C $NativeRoot apply --check $patch
    if($LASTEXITCODE -ne 0){throw 'API 18 extension patch conflicts with the native checkout.'}
    & git -C $NativeRoot apply $patch
    if($LASTEXITCODE -ne 0){throw 'API 18 extension patch failed.'}
}
& $MSBuild "$NativeRoot\FFF.Native\FFF.Native.vcxproj" /t:Build /m:6 /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v145 /v:minimal /nologo
if($LASTEXITCODE -ne 0){throw 'Native API 18 build failed.'}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
Copy-Item -LiteralPath "$NativeRoot\FFF.Native\x64\Release\FFF.Native.dll" -Destination (Join-Path $OutputDirectory 'FFF.Native.dll') -Force
