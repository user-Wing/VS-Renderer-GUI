# Dot-source this file before building VSPlayer / vsr_player_tests.
$root = Split-Path $PSScriptRoot -Parent
$env:QT_ROOT = 'C:\Qt\6.10.2\mingw_64'
$env:PYTHONPATH = Join-Path $root 'build\setup\python-packages'
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;$root\build\setup\python-packages\cmake\data\bin;$root\build\setup\python-packages\bin;" + $env:PATH
