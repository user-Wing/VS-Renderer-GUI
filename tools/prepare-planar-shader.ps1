$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$headers=Join-Path $root '.deps/vulkan-headers'
$commit='73814d006638aa00bd553215b054f0f02e457e22'
if(!(Test-Path -LiteralPath $headers)){
    git clone --no-checkout https://github.com/KhronosGroup/Vulkan-Headers.git $headers
    if($LASTEXITCODE -ne 0){throw 'Cannot fetch Vulkan headers'}
    git -C $headers checkout --detach $commit
    if($LASTEXITCODE -ne 0){throw 'Cannot check out pinned Vulkan headers'}
}
if((& git -C $headers rev-parse HEAD).Trim() -ne $commit){throw 'Vulkan headers do not match the pinned commit; preserve and inspect the existing checkout.'}
Write-Host 'Pinned Vulkan headers ready; configure CMake after preparing the color runtime.'
