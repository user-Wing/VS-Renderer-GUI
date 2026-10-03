param(
    [string]$SourceDirectory = '.deps/ffmpeg-enhanced/04-full-expanded',
    [string[]]$OutputDirectory = @('build/mingw-release', 'dist/VS-Renderer-GUI-windows-x64')
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$source = (Resolve-Path -LiteralPath $SourceDirectory).Path
foreach ($name in @('ffmpeg.exe', 'ffprobe.exe', 'avcodec-63.dll', 'avformat-63.dll', 'avutil-61.dll')) {
    if (-not (Test-Path -LiteralPath (Join-Path $source $name))) { throw "Missing FFmpeg runtime: $name" }
}
& (Join-Path $source 'ffmpeg.exe') -version *> $null
if ($LASTEXITCODE -ne 0) { throw 'FFmpeg runtime cannot start.' }
$backupRoot = Join-Path $projectRoot ('build/ffmpeg-backups/' + [DateTime]::Now.ToString('yyyyMMdd-HHmmss-fff'))
foreach ($directory in $OutputDirectory) {
    $target = (Resolve-Path -LiteralPath $directory).Path
    $runtime = Join-Path $target 'runtime/ffmpeg'
    if ($runtime -eq $source) { throw 'FFmpeg source and destination must differ.' }
    $backup = Join-Path $backupRoot (Split-Path -Leaf $target)
    New-Item -ItemType Directory -Path $backup -Force | Out-Null
    # Only replace the dedicated CLI directory and matching native dependency files.
    if (Test-Path -LiteralPath $runtime) {
        $resolvedRuntime = (Resolve-Path -LiteralPath $runtime).Path
        if (-not $resolvedRuntime.StartsWith($target.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) {
            throw "FFmpeg destination escapes target: $resolvedRuntime"
        }
        Move-Item -LiteralPath $resolvedRuntime -Destination (Join-Path $backup 'cli')
    }
    New-Item -ItemType Directory -Path $runtime -Force | Out-Null
    foreach ($name in @('README.md', 'build-info')) { Copy-Item -LiteralPath (Join-Path $source $name) -Destination $runtime -Recurse -Force }
    foreach ($name in @('ffmpeg.exe', 'ffprobe.exe', 'ffplay.exe')) { Copy-Item -LiteralPath (Join-Path $source $name) -Destination $target -Force }
    $nativeBackup = Join-Path $backup 'native'
    New-Item -ItemType Directory -Path $nativeBackup -Force | Out-Null
    foreach ($dll in Get-ChildItem -LiteralPath $source -Filter '*.dll' -File) {
        $destination = Join-Path $target $dll.Name
        if (Test-Path -LiteralPath $destination) { Copy-Item -LiteralPath $destination -Destination $nativeBackup }
        Copy-Item -LiteralPath $dll.FullName -Destination $destination -Force
        if ((Get-FileHash -LiteralPath $destination).Hash -ne (Get-FileHash -LiteralPath $dll.FullName).Hash) {
            throw "FFmpeg DLL copy mismatch: $destination"
        }
    }
    foreach ($file in Get-ChildItem -LiteralPath $source -Filter '*.exe' -File) {
        if ((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath (Join-Path $target $file.Name)).Hash) {
            throw "FFmpeg CLI copy mismatch: $($file.Name)"
        }
    }
    Write-Host "Updated FFmpeg CLI and native dependencies: $target"
}
Write-Host "Previous FFmpeg files preserved: $backupRoot"
