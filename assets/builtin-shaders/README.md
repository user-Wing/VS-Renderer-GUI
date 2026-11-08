# 内置 HLSL 模型来源

- `anime4k-mode-a.glsl`：沿用项目随包完整 Anime4K V4 Mode A，原版权声明保留在源码内。预编译文件由现有 ShaderPackage 生成并绑定源码 SHA-256。
- `source/moesr-50k-original.hlsl`：用户提供的 `moesr-realtime-2x.hlsl`，SHA-256 `4acf5787ce6d3c9a886d3e4b6f81f6dfcd757a8314bd2a9be082d7395a044d24`；开发者自训练的 MoeSR/RealHATGan 蒸馏模型，菜单名由开发者指定为 MoeSR-50K。
- `moesr-50k.hlsl`：由 `python tools/optimize-moesr-hlsl.py assets/builtin-shaders/source/moesr-50k-original.hlsl assets/builtin-shaders/moesr-50k.hlsl` 生成。保持52个pass、全部权重与最后的像素重排，仅将1:1卷积的零填充采样改为整数Load。工具只接受该导出器的已知结构，不能作为通用优化器。
- `*.vsrhlsl.json` 记录对应源码哈希；源码改变时必须用 `vsr_shader_precompile` 重建字节码和元数据。运行期验证哈希，失配不使用旧字节码。
