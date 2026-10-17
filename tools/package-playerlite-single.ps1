param(
    [string]$SourceDirectory = "",
    [string]$OutputFile = ""
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not $SourceDirectory) { $SourceDirectory = Join-Path $root 'dist\1.0.12-PlayerLite' }
if (-not $OutputFile) { $OutputFile = Join-Path $root 'dist\VS-Player.exe' }
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
$stage = [IO.Path]::GetFullPath($stage)
$buildRoot = [IO.Path]::GetFullPath((Join-Path $root 'build')) + [IO.Path]::DirectorySeparatorChar
if (!$stage.StartsWith($buildRoot, [StringComparison]::OrdinalIgnoreCase)) { throw "Stage is outside build: $stage" }
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage | Out-Null
# FFmpeg is provided externally beside the launcher or through PATH.
# User configuration stays beside the launcher and is never embedded.
$ffmpegFiles = @((Get-Content -LiteralPath (Join-Path $SourceDirectory 'release.json') -Raw | ConvertFrom-Json).ffmpegOnlyFiles)
$ffmpegPattern = '^(ffmpeg|ffprobe)\.exe$|^(avcodec|avdevice|avfilter|avformat|avutil|swresample|swscale)(-\d+)?\.dll$'
Get-ChildItem -LiteralPath $SourceDirectory -Force | Where-Object {
    $_.Name -ne 'player.ini' -and $_.Name -notin $ffmpegFiles -and $_.Name -notmatch $ffmpegPattern
} | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $stage -Recurse -Force
}
Copy-Item -LiteralPath $built -Destination (Join-Path $stage 'vs-player.exe') -Force
& (Join-Path (Split-Path $compiler) 'strip.exe') --strip-all (Join-Path $stage 'vs-player.exe')
if ($LASTEXITCODE -ne 0) { throw 'Cannot strip the staged player' }
$forbidden = @(Get-ChildItem -LiteralPath $stage -Recurse -File | Where-Object { $_.Name -in $ffmpegFiles -or $_.Name -match $ffmpegPattern })
if ($forbidden.Count) { throw "FFmpeg unexpectedly embedded: $($forbidden.FullName -join '; ')" }
$releasePath = Join-Path $stage 'release.json'
$release = Get-Content -LiteralPath $releasePath -Raw | ConvertFrom-Json
$files = @(Get-ChildItem -LiteralPath $stage -Recurse -File | Where-Object { $_.Name -notin @('release.json','resource-manifest.json') })
$release.ffmpegOnlyFiles = @()
$release | Add-Member -NotePropertyName externalFFmpeg -NotePropertyValue @('ffmpeg.exe','ffprobe.exe','avcodec-63.dll','avformat-63.dll','avutil-61.dll','swresample-7.dll','swscale-10.dll','avfilter-12.dll') -Force
$release.totalBytes = ($files | Measure-Object Length -Sum).Sum
$release.excludingFFmpegBytes = ($files | Where-Object { $_.Name -notin $release.ffmpegOnlyFiles } | Measure-Object Length -Sum).Sum
$release | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $releasePath -Encoding UTF8
$manifest = [ordered]@{}
Get-ChildItem -LiteralPath $stage -Recurse -File | Where-Object { $_.Name -ne 'resource-manifest.json' } | Sort-Object FullName | ForEach-Object {
    $relative = $_.FullName.Substring($stage.Length + 1).Replace('\','/')
    $manifest[$relative] = @{bytes=$_.Length; sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
}
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $stage 'resource-manifest.json') -Encoding UTF8
if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -CompressionLevel Fastest
$resource = Join-Path $work 'launcher-icons.rc'
$resourceObject = Join-Path $work 'launcher-icons.o'
$iconPath = (Join-Path $root 'assets\icons\player.ico').Replace('\','/')
[IO.File]::WriteAllText($resource, ('101 ICON "{0}"' -f $iconPath), [Text.Encoding]::UTF8)
& (Join-Path (Split-Path $compiler) 'windres.exe') -i $resource -o $resourceObject -O coff
if ($LASTEXITCODE -ne 0) { throw 'Player icon resource compilation failed' }
& $compiler -std=c11 -Os -s -municode -mwindows -static-libgcc (Join-Path $PSScriptRoot 'playerlite-launcher.c') $resourceObject -o $runner -luser32
if ($LASTEXITCODE -ne 0) { throw "PlayerLite launcher compilation failed ($LASTEXITCODE)" }
$hash = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant().Substring(0,24)
$temp = Join-Path $work 'VS-Player.partial.exe'
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
$destination = [IO.Path]::GetFullPath($OutputFile)
$pending = $destination + '.new'
Copy-Item -LiteralPath $temp -Destination $pending -Force
Move-Item -LiteralPath $pending -Destination $destination -Force
Write-Output ('Single EXE: {0} ({1:N2} MiB)' -f $OutputFile,((Get-Item -LiteralPath $OutputFile).Length / 1MB))
Write-Output ('FFmpeg bundled: no; cache LOCALAPPDATA\VSPlayerLite\{0}' -f $hash)
Write-Output 'Configuration: player.ini beside the launcher, preserved across updates.'
