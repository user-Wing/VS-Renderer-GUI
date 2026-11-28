param(
    [string]$OutputDirectory,
    [string]$SevenZip,
    [string]$CompiledDirectory = "",
    [switch]$IncludeOptional,
    [switch]$ExcludePrivate
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$source = Join-Path $projectRoot 'assets/mpv-shaders'
if (!$CompiledDirectory) { $CompiledDirectory = Join-Path $projectRoot '.deps/compiled-shaders110' }
if (Test-Path -LiteralPath (Join-Path $CompiledDirectory 'mpv-shaders')) { $CompiledDirectory = Join-Path $CompiledDirectory 'mpv-shaders' }
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (-not $output.StartsWith($projectRoot.TrimEnd('\')+'\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Shader deployment escapes workspace' }
$openNgu = Join-Path $projectRoot '.deps/openngu'
if (!$ExcludePrivate -and (Test-Path -LiteralPath $openNgu)) {
    New-Item -ItemType Directory -Path $output -Force | Out-Null
    Get-ChildItem -LiteralPath $openNgu -File | Where-Object { $_.Name -like 'OpenNGU-*' } | Copy-Item -Destination $output -Force
}
if (!$IncludeOptional) {
    New-Item -ItemType Directory -Path $output -Force | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $projectRoot 'assets/builtin-shaders') -File |
        Where-Object { $_.Extension -in @('.glsl', '.hlsl', '.vsrhlsl', '.json') } |
        Copy-Item -Destination $output -Force
    $backup = Join-Path $projectRoot ('build/shader-backups/' + [guid]::NewGuid().ToString('N'))
    $curated = @(Get-ChildItem -LiteralPath (Join-Path $projectRoot 'assets/builtin-shaders') -File | ForEach-Object Name)
    $obsolete = @(Get-ChildItem -LiteralPath $output -File | Where-Object { $_.Name -like 'anime4k*' -and $_.Name -notin $curated } | ForEach-Object Name)
    foreach ($name in (@('mpv-shaders', 'mpv-shaders.7z') + $obsolete)) {
        $existing = Join-Path $output $name
        if (Test-Path -LiteralPath $existing) {
            New-Item -ItemType Directory -Path $backup -Force | Out-Null
            Move-Item -LiteralPath $existing -Destination (Join-Path $backup $name)
        }
    }
    return
}
if (Test-Path -LiteralPath (Join-Path $output 'mpv-shaders')) {
    $existing = (Resolve-Path -LiteralPath (Join-Path $output 'mpv-shaders')).Path
    if (-not $existing.StartsWith($output.TrimEnd('\')+'\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid shader deployment path' }
    $backup = Join-Path $projectRoot ('build/shader-backups/' + [DateTime]::Now.ToString('yyyyMMdd-HHmmss-fff'))
    New-Item -ItemType Directory -Path $backup -Force | Out-Null
    Move-Item -LiteralPath $existing -Destination $backup
}
New-Item -ItemType Directory -Path (Join-Path $output 'mpv-shaders') -Force | Out-Null
$archive = Join-Path $output 'mpv-shaders-new.7z'
$staging = Join-Path $projectRoot 'build/shader-archive-stage'
New-Item -ItemType Directory -Path $staging -Force | Out-Null
& robocopy.exe $source (Join-Path $staging 'mpv-shaders') /MIR /R:1 /W:1 /NFL /NDL /NJH /NJS /NP | Out-Null
if ($LASTEXITCODE -ge 8) { throw 'Shader source staging failed' }
if (Test-Path -LiteralPath $CompiledDirectory) {
    & robocopy.exe $CompiledDirectory (Join-Path $staging 'mpv-shaders') /E /R:1 /W:1 /NFL /NDL /NJH /NJS /NP | Out-Null
    if ($LASTEXITCODE -ge 8) { throw 'Compiled shader staging failed' }
}
Push-Location $staging
try {
    & $SevenZip a $archive mpv-shaders -t7z -m0=lzma2 -mx=9 -ms=64m -md=32m -mmt=2 -bso0 -bsp0
    if ($LASTEXITCODE -ne 0) { throw 'Shader archive creation failed' }
    & $SevenZip t $archive -bso0 -bsp0
    if ($LASTEXITCODE -ne 0) { throw 'Shader archive verification failed' }
} finally { Pop-Location }
foreach ($file in Get-ChildItem -LiteralPath $source -Recurse -File | Where-Object { $_.Extension -notin @('.glsl', '.hook') }) {
    $destination = Join-Path (Join-Path $output 'mpv-shaders') $file.FullName.Substring($source.Length+1)
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath $file.FullName -Destination $destination -Force
}
Move-Item -LiteralPath $archive -Destination (Join-Path $output 'mpv-shaders.7z') -Force
