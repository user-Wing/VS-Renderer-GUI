# AI 实时字幕组件

本轮使用 [Qwen3-ASR-0.6B](https://huggingface.co/Qwen/Qwen3-ASR-0.6B) 的 sherpa-onnx INT8 转换版，非名为“Qwen3.8”的模型。权重来源与文件列表见 [转换模型](https://huggingface.co/csukuangfj2/sherpa-onnx-qwen3-asr-0.6B-int8-2026-03-25)。独立包含 Python 3.12.10、numpy 2.5.4、sherpa-onnx/core 2.0.1、三个 ONNX 与 tokenizer，不要求系统 Python，不随 Full 主包重复携带权重。

1. 在下载页面安装 AI-Subtitles；默认位置是程序旁 `runtime/ai-subtitles`。维护者尚未上传前，可在字幕设置指定已解压组件目录。
2. 目录根必须有 `ai-subtitles.json`、`asr_worker.py`、`qwen3_ort.py`、`models/` 和 `runtime/python/`，JSON v2 使用相对路径。
3. 打开媒体后启用 AI 字幕。默认 CPU、5 秒窗口和步进，暂停停止输入，跳转按 generation 清空旧窗口，播放结束会提交剩余音频。
4. 运行时报告 RTF；大于 1 时停用并提示。CPU 官方日语样本已在独立 worker 和播放器音频 tap 中识别成功，RTF 约 0.14；这不是转写准确率或所有语种/设备的验收。

维护者上传 **`dist/AI-Subtitles-1.1.0.zip`** 到 `ARXChem/Software-List` 数据集的 **`AI-Subtitles/1.1.0/AI-Subtitles-1.1.0.zip`**。ZIP 根布局已适配组件下载器；无需再把三个 ONNX 分别上传。大小和 SHA-256 见同目录 `AI-Subtitles-1.1.0-upload.json`。本轮没有上传操作，远端列表尚不能据此宣称已可下载。

包内保留 `SOURCES.json`、`resource-manifest.json`、模型 Apache-2.0、sherpa-onnx 和 Python/依赖许可。Worker 同步自 FFF main `72af915f8bf2567155c1f87851a6fa263f73b14f`，VSP 补充同目录模块导入与末尾窗口提交。旧 JSON v1 的 GGUF/llama-server 路径仍可读取，相关文件需另行准备。字幕只转写，不提供翻译或逐词对齐。
