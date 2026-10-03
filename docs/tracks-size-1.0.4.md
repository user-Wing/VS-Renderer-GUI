# 1.0.4 轨道信息与部署体积（2026-10-03）

## 轨道

菜单显示 `Track 0:a:0 - 名称 - 语言 - Default/Forced/Default+Forced`，字幕使用 `0:s:0`。同类编号从0递增，点击映射到真实全局流编号；编码格式、声道数和全局编号放在提示中。无标记显示None，无标题使用编码格式，无语言使用und；可用时优先显示language-ietf，不根据轨道标题猜语言。

后端原本提供metadata/default/forced，菜单却读取tags，导致标题和语言缺失。现在优先读取metadata并兼容旧tags，无需引入MKVToolNix。

多个Default选择同类全局流编号最小者，Forced不参与默认选择；无Default时音频和字幕均选择同类首条（0:a:0 / 0:s:0）。默认选择在自动播放前完成，手动切换仍有效。已挂载/自动匹配的外部轨道继续使用现有优先规则，字幕显示开关保留。

## 部署与体积

按维护者指定的空间分析区分程序文件和用户缓存，不用SFX整包展开，不把整个程序解压到内存。

- FFmpeg CLI位于程序根目录，与3FP共用同一组DLL；runtime/ffmpeg仅保留README/build-info，减少约188 MiB重复运行时。Player图片兜底解码、截图、Anime4K输出、字幕提取和Renderer导出均使用新位置；导出兼容旧隔离布局及显式绝对路径。
- 915份GLSL/HOOK原始源码在开发目录保留，发行目录使用约59 MiB的 `shaders/mpv-shaders.7z`：LZMA2、64 MiB固实块、32 MiB字典。清单和许可证仍为普通文件，菜单无需解压。VPY只通过随包7-Zip读取所选文本并传shader_s，不建立整库临时目录；VS解释器缓存最近一份文本，按归档mtime更新。自定义普通GLSL直接读取。
- 基础Anime/Jinc/D3D11的普通着色器保持原路径。手写VPY若把压缩库的旧路径直接传给placebo的shader文件参数，需提取相应文件或用Renderer重新生成VPY；当前随包/用户VPY未发现这种引用。
- 本项目VS core以enable_x86_asm=false构建，不编译自动ISA选择分支；实际确认std/zsmooth/vszip加载基线DLL，只移走当前不会使用的AVX2/ZN4副本，保留所有滤镜与RIFE模型。裁剪脚本先验证模块选择，检测到优化版正在使用则保留ISA副本。
- x64播放器加载madVR64.ax；只移走两个debug AX和32位madVR.ax，设置控制程序、对应依赖和settings.bin保留。移走文件暂留build备份，清理BAT可由维护者删除。

程序文件约596 MiB / 625 MB，现有用户索引缓存约34 MiB保留，整个便携目录约630 MiB / 661 MB。数值是普通文件长度，不是归档大小或NTFS压缩占用。没有删除有效缓存来虚报程序精简；后续缓存增长会改变目录总量。

逐帧使用的DLL、模型及Shader数学内容不变。额外读取发生在首次加载所选压缩Shader时，最坏解码所在固实块；缓存命中不再启动解压进程。未在当前高负载环境上作FPS或冷启动性能结论。

## 构建与验证

stage-ffmpeg.ps1部署共享DLL布局，stage-shaders.ps1生成并校验资源库，trim-portable.ps1验证实际模块并裁剪；stage-portable.ps1调用三者。已有压缩库时CMake不再复制原始整库；修改Shader库源码后应重新执行stage-shaders并复查。

真实MKV三音轨/三字幕轨、中文名称/语言、多Default与Forced组合的标签、自动选择、手动切换通过。压缩库实际Vulkan出帧、共享FFmpeg AVS3编解码/呈现、图片算法和Anime4K输出、外部轨道、应用设置保留状态通过；GLSL分类/脚本生成及旧导出CLI兼容仅跑相关专项。裁剪后便携Python的std、zsmooth.Median、vszip.BoxBlur实际出帧和完整Shader归档CRC检查通过。

日志：build/track-size-final-tests.txt、build/compressed-shader-final.txt、build/track-size-graph-tests.txt、build/track-size-export-tests.txt。配置、预设、大小和部署哈希见build/track-size-delivery-audit.json。保持1.0.4，不推送GitHub，不生成完整发行归档。
