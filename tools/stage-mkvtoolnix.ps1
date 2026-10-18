param(
    [string]$SourceDirectory='C:/PortableSoft/Mkvtoolnix',
    [string]$OutputDirectory='build/mingw-release/runtime'
)
$ErrorActionPreference='Stop'
$mkvSource=(Resolve-Path -LiteralPath $SourceDirectory).Path
$work=Join-Path ([IO.Path]::GetTempPath()) ('vsr-mkv-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work -Force | Out-Null
try {
    foreach($name in @('mkvmerge.exe','mkvextract.exe','mkvinfo.exe','mkvpropedit.exe')) {
        Copy-Item -LiteralPath (Join-Path $mkvSource $name) -Destination $work
    }
    # CLI tools are statically linked; GUI, its helper tools and locale packs are unused.
    New-Item -ItemType Directory -Path (Join-Path $work 'doc') | Out-Null
    foreach($name in @('licenses','en')) {
        Copy-Item -LiteralPath (Join-Path $mkvSource "doc/$name") -Destination (Join-Path $work 'doc') -Recurse
    }
    New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
    $archive=Join-Path ([IO.Path]::GetFullPath($OutputDirectory)) 'mkvtoolnix.zip'
    Compress-Archive -Path (Join-Path $work '*') -DestinationPath $archive -CompressionLevel Optimal -Force
    $versionOutput = & (Join-Path $mkvSource 'mkvmerge.exe') --version
    if ($LASTEXITCODE -ne 0) { throw 'Cannot query MKVToolNix version' }
    $version = [regex]::Match($versionOutput, 'mkvmerge v([0-9.]+)').Groups[1].Value
    if (!$version) { throw 'MKVToolNix version missing' }
    [IO.File]::WriteAllText((Join-Path ([IO.Path]::GetFullPath($OutputDirectory)) 'mkvtoolnix.version'), $version)
    Write-Output "MKVToolNix CLI archive staged: $archive"
} finally {
    $resolved=[IO.Path]::GetFullPath($work)
    if ((Split-Path -Parent $resolved) -ne [IO.Path]::GetTempPath().TrimEnd('\') -or (Split-Path -Leaf $resolved) -notmatch '^vsr-mkv-[a-f0-9]{32}$') { throw 'Invalid MKV staging directory' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
