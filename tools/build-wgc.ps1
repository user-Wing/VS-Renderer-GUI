param([Parameter(Mandatory=$true)][string]$Source,[Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference='Stop'
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$installation=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$installation){throw 'WGC bridge requires MSVC C++ tools and Windows SDK C++/WinRT headers'}
$directory=Split-Path -Parent $Output
New-Item -ItemType Directory -Force -Path $directory | Out-Null
$batch=Join-Path $directory 'compile-wgc.cmd'
$vcvars=Join-Path $installation 'VC\Auxiliary\Build\vcvars64.bat'
@"
@call "$vcvars" >nul
@if errorlevel 1 exit /b %errorlevel%
@cl /nologo /std:c++20 /EHsc /O2 /MT /LD /DWIN32_LEAN_AND_MEAN /DNOMINMAX "$Source" /Fo"$directory\WgcBridge.obj" /link /OUT:"$Output" /IMPLIB:"$directory\vsr-wgc.lib" d3d11.lib dxgi.lib windowsapp.lib
"@ | Set-Content -Encoding ASCII -LiteralPath $batch
& $env:ComSpec /d /c $batch
if($LASTEXITCODE -ne 0){throw "WGC bridge compilation failed: $LASTEXITCODE"}
