param(
    [string]$QtRoot='C:/Private/Soft/QT/6.10.2/mingw_64',
    [string]$OutputDirectory='build/mingw-release/imageformats'
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$source=Join-Path $root '.deps/qtimageformats110'
$build=Join-Path $root 'build/qtimageformats110'
$pin='076fb82c55321e42beeae62d9e3ca8c4bb71439c'
if(!(Test-Path -LiteralPath $source)){
    & git clone --depth 1 --branch v6.10.2 https://code.qt.io/qt/qtimageformats.git $source
    if($LASTEXITCODE){throw 'Qt ImageFormats checkout failed'}
}
if((& git -C $source rev-parse HEAD).Trim() -ne $pin){throw 'Unexpected Qt ImageFormats revision'}
$project=Join-Path $source 'CMakeLists.txt'
$text=[IO.File]::ReadAllText($project)
$rule='set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff <SOURCE> <OBJECT>")'
if(!$text.Contains($rule)){
    # windres forwards quoted include paths with spaces incorrectly; this RC uses no includes.
    [IO.File]::WriteAllText($project,$text.Replace('qt_build_repo()',"$rule`nqt_build_repo()"))
}
& cmake -S $source -B $build -G Ninja "-DCMAKE_PREFIX_PATH=$QtRoot" -DCMAKE_BUILD_TYPE=Release -DQT_BUILD_TESTS=OFF -DQT_BUILD_EXAMPLES=OFF -DQT_FEATURE_system_tiff=OFF
if($LASTEXITCODE){throw 'Qt ImageFormats configure failed'}
& cmake --build $build --target QTiffPlugin -j8
if($LASTEXITCODE){throw 'Qt TIFF plugin build failed'}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $build 'plugins/imageformats/qtiff.dll') -Destination $OutputDirectory -Force
