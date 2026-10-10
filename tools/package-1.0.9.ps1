param(
    [ValidateSet("Full","Lite","Both")][string]$Edition = "Both",
    [string]$NativeRoot = "C:\Private\FFF_Project-1.0.9",
    [string]$RuntimeDirectory = "C:\PortableSoft\VS-Renderer-GUI",
    [string]$QtDirectory = "C:\Qt\6.10.2\mingw_64",
    [string]$MingwDirectory = "C:\Qt\Tools\mingw1310_64\bin",
    [string]$CrtDirectory = "C:\BuildTools\2026\VC\Redist\MSVC\14.51.36231\x64\Microsoft.VC145.CRT",
    [string]$Dumpbin = "C:\BuildTools\2026\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe"
)
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$python = Join-Path $RuntimeDirectory "runtime\python\python.exe"
& $python "$PSScriptRoot\stage-editions-1.0.9.py" --root $root --runtime $RuntimeDirectory --native "$NativeRoot\FFF.Native\x64\Release\FFF.Native.dll" --native-dependencies "$NativeRoot\third_party\vcpkg_installed\x64-windows\bin" --qt $QtDirectory --mingw $MingwDirectory --dumpbin $Dumpbin --edition $Edition --crt $CrtDirectory
if ($LASTEXITCODE -ne 0) { throw "Edition packaging failed: $LASTEXITCODE" }
