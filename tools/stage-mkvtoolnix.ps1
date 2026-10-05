param(
    [string]$SourceDirectory='C:/PortableSoft/Mkvtoolnix',
    [string]$OutputDirectory='dist/VS-Renderer-GUI-windows-x64/runtime/mkvtoolnix'
)
$ErrorActionPreference='Stop'
$mkvSource=(Resolve-Path -LiteralPath $SourceDirectory).Path
foreach($name in @('mkvmerge.exe','mkvextract.exe','mkvinfo.exe','mkvpropedit.exe')){if(!(Test-Path -LiteralPath (Join-Path $mkvSource $name))){throw "Missing MKVToolNix: $name"}}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
& robocopy.exe $mkvSource $OutputDirectory /E /COPY:DAT /DCOPY:DAT /R:1 /W:1 /XD cache /XF '*.ini' '*.log' '*.tmp' /NFL /NDL /NJH /NJS /NP | Out-Null
if($LASTEXITCODE -ge 8){throw 'MKVToolNix staging failed'}
Write-Output "MKVToolNix staged: $OutputDirectory"
