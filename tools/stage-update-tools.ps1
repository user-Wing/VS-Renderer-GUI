param(
    [string]$OutputDirectory = 'build/mingw-release/runtime/tools',
    [string]$Aria2Binary = 'build/update-tools/aria2-next-2.8.3-windows-x86_64.exe',
    [string]$SevenZipDirectory = 'C:/PortableSoft/7-Zip-Zstandard'
)
$ErrorActionPreference = 'Stop'
$expected = '08afaf2a44811d38e7ce538da719ab06d6925bcaad1231ee7b92c497f58e5aac'
if (-not (Test-Path -LiteralPath $Aria2Binary)) {
    $downloadDir = Split-Path -Parent $Aria2Binary
    New-Item -ItemType Directory -Path $downloadDir -Force | Out-Null
    Invoke-WebRequest 'https://github.com/AnInsomniacy/aria2-next/releases/download/v2.8.3/aria2-next-2.8.3-windows-x86_64.exe' -OutFile $Aria2Binary
}
if ((Get-FileHash -LiteralPath $Aria2Binary -Algorithm SHA256).Hash.ToLower() -ne $expected) { throw 'aria2-next SHA-256 mismatch' }
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
Copy-Item -LiteralPath $Aria2Binary -Destination (Join-Path $OutputDirectory 'aria2-next.exe') -Force
foreach ($name in @('7z.exe','7z.dll','License.txt')) {
    Copy-Item -LiteralPath (Join-Path $SevenZipDirectory $name) -Destination (Join-Path $OutputDirectory $name) -Force
}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'apply-update.ps1') -Destination $OutputDirectory -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'apply-components.ps1') -Destination $OutputDirectory -Force
Invoke-WebRequest 'https://raw.githubusercontent.com/AnInsomniacy/aria2-next/v2.8.3/COPYING' -OutFile (Join-Path $OutputDirectory 'aria2-COPYING.txt')
Write-Host "Verified aria2-next 2.8.3 and staged full-update tools: $OutputDirectory"
