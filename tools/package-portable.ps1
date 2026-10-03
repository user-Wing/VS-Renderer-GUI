param(
    [string]$ProgramDirectory = 'dist/VS-Renderer-GUI-windows-x64',
    [string]$SevenZip = 'C:/PortableSoft/7-Zip-Zstandard/7z.exe',
    [string]$BuiltinDirectory = ''
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$program = (Resolve-Path -LiteralPath $ProgramDirectory).Path
$builtinSource = if ($BuiltinDirectory) { (Resolve-Path -LiteralPath $BuiltinDirectory).Path } else { Join-Path $program 'vpy/builtin' }
$version = [regex]::Match((Get-Content -LiteralPath (Join-Path $root 'CMakeLists.txt') -Raw), 'project\(VSRenderer VERSION ([0-9.]+)').Groups[1].Value
if (-not $version) { throw 'Project version not found' }
$stage = Join-Path $root "build/release-stage-$version"
# Released 1.0.2 clients require a versioned archive root; install target stays unchanged.
$archiveRoot = "VS-Renderer-GUI-$version-windows-x64"
$payload = Join-Path $stage $archiveRoot
$archive = Join-Path $root "dist/$version.7z"
if (Test-Path -LiteralPath $payload) { throw 'Release staging directory already exists; inspect it before creating another package.' }
if (Test-Path -LiteralPath $archive) { throw 'Archive already exists; do not silently update an existing archive.' }
New-Item -ItemType Directory -Path $payload -Force | Out-Null
# Construct a clean distribution, retaining the live installation's settings and user files.
& robocopy.exe $program $payload /E /COPY:DAT /DCOPY:DAT /R:2 /W:1 /NFL /NDL /NJH /NJS /NP /XD cache shader-cache screenshots vpy __pycache__ .git .deps temp logs /XF '*.ini' '*.lwi' '*.ffindex' '*.pyc' '*.pdb' '*test*.exe' Qt6Test.dll '*.log' '*.before-*' '*.tmp' '*.autosave' settings.bin | Out-Null
if ($LASTEXITCODE -ge 8) { throw 'Clean payload copy failed' }
$builtin = Join-Path $payload 'vpy/builtin'
New-Item -ItemType Directory -Path $builtin -Force | Out-Null
foreach ($name in @('Anime.vpy','Realistic.vpy','Anime-0-CNN-Enhanced.vpy','Anime-1-CNN.vpy','Anime-2-No-CNN-Enhanced.vpy','Anime-3-No-CNN.vpy','Anime-4-Jinc.vpy','Anime-5-D3D11.vpy','Interpolation-0-RIFE.vpy','Interpolation-1-RIFE-Half.vpy','Interpolation-2-MVTools-HQ.vpy','Interpolation-3-MVTools.vpy')) {
    Copy-Item -LiteralPath (Join-Path $builtinSource $name) -Destination $builtin -Force
}
# These scripts register/reset the third-party renderer globally; portable COM loading does not use them.
$madvr = Join-Path $payload 'madVR09217'
foreach ($name in @('activate debug mode.bat','install.bat','uninstall.bat','restore default settings.bat','enable nvidia 3d.reg')) {
    $path = Join-Path $madvr $name
    if (Test-Path -LiteralPath $path -PathType Leaf) { Remove-Item -LiteralPath $path }
}
@{schema=1;version=$version;platform='windows-x64'} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $payload 'release.json') -Encoding UTF8
$files = Get-ChildItem -LiteralPath $payload -Recurse -File | Where-Object { $_.Name -ne 'local-resource-sha256.json' }
$manifest = @{}
foreach ($file in $files) { $manifest[$file.FullName.Substring($payload.Length+1).Replace('\','/')] = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLower() }
$manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $payload 'local-resource-sha256.json') -Encoding UTF8
Push-Location $stage
try {
    & $SevenZip a -t7z $archive $archiveRoot -m0=lzma2 -mx=9 -md=256m -mfb=273 -ms=on -mmt=2 -bb0
    if ($LASTEXITCODE -ne 0) { throw 'Compression failed' }
} finally { Pop-Location }
& $SevenZip t $archive
if ($LASTEXITCODE -ne 0) { throw 'Archive integrity test failed' }
$digest = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLower()
[IO.File]::WriteAllText("$archive.sha256", "$digest  $version.7z`n", [Text.UTF8Encoding]::new($false))
Write-Host "Clean LZMA2 level-9 package: $archive"
Write-Host "SHA-256: $digest"
