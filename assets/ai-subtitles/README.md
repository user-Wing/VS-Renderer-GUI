# AI 实时字幕组件

模型、Python 依赖和推理后端由用户手动准备，不随 VS-GUI 分发。

1. 从 [ggml-org 模型目录](https://huggingface.co/ggml-org/Qwen3-ASR-0.6B-GGUF/tree/main) 下载成对的主权重与 mmproj；不要混用其他推理引擎的 GGUF 格式。
2. 从 [llama.cpp Releases](https://github.com/ggml-org/llama.cpp/releases) 准备支持音频转写的 `llama-server.exe`。NVIDIA 设备选择 CUDA 包；其他设备按实际后端选择。
3. 在独立 Python 环境安装 `numpy requests soundfile`。将本目录复制到自己的 AI 组件目录，把 `ai-subtitles.example.json` 改名为 `ai-subtitles.json`，填写五个文件路径。相对路径以这个 JSON 所在目录为准。
4. 在 Player 设置→字幕设置选择该目录；打开媒体，再从右键→字幕设置启用 AI 实时字幕。

启动会检查缺失文件，并由 worker 探测实际推理后端。运行两次推理后报告 RTF；RTF 大于 1 时停止 AI 字幕并提示更换模型/后端。音频环和进程输入均有上限，识别不阻塞播放。

字幕是语音识别结果，不进行翻译。切换媒体停止识别，跳转通过内核 generation 清除旧字幕。暂停停止输入；恢复后继续。识别有窗口延迟，不等同于离线字幕的逐帧准确同步。

Worker 基于 3FP PR #37 的 `tools/ai-subtitles/asr_worker.py`（`edf533155727f681eefa771082702bab275bca48`）；VSP 增加整数/双精度 PCM 转换及初次性能报告；RTF 排除服务首次加载时间。许可见 `UPSTREAM-LICENSE.txt`。未配置模型时只能验证接口、缺失检测与进程协议，不能证明识别质量或实际实时性。
