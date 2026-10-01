param(
    [string]$ProgramDirectory = 'dist/VS-Renderer-GUI-1.0.2-windows-x64',
    [string]$SevenZip = 'C:/PortableSoft/7-Zip-Zstandard/7z.exe'
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$program = (Resolve-Path -LiteralPath $ProgramDirectory).Path
$stage = Join-Path $root 'build/release-stage'
$payload = Join-Path $stage 'VS-Renderer-GUI-1.0.2-windows-x64'
$archive = Join-Path $root 'dist/1.0.2.7z'
if (Test-Path -LiteralPath $payload) { throw 'Release staging directory already exists; inspect it before creating another package.' }
if (Test-Path -LiteralPath $archive) { throw 'Archive already exists; do not silently update an existing archive.' }
New-Item -ItemType Directory -Path $payload -Force | Out-Null
# Construct a clean distribution, retaining the live installation's settings and user files.
& robocopy.exe $program $payload /E /COPY:DAT /DCOPY:DAT /R:2 /W:1 /NFL /NDL /NJH /NJS /NP /XD cache shader-cache screenshots vpy __pycache__ /XF '*.ini' '*.lwi' '*.ffindex' '*.pyc' '*.pdb' '*test*.exe' Qt6Test.dll '*.log' '*.before-bitdepth-fix' settings.bin | Out-Null
if ($LASTEXITCODE -ge 8) { throw 'Clean payload copy failed' }
$builtin = Join-Path $payload 'vpy/builtin'
New-Item -ItemType Directory -Path $builtin -Force | Out-Null
foreach ($name in @('Anime.vpy','Realistic.vpy')) {
    Copy-Item -LiteralPath (Join-Path $program "vpy/builtin/$name") -Destination $builtin -Force
}
# These scripts register/reset the third-party renderer globally; portable COM loading does not use them.
$madvr = Join-Path $payload 'madVR09217'
foreach ($name in @('activate debug mode.bat','install.bat','uninstall.bat','restore default settings.bat','enable nvidia 3d.reg')) {
    $path = Join-Path $madvr $name
    if (Test-Path -LiteralPath $path -PathType Leaf) { Remove-Item -LiteralPath $path }
}
@{schema=1;version='1.0.2';platform='windows-x64'} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $payload 'release.json') -Encoding UTF8
$files = Get-ChildItem -LiteralPath $payload -Recurse -File | Where-Object { $_.Name -ne 'local-resource-sha256.json' }
$manifest = @{}
foreach ($file in $files) { $manifest[$file.FullName.Substring($payload.Length+1).Replace('\','/')] = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLower() }
$manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $payload 'local-resource-sha256.json') -Encoding UTF8
Push-Location $stage
try {
    & $SevenZip a -t7z $archive 'VS-Renderer-GUI-1.0.2-windows-x64' -m0=zstd -mx=22 -ms=on -mmt=2 -bb0
    if ($LASTEXITCODE -ne 0) { throw 'Compression failed' }
} finally { Pop-Location }
& $SevenZip t $archive
if ($LASTEXITCODE -ne 0) { throw 'Archive integrity test failed' }
$digest = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLower()
[IO.File]::WriteAllText("$archive.sha256", "$digest  1.0.2.7z`n", [Text.UTF8Encoding]::new($false))
Write-Host "Clean Zstandard Ultra package: $archive"
Write-Host "SHA-256: $digest"
