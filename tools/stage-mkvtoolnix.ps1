param(
    [string]$SourceDirectory='C:/PortableSoft/Mkvtoolnix',
    [string]$OutputDirectory='build/mingw-release/runtime',
    [string]$SevenZip='C:/PortableSoft/7-Zip-Zstandard/7z.exe'
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
    $archive=Join-Path $work 'mkvtoolnix.7z'
    Push-Location $work
    try {
        & $SevenZip a -t7z $archive mkvmerge.exe mkvextract.exe mkvinfo.exe mkvpropedit.exe doc -mx=9 -md=64m -ms=on -mmt=4 -bb0
        if ($LASTEXITCODE -ne 0) { throw 'MKVToolNix archive compression failed' }
    } finally { Pop-Location }
    $versionOutput = & (Join-Path $mkvSource 'mkvmerge.exe') --version
    if ($LASTEXITCODE -ne 0) { throw 'Cannot query MKVToolNix version' }
    $version = [regex]::Match($versionOutput, 'mkvmerge v([0-9.]+)').Groups[1].Value
    if (!$version) { throw 'MKVToolNix version missing' }
    Copy-Item -LiteralPath $archive -Destination (Join-Path $OutputDirectory 'mkvtoolnix.7z') -Force
    Remove-Item -LiteralPath (Join-Path $OutputDirectory 'mkvtoolnix.zip') -Force -ErrorAction SilentlyContinue
    [IO.File]::WriteAllText((Join-Path ([IO.Path]::GetFullPath($OutputDirectory)) 'mkvtoolnix.version'), $version)
    Write-Output "MKVToolNix CLI archive staged: $([IO.Path]::GetFullPath((Join-Path $OutputDirectory 'mkvtoolnix.7z')))"
} finally {
    $resolved=[IO.Path]::GetFullPath($work)
    if ((Split-Path -Parent $resolved) -ne [IO.Path]::GetTempPath().TrimEnd('\') -or (Split-Path -Leaf $resolved) -notmatch '^vsr-mkv-[a-f0-9]{32}$') { throw 'Invalid MKV staging directory' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
