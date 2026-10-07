param([string]$TargetDirectory = 'build/photocraft/target')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$pin = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'photocraft-source.json') -Raw | ConvertFrom-Json
$source = Join-Path $root '.deps/photocraft'
$patch = Join-Path $root 'patches/photocraft-vsp-integration.patch'
if (-not (Test-Path -LiteralPath $source)) {
    & git clone $pin.repository $source
    if ($LASTEXITCODE) { throw 'PhotoCraft clone failed' }
    & git -C $source checkout --detach $pin.commit
    if ($LASTEXITCODE) { throw 'PhotoCraft checkout failed' }
}
$head = & git -C $source rev-parse HEAD
if ($head -ne $pin.commit) { throw "PhotoCraft source must be at $($pin.commit); refusing to reset local changes" }
& git -C $source apply --reverse --check $patch 2>$null
if ($LASTEXITCODE) {
    & git -C $source apply --check $patch
    if ($LASTEXITCODE) { throw 'PhotoCraft integration patch conflicts with local edits' }
    & git -C $source apply $patch
    if ($LASTEXITCODE) { throw 'PhotoCraft integration patch failed' }
}
$cargo = Get-Command cargo.exe -ErrorAction SilentlyContinue
$cargoPath = if ($cargo) { $cargo.Source } else { Join-Path $env:USERPROFILE '.cargo/bin/cargo.exe' }
if (-not (Test-Path -LiteralPath $cargoPath)) { throw 'Rust MSVC toolchain (rustup) and Visual Studio C++ Build Tools are required for building only' }
$previousTarget = $env:CARGO_TARGET_DIR
$previousFonts = $env:CRAFT_FONTS_DIR
try {
    $env:CARGO_TARGET_DIR = [IO.Path]::GetFullPath((Join-Path $root $TargetDirectory))
    $env:CRAFT_FONTS_DIR = $null
    & $cargoPath build --manifest-path (Join-Path $source 'Cargo.toml') --locked --profile $pin.profile -p photocraft
    if ($LASTEXITCODE) { throw 'PhotoCraft build failed' }
    $payload = Join-Path $root 'build/photocraft-runtime'
    New-Item -ItemType Directory -Path $payload -Force | Out-Null
    New-Item -ItemType File -Path (Join-Path $payload 'PhotoCraft.portable') -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $env:CARGO_TARGET_DIR "$($pin.profile)/photocraft.exe") -Destination $payload -Force
    # Rust's MSVC build imports this CRT DLL. Ship the x64 redistributable beside the editor.
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $vsRoot = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $crt = Get-ChildItem -LiteralPath (Join-Path $vsRoot 'VC/Redist/MSVC') -Filter vcruntime140.dll -File -Recurse |
        Where-Object { $_.FullName -match '\\x64\\Microsoft\.VC\d+\.CRT\\vcruntime140\.dll$' } |
        Sort-Object FullName -Descending | Select-Object -First 1
    if (-not $crt) { throw 'The x64 MSVC CRT redistributable was not found' }
    Copy-Item -LiteralPath $crt.FullName -Destination $payload -Force
    @'
VCRUNTIME140.dll: Microsoft Visual C++ x64 Redistributable, copyright Microsoft Corporation.
Redistributed from the licensed Visual Studio C++ Build Tools redistributable directory.
This component is not covered by PhotoCraft's MIT license.
PhotoCraft itself is distributed under LICENSE-MIT; bundled fonts, icons and dictionary
retain their respective licenses in this folder. No proprietary ArtCraft Marks are bundled.
'@ | Set-Content -LiteralPath (Join-Path $payload 'RUNTIME-NOTICE.txt') -Encoding UTF8
    foreach ($file in @('LICENSE-MIT','LICENSE-APACHE','NOTICE','ATTRIBUTION.md','assets/fonts/OFL-Inter.txt','assets/fonts/OFL-JetBrainsMono.txt','assets/icons/LICENSE-lucide.txt','assets/dict/LICENSE-SCOWL.txt','assets/app-icon/LICENSE.txt')) {
        $destination = if ($file -eq 'assets/app-icon/LICENSE.txt') { 'LICENSE-app-icon.txt' } else { Split-Path $file -Leaf }
        Copy-Item -LiteralPath (Join-Path $source $file) -Destination (Join-Path $payload $destination) -Force
    }
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'photocraft-source.json') -Destination $payload -Force
    $size = (Get-ChildItem -LiteralPath $payload -File | Measure-Object Length -Sum).Sum
    if ($size -gt $pin.maximumComponentBytes) { throw "PhotoCraft component exceeds 45 MB: $size bytes" }
    Write-Output "PhotoCraft runtime: $size bytes; Rust/Cargo and CLI are not runtime dependencies"
} finally {
    $env:CARGO_TARGET_DIR = $previousTarget
    $env:CRAFT_FONTS_DIR = $previousFonts
}
