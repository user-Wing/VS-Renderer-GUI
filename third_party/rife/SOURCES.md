# RIFE runtime provenance

- Plugin: https://github.com/styler00dollar/VapourSynth-RIFE-ncnn-Vulkan/releases/tag/r9_mod_v33
- Upstream inference/model project: https://github.com/hzwer/Practical-RIFE
- Vulkan inference library: https://github.com/Tencent/ncnn
- Converted model files supplied by the user installation: `C:\PortableSoft\FFmpegFreeUI ReadyToRun x64\plugin\videoenhancer\models\Frame-Interpolation\RIFE`, directories `rife-v4.26` and `rife-v4.26-heavy`.
- Pinning: runtime-sha256.json; installer refuses a mismatched DLL or model.
- Binaries and weights are staged into the portable runtime, not committed as source. PKL weights are not loaded by the NCNN backend.
