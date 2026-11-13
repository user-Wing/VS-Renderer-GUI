# Opt-in 780M HLSL microbenchmarks

These are standalone source-level GPU executors, not a player backend and not an end-to-end FPS claim. No process suspension or user media is bundled. First make the GPU idle with the user's authorization; never stop someone else's work. Keep original and candidate inputs/settings identical and interleave repetitions.

1. With MSVC vcvars64 and Windows SDK active, build:
```
cl /std:c++20 /EHsc /O2 /I src /I build/anime4k-native/shaders tests/performance/hlsl/bench11.cpp /Fe:build/bench11.exe d3d11.lib d3d12.lib d3dcompiler.lib dxgi.lib
cl /std:c++20 /EHsc /O2 tests/performance/hlsl/bench12.cpp /Fe:build/bench12.exe d3d12.lib d3dcompiler.lib dxgi.lib
cl /std:c++20 /EHsc /O2 tests/performance/hlsl/compare.cpp /Fe:build/compare.exe
```
The Anime4KFast header requires the existing `tools/convert-anime4k-fast.py` output. The bench selects AMD hardware explicitly. The shader-chain timers exclude initial texture upload and final readback. CPU time is command recording/submission, not whole process CPU.

2. Extract a user-selected local 1920×1080 SDR frame to `rgba64le` with an explicit appropriate matrix/range, no rescale. It is converted to RGBA16F before benchmarking. For other dimensions `bench11` accepts optional W H OUT_W OUT_H after its standard arguments.
```
build/bench11.exe tests/fixtures/anime4k-mode-a-reference.vsrhlsl input.rgba64 60 reference.fp16
build/bench11.exe assets/builtin-shaders/anime4k-mode-a.glsl.vsrhlsl input.rgba64 60 candidate.fp16
build/compare.exe reference.fp16 candidate.fp16
```
`compare.cpp` compares **all RGBA** in final source. Early exploratory logs compared RGB only; do not use those to claim full bytewise equivalence. Optional BMP output is only valid for a 3840×2160 result. No screenshot conversion is needed for numeric validation.

3. `prepare.py a base` / `prepare.py a load` / `prepare.py a half` reproduce isolated conversion experiments in `build/hlsl-benchmark/`. They require the project's glslc/spirv-cross and Windows SDK fxc. `plan12.py base` (or matching variant) evaluates the fixed1920×1080→3840×2160 graph and compiles active pixel shaders via DXC/SM6.2. Compile `vs.hlsl` and `vs-pos.hlsl` with `dxc -T vs_6_2 -E main` into `build/hlsl-benchmark/vs.dxil` and `vs-pos.dxil`. Then:
```
build/bench12.exe build/hlsl-benchmark/a-base input.rgba64 60 d12.fp16
```
`VSR_D12_DXBC=1` uses original SM5 bytecode with SM5 vertex shader to isolate API changes; unset for DXIL. `VSR_D12_DEBUG=1` is debugging only, never timing. Pos-only SM6 shaders need the matching pos-only vertex signature. The simple generated fixed-geometry plan is an experiment, not a production resize/device-lost implementation.

4. Regression: build `vsr_anime_fusion_tests`, set `VSR_TEST_GPU=1` (`VSR_TEST_AMD=1` on NuxBox), run it. A repeat-baseline mode (`VSR_FUSION_REPEAT_BASE=1`) checks harness determinism. ClearState and warmup are necessary when running two independent pipelines in one D3D11 context; do not attribute that earlier harness failure to algorithm quality.

Interpret all results together with `docs/780m-hlsl-performance-20261111.md`. DXC/FP16 variants are not automatically better. Production only uses the verified FP32 final reconstruction fusion and leaves the GLSL and Native ABI unchanged.
