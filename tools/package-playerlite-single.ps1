param(
    [string]$SourceDirectory = "",
    [string]$OutputFile = ""
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not $SourceDirectory) { $SourceDirectory = Join-Path $root 'dist\1.0.10-PlayerLite' }
if (-not $OutputFile) { $OutputFile = Join-Path $root 'dist\VSPlayerLite.exe' }
$built = Join-Path $root 'build\mingw-release\lite\vs-player.exe'
if (!(Test-Path -LiteralPath $built)) { throw "Rebuild VSPlayerLite first: $built" }
if (!(Test-Path -LiteralPath (Join-Path $SourceDirectory 'FFF.Native.dll'))) { throw "Missing FFF.Native.dll: $SourceDirectory" }
$compiler = 'C:\Private\Soft\QT\Tools\mingw1310_64\bin\gcc.exe'
if (!(Test-Path -LiteralPath $compiler)) { throw "Missing compiler: $compiler" }
$work = Join-Path $root 'build\playerlite-single'
$stage = Join-Path $work 'stage'
$zip = Join-Path $work 'payload.zip'
$runner = Join-Path $work 'launcher.exe'
New-Item -ItemType Directory -Path $work -Force | Out-Null
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage | Out-Null
# FFmpeg shared ABI must be provided next to the EXE or through PATH.
$ffmpeg = '^(ffmpeg|ffprobe)\.exe$|^(avcodec|avdevice|avfilter|avformat|avutil|swresample|swscale)-\d+\.dll$'
Get-ChildItem -LiteralPath $SourceDirectory -Force | Where-Object {
    $_.Name -notmatch $ffmpeg -and $_.Name -ne 'player.ini'
} | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $stage -Recurse -Force
}
Copy-Item -LiteralPath $built -Destination (Join-Path $stage 'vs-player.exe') -Force
$forbidden = @(Get-ChildItem -LiteralPath $stage -Recurse -Force -File | Where-Object { $_.Name -match $ffmpeg })
if ($forbidden.Count) { throw "FFmpeg unexpectedly embedded: $($forbidden.FullName -join '; ')" }
if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -CompressionLevel Fastest
& $compiler -std=c11 -Os -s -municode -mwindows -static-libgcc (Join-Path $PSScriptRoot 'playerlite-launcher.c') -o $runner -luser32
if ($LASTEXITCODE -ne 0) { throw "PlayerLite launcher compilation failed ($LASTEXITCODE)" }
$hash = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant().Substring(0,24)
$temp = Join-Path $work 'VSPlayerLite.partial.exe'
Copy-Item -LiteralPath $runner -Destination $temp -Force
$stream = [IO.File]::Open($temp,[IO.FileMode]::Append,[IO.FileAccess]::Write)
try {
    $archive = [IO.File]::OpenRead($zip)
    try { $archive.CopyTo($stream) } finally { $archive.Dispose() }
    $marker = [Text.Encoding]::ASCII.GetBytes('VSLITE02')
    $stream.Write($marker,0,$marker.Length)
    $count = [BitConverter]::GetBytes([uint64](Get-Item -LiteralPath $zip).Length)
    $stream.Write($count,0,$count.Length)
    $fingerprint = [Text.Encoding]::ASCII.GetBytes($hash)
    $stream.Write($fingerprint,0,$fingerprint.Length)
} finally { $stream.Dispose() }
Copy-Item -LiteralPath $temp -Destination $OutputFile -Force
Write-Output ('Single EXE: {0} ({1:N2} MiB)' -f $OutputFile,((Get-Item -LiteralPath $OutputFile).Length / 1MB))
Write-Output ('FFmpeg bundled: no; cache LOCALAPPDATA\VSPlayerLite\{0}' -f $hash)
Write-Output 'External dependency: matching FFmpeg shared DLLs in EXE directory or PATH.'
Write-Output 'Configuration: player.ini beside the launcher, preserved across updates.'
