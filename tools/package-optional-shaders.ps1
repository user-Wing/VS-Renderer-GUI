param(
    [string]$SevenZip = 'C:/PortableSoft/7-Zip-Zstandard/7z.exe',
    [string]$Archive = 'dist/VS-GUI-mpv-shaders-optional.7z'
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$archivePath = [IO.Path]::GetFullPath((Join-Path $root $Archive))
if (Test-Path -LiteralPath $archivePath) { throw 'Optional shader archive already exists; inspect it before replacing it.' }
$stage = Join-Path $root ('build/optional-shaders-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path (Join-Path $stage 'shaders') -Force | Out-Null
& (Join-Path $PSScriptRoot 'stage-shaders.ps1') -OutputDirectory (Join-Path $stage 'shaders') -SevenZip $SevenZip -IncludeOptional
[IO.File]::WriteAllText((Join-Path $stage 'SHADERS-README.txt'), "VS-GUI 可选 mpv 着色器库`r`n将 shaders 文件夹解压到 Full 安装目录，然后重启 VS Renderer / VS Player。默认 Anime4K 与 MoeSR 无需此扩展包。源码版本、SHA256 与许可随附于 shaders/mpv-shaders。`r`n", [Text.UTF8Encoding]::new($false))
Push-Location $stage
try {
    & $SevenZip a -t7z $archivePath shaders SHADERS-README.txt -m0=lzma2 -mx=9 -md=64m -mmt=4 -bso0 -bsp0
    if ($LASTEXITCODE -ne 0) { throw 'Optional shader compression failed' }
} finally { Pop-Location }
& $SevenZip t $archivePath -bso0 -bsp0
if ($LASTEXITCODE -ne 0) { throw 'Optional shader archive integrity test failed' }
$hash = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText("$archivePath.sha256", "$hash  $([IO.Path]::GetFileName($archivePath))`n", [Text.UTF8Encoding]::new($false))
Write-Host "Optional shader package: $archivePath"
