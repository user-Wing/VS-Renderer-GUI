param(
    [string]$ProgramDirectory = 'dist/VS-Renderer-GUI-windows-x64',
    [string]$SevenZip = 'C:/PortableSoft/7-Zip-Zstandard/7z.exe',
    [string]$BuiltinDirectory = '',
    [ValidateSet('Full', 'Lite')][string]$Edition = 'Full',
    [string]$Strip = 'C:/Private/Soft/QT/Tools/mingw1310_64/bin/strip.exe',
    [string]$StageDirectory = '',
    [string]$OutputFile = '',
    [switch]$IncludeOpenNgu,
    [ValidateRange(1,2)][int]$CompressionThreads = 2
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$program = (Resolve-Path -LiteralPath $ProgramDirectory).Path
if (-not (Test-Path -LiteralPath (Join-Path $program 'vs-export-worker.exe'))) { throw 'Deploy vs-export-worker.exe with VSRenderer before packaging' }
foreach ($name in @('vsr-thumbnail.dll', 'vsr-thumbnail-worker.exe', 'killvsp.exe')) {
    if (-not (Test-Path -LiteralPath (Join-Path $program $name))) { throw "Deploy thumbnail component before packaging: $name" }
}
$editor = Join-Path $program 'tools/photocraft'
if (!(Test-Path $editor)) { $editor=Join-Path $program 'runtime/photocraft' }
if (-not (Test-Path -LiteralPath (Join-Path $editor 'photocraft.exe'))) { throw 'Build and deploy the pinned PhotoCraft runtime before packaging' }
$builtinSource = if ($BuiltinDirectory) { (Resolve-Path -LiteralPath $BuiltinDirectory).Path } else { Join-Path $program 'vpy/builtin' }
$version = [regex]::Match((Get-Content -LiteralPath (Join-Path $root 'CMakeLists.txt') -Raw), 'project\(VSRenderer VERSION ([0-9.]+)').Groups[1].Value
if (-not $version) { throw 'Project version not found' }
$suffix = if ($Edition -eq 'Lite') { '-Lite' } else { '' }
$stage = if ($StageDirectory) { [IO.Path]::GetFullPath($StageDirectory) } else { Join-Path $root "build/release-stage-$version$suffix" }
# Keep a single versioned root for older portable updaters.
$archiveRoot = if ($Edition -eq 'Full') { "VS-GUI-Full-$version" } else { "VS-GUI-Lite-$version" }
$payload = Join-Path $stage $archiveRoot
$archive = if ($OutputFile) { [IO.Path]::GetFullPath($OutputFile) } else { Join-Path $root "dist/$version$suffix.7z" }
if (Test-Path -LiteralPath $payload) { throw 'Release staging directory already exists; inspect it before creating another package.' }
if (Test-Path -LiteralPath $archive) { throw 'Archive already exists; do not silently update an existing archive.' }
New-Item -ItemType Directory -Path $payload -Force | Out-Null
# Construct a clean distribution, retaining the live installation's settings and user files.
$excludedDirectories = @((Join-Path $program 'cache'), (Join-Path $program 'data'), 'PhotoCraftData', 'shader-cache', 'screenshots', 'vpy', '__pycache__', '.git', '.deps', 'temp', 'logs')
$excludedDirectories += @('ncnn',(Join-Path $program 'config'),(Join-Path $program 'docs'))
$excludedDirectories += (Join-Path $program 'themes')
$excludedDirectories += (Join-Path $program 'shaders/custom')
$excludedDirectories += @((Join-Path $program 'runtime/ffmpeg/build-info'),(Join-Path $program 'tools/ffmpeg/build-info'),(Join-Path $program 'tools/lav'),(Join-Path $program 'tools/madvr09217'))
$excludedDirectories += @((Join-Path $program 'LAVFilters64'), (Join-Path $program 'madVR09217'), (Join-Path $program 'shaders/mpv-shaders'))
if ($Edition -eq 'Lite') {
    $excludedDirectories += @((Join-Path $program 'runtime/mkvtoolnix'),(Join-Path $program 'tools/mkvtoolnix'))
}
& robocopy.exe $program $payload /E /COPY:DAT /DCOPY:DAT /R:2 /W:1 /NFL /NDL /NJH /NJS /NP /XD $excludedDirectories /XF '*.ini' '*.lwi' '*.ffindex' '*.pyc' '*.pdb' '*test*.exe' Qt6Test.dll '*.log' '*.before-*' '*.tmp' '*.autosave' '.awj-update-security-state*' settings.bin resource-manifest.json local-resource-sha256.json mpv-shaders.7z | Out-Null
if ($LASTEXITCODE -ge 8) { throw 'Clean payload copy failed' }
$ffInfo = Join-Path $payload 'tools/ffmpeg/build-info'
New-Item -ItemType Directory -Path $ffInfo -Force | Out-Null
foreach ($name in @('manifest.json','config.mak','build-full09.sh','build-full10.sh','resume-ffmpeg.sh','subsampling-initialization.patch','source-identity.json','package_release.py','result.json','release-size-and-pe.json')) {
    $source = Join-Path $program "tools/ffmpeg/build-info/$name"
    if (!(Test-Path $source)) { $source=Join-Path $program "runtime/ffmpeg/build-info/$name" }
    if (Test-Path -LiteralPath $source) { Copy-Item -LiteralPath $source -Destination $ffInfo -Force }
}
if (!$IncludeOpenNgu -and (Get-ChildItem -LiteralPath $payload -Recurse -File -Filter 'OpenNGU-*')) { throw 'Private OpenNGU assets must be removed from the input staging directory before public packaging.' }
& (Join-Path $PSScriptRoot 'stage-shaders.ps1') -OutputDirectory (Join-Path $payload 'shaders') -SevenZip $SevenZip -ExcludePrivate:(!$IncludeOpenNgu)
if (!(Test-Path -LiteralPath $Strip)) { throw 'MinGW strip.exe is required for release packaging' }
foreach ($name in @('VSRenderer.exe','vs-player.exe','vs-cap.exe','vs-export-worker.exe','killvsp.exe')) {
    & $Strip --strip-unneeded (Join-Path $payload $name)
    if ($LASTEXITCODE -ne 0) { throw "Release symbol stripping failed: $name" }
}
$editor = Join-Path $payload 'tools/photocraft'
if (!(Test-Path $editor)) { $editor=Join-Path $payload 'runtime/photocraft' }
$editorSize = (Get-ChildItem -LiteralPath $editor -File | Measure-Object Length -Sum).Sum
$editorLimit = (Get-Content -LiteralPath (Join-Path $root 'tools/photocraft-source.json') -Raw | ConvertFrom-Json).maximumComponentBytes
if ($editorSize -gt $editorLimit) { throw "PhotoCraft runtime exceeds its pinned size budget: $editorSize > $editorLimit bytes" }
foreach ($name in @('README.md', 'project.md', 'changelog.md')) {
    Copy-Item -LiteralPath (Join-Path $root $name) -Destination $payload -Force
}
Copy-Item -LiteralPath (Join-Path $root 'docs') -Destination $payload -Recurse -Force
Copy-Item -LiteralPath (Join-Path $root 'plugins/3fui-export-bridge/README.md') -Destination (Join-Path $payload 'docs/3fui-install.md') -Force
New-Item -ItemType Directory -Path (Join-Path $payload 'licenses') -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $payload 'licenses/PawnIO-Modules') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'assets/pawnio/COPYING'),(Join-Path $root 'assets/pawnio/NOTICE.md') -Destination (Join-Path $payload 'licenses/PawnIO-Modules') -Force
Get-ChildItem -LiteralPath (Join-Path $root 'third_party/runtime-notices') -File | Copy-Item -Destination (Join-Path $payload 'licenses') -Force
Copy-Item -LiteralPath (Join-Path $root 'third_party/image-zlib/LICENSE') -Destination (Join-Path $payload 'licenses/image-zlib-LICENSE.txt') -Force
New-Item -ItemType Directory -Path (Join-Path $payload 'languages') -Force | Out-Null
foreach ($name in @('zh_CN.json', 'en_US.json')) {
    Copy-Item -LiteralPath (Join-Path $root "assets/languages/$name") -Destination (Join-Path $payload "languages/$name") -Force
}
& python (Join-Path $PSScriptRoot 'layout-portable.py') $payload
if ($LASTEXITCODE -ne 0) { throw 'Portable layout migration failed' }
$componentsPath = Join-Path $payload 'components.json'
$components = Get-Content -LiteralPath $componentsPath -Raw | ConvertFrom-Json
foreach ($name in @('MadVR', 'LAV-Filters', 'mpv-shaders')) { $components.PSObject.Properties.Remove($name) }
if ($Edition -eq 'Lite') { $components.PSObject.Properties.Remove('mkvtoolnix') }
$components | ConvertTo-Json | Set-Content -LiteralPath $componentsPath -Encoding UTF8
$builtin = Join-Path $payload 'vpy/builtin'
New-Item -ItemType Directory -Path $builtin -Force | Out-Null
$descriptions = Join-Path $program 'vpy/preset-descriptions.json'
if (Test-Path -LiteralPath $descriptions) { Copy-Item -LiteralPath $descriptions -Destination (Join-Path $payload 'vpy/preset-descriptions.json') -Force }
foreach ($name in @('Anime.vpy','Realistic.vpy','Anime-0-CNN-Enhanced.vpy','Anime-1-CNN.vpy','Anime-4-Jinc.vpy','Anime-5-D3D11.vpy','Anime-6-AFast-1X-Enhanced.vpy','Anime-7-AFast-1X.vpy','Anime-8-A-CNN-Enhanced.vpy','Anime-9-A-CNN.vpy','MoeSR-50K.vpy','Super-0-A4K-RIFE-A4K.vpy','Super-1-A4K-RIFE-Jinc.vpy','Super-2-A4K-RIFE-D3D11.vpy','Super-3-RIFE-A4K.vpy','Interpolation-0-RIFE.vpy','Interpolation-1-RIFE-Half.vpy','Interpolation-2-MVTools-HQ.vpy','Interpolation-3-MVTools.vpy')) {
    Copy-Item -LiteralPath (Join-Path $builtinSource $name) -Destination $builtin -Force
    if ($name.StartsWith('Interpolation-')) {
        $variant = $name.Replace('.vpy', '-D3D11.vpy')
        $variantSource = Join-Path $builtinSource $variant
        Copy-Item -LiteralPath $(if (Test-Path -LiteralPath $variantSource) { $variantSource } else { Join-Path $builtinSource $name }) -Destination (Join-Path $builtin $variant) -Force
    }
}
if ($Edition -eq 'Full') {
    & python (Join-Path $root 'tools/trim-full-runtime.py') --directory $payload --strip $Strip --sevenzip $SevenZip --report (Join-Path $stage 'runtime-trim.json')
    if ($LASTEXITCODE -ne 0) { throw 'Full runtime trim failed' }
}
@{schema=1;version=$version;platform='windows-x64';edition=$Edition} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $payload 'release.json') -Encoding UTF8
$files = Get-ChildItem -LiteralPath $payload -Recurse -File -Force | Where-Object { $_.Name -ne 'local-resource-sha256.json' }
$manifest = @{}
foreach ($file in $files) { $manifest[$file.FullName.Substring($payload.Length+1).Replace('\','/')] = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLower() }
$manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $payload 'local-resource-sha256.json') -Encoding UTF8
Push-Location $stage
try {
    & $SevenZip a -t7z $archive $archiveRoot -m0=lzma2 -mx=9 -md=256m -mfb=273 -ms=on "-mmt=$CompressionThreads" -bb0
    if ($LASTEXITCODE -ne 0) { throw 'Compression failed' }
} finally { Pop-Location }
& $SevenZip t $archive
if ($LASTEXITCODE -ne 0) { throw 'Archive integrity test failed' }
$digest = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLower()
[IO.File]::WriteAllText("$archive.sha256", "$digest  $([IO.Path]::GetFileName($archive))`n", [Text.UTF8Encoding]::new($false))
Write-Host "Clean LZMA2 level-9 package: $archive"
Write-Host "SHA-256: $digest"
