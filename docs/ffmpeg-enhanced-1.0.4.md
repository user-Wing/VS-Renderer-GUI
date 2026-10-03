# 1.0.4 本地增强版 FFmpeg

后续体积优化已将ffmpeg/ffprobe/ffplay移至程序根目录共用3FP的DLL，runtime/ffmpeg仅保留README/build-info；最新布局见tracks-size-1.0.4.md。下面的隔离部署是首次替换时的历史记录。

来源：维护者提供的 `C:/Users/ARXChem/Documents/Open Source Projects/ffmpeg-autokey444-full.7z`，2026-10-03 构建，FFmpeg 8.0.git，clang 23.1.2，x86-64-v3。归档SHA-256：`b56bcecdb5de1e768f14b2e18564dd052446b73e36af8903b043561c86684811`。提取后的原始文件及逐文件哈希保存在 `ffmpeg-enhanced-runtime.json`。

## 部署

完整原始包保存在本机 `.deps/ffmpeg-enhanced/04-full-expanded`。运行 `tools/stage-ffmpeg.ps1` 更新以下两处：

- `build/mingw-release/runtime/ffmpeg` 和对应构建目录旁的FFmpeg共享库/依赖。
- `dist/VS-Renderer-GUI-windows-x64/runtime/ffmpeg` 和对应程序目录旁的FFmpeg共享库/依赖。

独立CLI目录整体更新，保留ffmpeg/ffprobe/ffplay、全部配套DLL、上游README和build-info。原生3FP使用同主版本的avcodec63/avformat63/avutil61/swscale10/swresample7等更新库；未替换FFF.Native.dll、LAV、VapourSynth源插件和用户配置。VS插件自身的内置编解码器不会因这次替换自动改变。

旧CLI目录与被替换的原生依赖保存在 `build/ffmpeg-backups/20261003-111635-042`。便携部署从构建运行时同步；旧发布脚本优先采用构建目录 `runtime/ffmpeg/ffmpeg.exe` 并携带构建记录。此轮没有重新构建EXE、制作归档或推送代码。

## 能力与边界

实际枚举确认AVS3编码 `libuavs3e` 和解码 `libuavs3d`；还有 `libzenrav1e`、`librav1e`、`libopenh264`、`libkvazaar`、`liboapv` 等。导出自定义命令可指定编码器，不额外增加本轮未要求的导出预设。上游构建保留旧编解码器，实际完整列表随包 `build-info/encoders.txt` 和 `decoders.txt`。

上游描述AVS3接入输出10bit；SVT采用修复后的simple分支，支持YUV444P10和自动关键帧，默认scd=1。上述来源与构建补丁保留在build-info；未对所有旧编码器或GPU编解码做运行验证。

实际构建包含 `--enable-gpl --enable-version3 --enable-nonfree`；不能将它标记为纯LGPL或自由可再分发版本。来源清单与构建记录不等于完整对应源码，本轮仅本地使用。

## 验证

- libuavs3e实际编码128×128四帧短片，libuavs3d完整解码四帧；ffprobe确认AVS3、尺寸和帧数，帧哈希见 `build/ffmpeg-enhanced-avs3-frames.md5`。
- `enhancedFfmpegAvs3Playback`等待原生异步加载完成后实际播放，并检查呈现帧数与尺寸。
- 新库下原生Lanczos4呈现、音频均衡器与声道策略、后台图片输出、图片缩放含Anime4K输出均通过。只运行五项相关用例，含Qt初始化/清理7/7；日志 `build/ffmpeg-enhanced-final-tests.txt`。
- 两处CLI启动与PNG输出通过；部署文件哈希与原始包一致。FFF.Native和两个EXE哈希未变；部署后即时检查player.ini未变，最终复核时便携player.ini已变化，保留其当前内容，没有回写旧配置。PowerShell部署/发布脚本语法检查通过。核对见 `build/ffmpeg-enhanced-delivery-audit.json`。

没有用当前机器负载下的耗时作性能结论；未运行无关全量回归。
