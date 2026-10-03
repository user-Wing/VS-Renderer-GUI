param([string]$DependencyDirectory = '')
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (-not $DependencyDirectory) { $DependencyDirectory = Join-Path $projectRoot '.deps/color' }
New-Item -ItemType Directory -Force -Path (Join-Path $DependencyDirectory 'packages') | Out-Null
foreach ($package in (Get-Content (Join-Path $projectRoot 'third_party/color/packages.json') -Raw | ConvertFrom-Json)) {
    $archive = Join-Path (Join-Path $DependencyDirectory 'packages') ([IO.Path]::GetFileName($package.url))
    if (-not (Test-Path -LiteralPath $archive)) {
        & curl.exe --silent --show-error --fail --location --retry 2 --output $archive $package.url
        if ($LASTEXITCODE -ne 0) { throw "Color dependency download failed: $($package.name)" }
    }
    if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $package.sha256) { throw "Color dependency checksum mismatch: $($package.name)" }
    & tar.exe -xf $archive -C $DependencyDirectory
    if ($LASTEXITCODE -ne 0) { throw "Color dependency extraction failed: $($package.name)" }
}
