param([string]$OutputDirectory = 'dist/VS-Renderer-GUI-windows-x64')
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$output = (Resolve-Path -LiteralPath $OutputDirectory).Path
if (-not $output.StartsWith($projectRoot.TrimEnd('\')+'\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Portable trim escapes workspace' }
$paths = @('madVR09217/madVR [debug].ax', 'madVR09217/madVR64 [debug].ax', 'madVR09217/madVR.ax')
# This project's VS core is built with enable_x86_asm=false; verify that the
# running core loads baseline modules before omitting unused ISA copies.
$python = Join-Path $output 'runtime/python/python.exe'
$baseline = & $python -c "import vapoursynth as vs,ctypes; c=vs.core; list(c.plugins()); k=ctypes.WinDLL('kernel32'); k.GetModuleHandleW.argtypes=[ctypes.c_wchar_p]; k.GetModuleHandleW.restype=ctypes.c_void_p; print(int(all(k.GetModuleHandleW(n) for n in ('zsmooth.dll','vszip.dll','libvapoursynthfilters.dll')) and not any(k.GetModuleHandleW(n) for n in ('zsmooth.avx2.dll','zsmooth.zn4.dll','vszip.avx2.dll','vszip.zn4.dll','libvapoursynthfilters_avx2.dll','libvapoursynthfilters_zn4.dll'))))"
if ($LASTEXITCODE -ne 0) { throw 'Cannot verify VapourSynth module selection' }
if ($baseline.Trim() -eq '1') {
    $paths += @('runtime/python/Lib/site-packages/vapoursynth/libvapoursynthfilters_avx2.dll', 'runtime/python/Lib/site-packages/vapoursynth/libvapoursynthfilters_zn4.dll', 'runtime/python/Lib/site-packages/vapoursynth/plugins/zsmooth/zsmooth.avx2.dll', 'runtime/python/Lib/site-packages/vapoursynth/plugins/zsmooth/zsmooth.zn4.dll', 'runtime/python/Lib/site-packages/vapoursynth/plugins/vszip/vszip.avx2.dll', 'runtime/python/Lib/site-packages/vapoursynth/plugins/vszip/vszip.zn4.dll')
}
$backup = Join-Path $projectRoot ('build/portable-trim-backups/' + [DateTime]::Now.ToString('yyyyMMdd-HHmmss-fff'))
foreach ($relative in $paths) {
    $source = Join-Path $output $relative
    if (-not (Test-Path -LiteralPath $source)) { continue }
    $source = (Resolve-Path -LiteralPath $source).Path
    if (-not $source.StartsWith($output.TrimEnd('\')+'\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid portable trim path' }
    $destination = Join-Path $backup $relative
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Move-Item -LiteralPath $source -Destination $destination
}
Write-Host "Trimmed unused deployment files; originals retained in $backup"
