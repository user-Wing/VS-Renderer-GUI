param([string]$SevenZip = 'C:/PortableSoft/7-Zip-Zstandard/7z.exe')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$stage = Join-Path $root "build/dev-handoff-$stamp"
$payload = Join-Path $stage 'VS-Renderer-GUI-dev'
$archive = Join-Path $root "dist/handoff/VS-Renderer-GUI-dev-$stamp.7z"
if ((Test-Path -LiteralPath $stage) -or (Test-Path -LiteralPath $archive)) { throw 'Handoff target already exists.' }
if (-not (Test-Path -LiteralPath $SevenZip)) { throw '7-Zip executable not found.' }
New-Item -ItemType Directory -Path $payload,(Split-Path $archive) -Force | Out-Null
function Copy-FileToPayload([string]$relative) {
    $source = Join-Path $root $relative
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { throw "Missing source: $relative" }
    $target = Join-Path $payload $relative
    New-Item -ItemType Directory -Path (Split-Path $target) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $target
}
function Copy-TreeToPayload([string]$relative, [string[]]$excludedDirs = @(), [string[]]$excludedFiles = @()) {
    $source = Join-Path $root $relative
    if (-not (Test-Path -LiteralPath $source -PathType Container)) { throw "Missing dependency: $relative" }
    $arguments = @($source, (Join-Path $payload $relative), '/E', '/COPY:DAT', '/DCOPY:DAT', '/R:1', '/W:1', '/NFL', '/NDL', '/NJH', '/NJS', '/NP')
    if ($excludedDirs.Count) { $arguments += '/XD'; $arguments += $excludedDirs }
    if ($excludedFiles.Count) { $arguments += '/XF'; $arguments += $excludedFiles }
    & robocopy.exe @arguments | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "Dependency copy failed: $relative" }
}
$sourceFiles = & git -C $root ls-files --cached --others --exclude-standard
if ($LASTEXITCODE -ne 0) { throw 'Could not enumerate project sources.' }
foreach ($relative in $sourceFiles) {
    if ($relative -like 'assets/icon-concepts/*') { continue }
    Copy-FileToPayload $relative
}
# Keep the VS API headers, not its full Git checkout or build environment.
Copy-TreeToPayload 'third_party/vapoursynth/include'
Copy-FileToPayload 'third_party/vapoursynth/COPYING.LESSER'
# Current patched native source and link-time SDKs, without any old binaries.
Copy-TreeToPayload '.deps/fff-player/FFF.Native' @('obj','x64','Win32','.vs') @('*.user','*.pdb','*.dll','*.obj','*.lib','*.exp','*.log')
foreach ($part in @('include','lib')) { Copy-TreeToPayload ".deps/fff-player/third_party/ffmpeg/$part" }
foreach ($license in @('COPYING.LGPLv2.1','COPYING.LGPLv3')) { Copy-FileToPayload ".deps/fff-player/third_party/ffmpeg/$license" }
foreach ($part in @('include','lib','share')) { Copy-TreeToPayload ".deps/fff-player/third_party/vcpkg_installed/x64-windows/$part" }
# Extracted, pinned CMake dependency inputs avoid another network download.
foreach ($name in @('libdeflate','libavif','dav1d','libyuv','turbojpeg-source','nasm')) { Copy-TreeToPayload ".deps/image/$name" @('.git','__pycache__') }
Copy-TreeToPayload '.deps/color/ucrt64'
Copy-FileToPayload 'docs/nuxbox-performance-handoff.md'
Copy-Item -LiteralPath (Join-Path $root 'docs/nuxbox-performance-handoff.md') -Destination (Join-Path $payload 'START-HERE.md')
$commit = & git -C $root rev-parse HEAD
$changes = @(& git -C $root status --short)
$files = @(Get-ChildItem -LiteralPath $payload -Recurse -File | Sort-Object FullName)
$records = @($files | ForEach-Object {
    @{ path = $_.FullName.Substring($payload.Length + 1).Replace('\','/'); bytes = $_.Length; sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
})
$badFiles = @($records | Where-Object { $_.path -match '^(build|dist|cache|shader-cache|\.git)/|(^|/)(player\.ini|CMakeCache\.txt|[^/]+\.user|[^/]+\.pdb)$' })
if ($badFiles.Count) { throw 'Personal configuration or build artifacts leaked into the handoff.' }
@{ schema=1; baseCommit=$commit; workingTreeChanges=$changes; fileCount=$records.Count; files=$records } | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $payload 'handoff-manifest.json') -Encoding UTF8
Push-Location $stage
try { & $SevenZip a -t7z -m0=lzma2 -mx=5 -mmt=4 $archive 'VS-Renderer-GUI-dev' | Out-Null; if ($LASTEXITCODE) { throw 'Handoff archive failed.' } }
finally { Pop-Location }
& $SevenZip t $archive | Out-Null
if ($LASTEXITCODE) { throw 'Handoff archive integrity check failed.' }
$hash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
@{ archive=$archive; sha256=$hash; bytes=(Get-Item -LiteralPath $archive).Length; files=$records.Count; payload=$payload } | ConvertTo-Json
