param(
    [Parameter(Mandatory = $true)][string]$SourceDirectory,
    [string]$OutputDirectory = 'build/mingw-release/runtime/vlc'
)
$ErrorActionPreference = 'Stop'
$source = (Resolve-Path -LiteralPath $SourceDirectory).Path
foreach ($file in @('libvlc.dll', 'libvlccore.dll', 'plugins', 'COPYING.txt', 'AUTHORS.txt')) {
    if (-not (Test-Path -LiteralPath (Join-Path $source $file))) { throw "Incomplete VLC runtime: $file" }
}
$output = [System.IO.Path]::GetFullPath($OutputDirectory)
if ((Test-Path -LiteralPath $output) -and (Get-ChildItem -LiteralPath $output -Force | Select-Object -First 1)) { throw "Use an empty output directory: $output" }
New-Item -ItemType Directory -Force -Path $output | Out-Null
# BD navigation only: retain the demux/codec/output chain, not VLC's GUI,
# network protocols, encoders, muxers or streaming/discovery services.
foreach ($file in @('libvlc.dll','libvlccore.dll','COPYING.txt','AUTHORS.txt')) {
    Copy-Item -LiteralPath (Join-Path $source $file) -Destination $output -Force
}
$plugins = @{
    access = @('liblibbluray_plugin.dll','libfilesystem_plugin.dll','libattachment_plugin.dll')
    codec = @('libavcodec_plugin.dll','libaraw_plugin.dll','liblpcm_plugin.dll','libspdif_plugin.dll','librawvideo_plugin.dll','libd3d11va_plugin.dll','libdxva2_plugin.dll','libpng_plugin.dll','libjpeg_plugin.dll','libtextst_plugin.dll')
    demux = @('libts_plugin.dll','libps_plugin.dll','libes_plugin.dll','libh26x_plugin.dll','libvc1_plugin.dll','libmpgv_plugin.dll','libimage_plugin.dll')
    audio_output = @('libmmdevice_plugin.dll','libwasapi_plugin.dll','libdirectsound_plugin.dll','libadummy_plugin.dll')
    video_output = @('libdirect3d11_plugin.dll','libdirect3d9_plugin.dll','libdrawable_plugin.dll','libwingdi_plugin.dll','libwinhibit_plugin.dll')
    video_filter = @('libblend_plugin.dll','libscale_plugin.dll','libdeinterlace_plugin.dll')
    misc = @('libstats_plugin.dll','liblogger_plugin.dll')
}
foreach ($category in @('audio_mixer','audio_filter','packetizer','video_chroma','text_renderer','spu','d3d11','d3d9','logger','stream_filter')) {
    $plugins[$category] = @(Get-ChildItem -LiteralPath (Join-Path $source "plugins/$category") -Filter '*.dll' -File | Select-Object -ExpandProperty Name)
}
foreach ($category in $plugins.Keys) {
    $destination = Join-Path $output "plugins/$category"
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    foreach ($plugin in $plugins[$category]) { Copy-Item -LiteralPath (Join-Path $source "plugins/$category/$plugin") -Destination $destination -Force }
}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot '../third_party/runtime-notices/libvlc-NOTICE.txt') -Destination $output -Force
Write-Host "BD-only menu runtime staged: $output"
