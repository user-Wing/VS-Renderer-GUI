param(
    [string]$NativeRoot = 'C:\Private\FFF_Project-1.0.9',
    [string]$RuntimeDirectory = 'C:\PortableSoft\VS-Renderer-GUI',
    [string]$QtDirectory = 'C:\Qt\6.10.2\mingw_64',
    [string]$MingwDirectory = 'C:\Qt\Tools\mingw1310_64\bin',
    [string]$MSBuild = 'C:\BuildTools\2026\MSBuild\Current\Bin\MSBuild.exe',
    [string]$CMake = 'C:\Private\VS-Renderer-GUI-dev\build\setup\python-packages\cmake\data\bin\cmake.exe',
    [string]$Dumpbin = 'C:\BuildTools\2026\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe',
    [string]$CrtDirectory = 'C:\BuildTools\2026\VC\Redist\MSVC\14.51.36231\x64\Microsoft.VC145.CRT',
    [string]$Ninja = 'C:\Private\VS-Renderer-GUI-dev\build\setup\python-packages\bin\ninja.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$build = Join-Path $root 'build\mingw-release'
$env:PATH = "$MingwDirectory;$QtDirectory\bin;$env:PATH"
$env:VSR_BUNDLED_VS_RUNTIME = Join-Path $RuntimeDirectory 'runtime\python'
& "$PSScriptRoot\build-native-api18.ps1" -NativeRoot $NativeRoot -OutputDirectory $build -MSBuild $MSBuild
& $CMake -S $root -B $build -G Ninja "-DCMAKE_MAKE_PROGRAM=$Ninja" "-DCMAKE_PREFIX_PATH=$QtDirectory" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
if ($LASTEXITCODE -ne 0) { throw 'Qt configuration failed.' }
& $CMake --build $build --target VSRenderer VSPlayer VSPlayerLite VSCap --parallel 6
if ($LASTEXITCODE -ne 0) { throw 'Qt build failed.' }
Copy-Item -LiteralPath "$NativeRoot\FFF.Native\x64\Release\FFF.Native.dll" -Destination "$build\FFF.Native.dll" -Force
& "$PSScriptRoot\package-1.0.9.ps1" -Edition Both -NativeRoot $NativeRoot -RuntimeDirectory $RuntimeDirectory -QtDirectory $QtDirectory -MingwDirectory $MingwDirectory -Dumpbin $Dumpbin -CrtDirectory $CrtDirectory
