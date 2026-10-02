# VS Player 1.0.3：独立图片与右键菜单

日期：2026-10-02。只更新本地源码与程序，版本保持1.0.3。

更正：首次 `build/supplied-large-grid.avif` 样本在编码阶段已经出现顶部错色和底部黑块，下面原有“打开、显示检查通过”只代表流程及尺寸验证，不能证明像素内容完整。原图对比及新的YUV420样本见 `player-apply-avif-1.0.3.md`；不再使用首次样本证明完整AVIF支持。

## 图片路径

此前本地图片与视频共用FFMS2/VS源流程，JPG被当成视频帧序列。现在在VS初始化等待之前识别本地图片，单独通过PlayerImage后台线程解码；显示交给PreviewPane的QImage绘制模式。不会创建视频索引、执行VPY滤镜、请求视频帧或预解码相邻图片。打开视频时恢复原生表面与视频控制。

打开一张图片后，以同文件夹可识别图片的名称排序重建列表；左右键切换、滚轮缩放、放大后拖动平移，左上角还原按钮恢复默认适配。视频滚轮设置保持原值。Qt优先读取并应用EXIF方向，未提供的图片codec转交随包FFmpeg；GIF/APNG等当前只显示第一帧。解码失败显示错误，切图后旧结果不会覆盖当前文件。

AVIF直接静态链接libavif1.4.2与dav1d1.5.3。Unicode路径由QFile流式IO读取，支持grid、高位深、透明、旋转与镜像。10/12位保存为RGBA64。官方libavif1.4.2不仅CLI，API也拒绝超过268435456像素的显式上限；构建时对这一处guard受控修改，Player将像素上限设置为UINT32_MAX、边长上限设为0，保留尺寸除法、grid及AV1验证。直接向QImage转换，不生成巨大临时PNG、不上传为受边长限制的视频纹理。依赖源码与dav1d静态库的版本和SHA-256固定在tools/image-runtime.cmake，随包许可证为IMAGE-LICENSE.txt。

这是当前图像的必要解码，并非视频预解码。完整原图仍占用相应内存；本机为64GiB级内存。内存不足时解码会失败；没有通过缩小图片规避原尺寸。

## 右键菜单

PlayerMenu保留QMenu的菜单动作、键盘与子菜单机制，统一应用到主级及所有子级。显示时采用180ms三次缓出揭示，PreciseTimer每8ms更新；实际呈现频率取决于桌面刷新率和负载。透明窗口绘制8px抗锯齿圆角，不只修改stylesheet的border-radius。文字从行左48px开始，16px方框的中心位于24px处，恰在左缘与文字间；单选与布尔动作都显示边框和明确勾选，禁用项变灰。

## 验证证据

- Release构建：build/image-menu-build.txt。
- 专项：build/image-menu-targeted.txt，6项通过、0失败、0跳过（含初始化/清理）。覆盖JPG大写后缀、PNG、WebP、BMP、左右排序、默认音量滚轮被图片缩放替代、坏JPG恢复、图片→视频→图片；另覆盖TIFF、JPEG XL、GIF、JPEG2000、TGA。
- AVIF固定小样本：tests/fixtures/wide-grid.avif为40000×64、8×1 grid；alpha-10bit.avif为128×96透明10位。验证尺寸、alpha、连续切图旧结果丢弃。
- 菜单：180ms过程中可观测中间状态，主/子菜单都完成展开；实际截图build/menu-check-test.png已检查圆角、蓝色方框及居中位置。
- 用户JPG：`C:/Users/ARXChem/Downloads/にんげんまめ￤2日目東ア-31_pid112201257_アリスp0.jpg`，文件1809331931字节，31603×65278，共2062980634像素；保持完整尺寸打开、显示画面颜色变化、缩放/平移检查通过。测试进程总耗时22053ms，QImage像素缓冲8251922536字节。日志build/image-menu-large-jpg.txt。
- 同源完整AVIF：build/supplied-large-grid.avif，205270684字节，8×16 grid，4:4:4、8位，原尺寸不变。为避免原JPG奇数宽度违反4:2:0 grid限制，采用4:4:4，不裁切原图。打开、显示、缩放与平移检查通过，专项总耗时9348ms；日志build/image-menu-large-avif.txt。测试AVIF由官方avifenc/aom生成，不冒称由zenrav1e编码；zenrav1e属于编码器，解码不依赖编码器品牌。
- 用户报告部分损坏的TEST.avif：81355277字节，解码结果32768×21845，像素缓冲5726535680字节，打开、显示、缩放检查通过，专项3370ms。日志build/image-menu-damaged-avif.txt。解码器未报告结构/码流错误，这不能证明图片内容完好，也不会恢复已损坏的像素；以完整同源AVIF验证尺寸支持。

- 共同完整回归：build/image-menu-ctest.txt，八组中七组首次通过；导出组一次失败，单独明细复测8项通过（image-menu-export-retest.txt），CTest导出单组再验通过（image-menu-export-ctest.txt），未改导出代码。Player完整组36通过、0失败、7跳过；跳过为未提供环境变量的可选外部/远程素材及巨图专项，巨图已通过上列独立实测。六档Anime用例同时通过，非本轮图片功能范围。


- 本地交付：执行一次stage-portable，更新dist/VS-Renderer-GUI-windows-x64。交付目录图片/菜单/原有还原按钮专项7通过、0失败、0跳过（image-menu-portable-tests.txt）。两正式EXE与FFF.Native.dll均与构建哈希一致；INI及用户VPY不变。共同六档工作使已知旧默认builtin/Anime.vpy在首次启动定向迁移，.before-six-stage备份哈希与部署前原件一致。临时测试EXE与Qt6Test.dll已移出正式目录；完整图片许可和本说明随包。审计为image-menu-delivery-audit.json。不打包、不推送、不发布。
