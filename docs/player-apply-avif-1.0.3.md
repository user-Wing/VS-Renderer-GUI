# VS Player 1.0.3：应用设置与 AVIF 内容验证

日期：2026-10-02，版本保持1.0.3。

设置新增“应用 / Apply”，保存并应用当前配置，保留对话框。重新打开视频时使用原播放状态和媒体时间，暂停不会因为默认自动播放而启动；即使关闭记忆位置，也恢复本次应用前的时间。图片应用设置只更新界面和缩放设置，不重新解码巨图。

首次生成的 `build/supplied-large-grid.avif` 不是完整参考样本。其CICP为1/13/6、YUV444，不是GBR。官方avifenc读取JPEG的RGB扫描行使用 `row * rgb.rowBytes` 的32位乘法；原图31603×65278的RGB缓冲超过4 GiB，偏移在约45301行后回绕，后面的扫描行覆盖顶部，末尾区域没有写入。对解码像素直接抽样确认顶部错色和底部黑块已存在于文件，而非窗口绘制。先前只检查解码返回、尺寸和少量颜色变化，不足以证实画面完整，相关历史记录已更正。

修正样本 `build/supplied-large-yuv420.avif` 通过libjpeg-turbo直接提取原JPEG的YUV420平面，绕过有问题的RGB扫描行读取，再由avifenc/aom生成8×16网格。原图宽31603为奇数，网格YUV420要求宽度为偶数，因此仅在右边补一列边缘像素，输出31604×65278；没有缩小或裁掉图像。文件283151835字节。它仍是有损编码，不宣称像素完全相同或由zenrav1e编码。

解码采用libavif/dav1d，开启同工具链编译的libyuv1924 SIMD色彩转换，保留正常双线性色度上采样。相同有效YUV420样本解码从23675 ms降至6380 ms；没有启用GPU解码或完整原图显存缓存。Tab增加AVIF原始CICP和GBR标识，方便确认实际色彩矩阵。

实际像素对比：均匀抽样316×653，共206348个位置，对比原JPG，平均RGB通道误差5.856，底部三分之一6.037；均低于专项上限30。抽样预览人工检查完整构图及底部，没有首次样本的黑块与顶部错色。证据：`build/avif-corrected-pixels.txt`（优化前）、`build/avif-simd-pixels.txt`（优化后）、`build/jpg-original-pixel-sample.png`、`build/avif-simd-pixel-sample.png`。

正常10位GBR参考样本 `tests/fixtures/gbr-10bit.avif` 使用identity矩阵、无损渐变；红/绿/蓝分布和透明通道专项通过。用户 `TEST.avif` 的直接像素抽样呈严重错色；没有对应原始参考，当前不宣称该文件完好或可以恢复缺失像素，证据 `build/avif-test-pixel-sample.png`。

仅验证本次相关专项：暂停/播放时Apply及媒体时间、对话框不关闭、原设置页与语言、GBR颜色、网格/高位深透明，以及实际巨图内容对比。Release构建通过，4项功能加初始化/清理6/6通过：`build/player-apply-avif-simd-tests.txt`；312条语言字符串检查通过。未运行或修复无关回归。

本地便携Player已更新，交付目录相同专项6/6通过（`build/player-apply-avif-portable-tests.txt`）。EXE构建/交付SHA-256一致，12份INI/VPY哈希保留（`build/player-apply-avif-delivery-audit.json`）；临时验证EXE和Qt6Test.dll已清理。没有改版本、生成发行压缩包、提交或推送。

本轮同时更新LAV Filters 0.83.0 x64，源为用户提供的 `C:/Users/ARXChem/Downloads/LAVFilters-0.83-x64.zip`，SHA-256 `0126982f47157bb86a6dbb43c4f332f7f98beba9ad552c19b65f9db2e7d4f186`。构建与便携目录21份根级运行时/许可/说明文件哈希均与解压源一致，三个AX版本均为0.83.0，移除旧版不再使用的带 `-lav-` 名称DLL。`stage-portable.ps1` 默认使用 `.deps/lav/0.83`，完整许可保留，不注册系统组件。便携LAV打开/播放/暂停/寻址专项通过：`build/lav-0.83-portable-tests.txt`（1项功能+初始化/清理3/3）；文件审计 `build/lav-0.83-delivery-audit.json`。12份INI/VPY哈希再次核对保留。
