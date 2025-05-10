# SenseVoice

「简体中文」|「[English](./README.md)」|「[日本語](./README_ja.md)」

SenseVoice 是具有音频理解能力的音频基础模型，包括语音识别（ASR）、语种识别（LID）、语音情感识别（SER）和声学事件分类（AEC）或声学事件检测（AED）。本项目提供 SenseVoice 模型的介绍以及在多个任务测试集上的 benchmark，以及体验模型所需的环境安装的与推理方式。

<div align="center">  
<img src="image/sensevoice2.png">
</div>

<div align="center">  
<h4>
<a href="https://funaudiollm.github.io/"> Homepage </a>
｜<a href="#最新动态"> 最新动态 </a>
｜<a href="#性能评测"> 性能评测 </a>
｜<a href="#环境安装"> 环境安装 </a>
｜<a href="#用法教程"> 用法教程 </a>
｜<a href="#联系我们"> 联系我们 </a>

</h4>

模型仓库：[modelscope](https://www.modelscope.cn/models/iic/SenseVoiceSmall)，[huggingface](https://huggingface.co/FunAudioLLM/SenseVoiceSmall)

在线体验：
[modelscope demo](https://www.modelscope.cn/studios/iic/SenseVoice), [huggingface space](https://huggingface.co/spaces/FunAudioLLM/SenseVoice)

</div>

<a name="核心功能"></a>

# 核心功能 🎯

**SenseVoice** 专注于高精度多语言语音识别、情感辨识和音频事件检测

- **多语言识别：** 采用超过 40 万小时数据训练，支持超过 50 种语言，识别效果上优于 Whisper 模型。
- **富文本识别：**
  - 具备优秀的情感识别，能够在测试数据上达到和超过目前最佳情感识别模型的效果。
  - 支持声音事件检测能力，支持音乐、掌声、笑声、哭声、咳嗽、喷嚏等多种常见人机交互事件进行检测。
- **高效推理：** SenseVoice-Small 模型采用非自回归端到端框架，推理延迟极低，10s 音频推理仅耗时 70ms，15 倍优于 Whisper-Large。
- **微调定制：** 具备便捷的微调脚本与策略，方便用户根据业务场景修复长尾样本问题。
- **服务部署：** 具有完整的服务部署链路，支持多并发请求，支持客户端语言有，python、c++、html、java 与 c# 等。

<a name="最新动态"></a>

# 最新动态 🔥

- 2024/7：新增加导出 [ONNX](./demo_onnx.py) 与 [libtorch](./demo_libtorch.py) 功能，以及 python 版本 runtime：[funasr-onnx-0.4.0](https://pypi.org/project/funasr-onnx/)，[funasr-torch-0.1.1](https://pypi.org/project/funasr-torch/)
- 2024/7: [SenseVoice-Small](https://www.modelscope.cn/models/iic/SenseVoiceSmall) 多语言音频理解模型开源，支持中、粤、英、日、韩语的多语言语音识别，情感识别和事件检测能力，具有极低的推理延迟。。
- 2024/7: CosyVoice 致力于自然语音生成，支持多语言、音色和情感控制，擅长多语言语音生成、零样本语音生成、跨语言语音克隆以及遵循指令的能力。[CosyVoice repo](https://github.com/FunAudioLLM/CosyVoice) and [CosyVoice 在线体验](https://www.modelscope.cn/studios/iic/CosyVoice-300M).
- 2024/7: [FunASR](https://github.com/modelscope/FunASR) 是一个基础语音识别工具包，提供多种功能，包括语音识别（ASR）、语音端点检测（VAD）、标点恢复、语言模型、说话人验证、说话人分离和多人对话语音识别等。

<a name="Benchmarks"></a>

# 性能评测 📝

## 多语言语音识别

我们在开源基准数据集（包括 AISHELL-1、AISHELL-2、Wenetspeech、Librispeech 和 Common Voice）上比较了 SenseVoice 与 Whisper 的多语言语音识别性能和推理效率。在中文和粤语识别效果上，SenseVoice-Small 模型具有明显的效果优势。

<div align="center">  
<img src="image/asr_results1.png" width="400" /><img src="image/asr_results2.png" width="400" />
</div>

## 情感识别

由于目前缺乏被广泛使用的情感识别测试指标和方法，我们在多个测试集的多种指标进行测试，并与近年来 Benchmark 上的多个结果进行了全面的对比。所选取的测试集同时包含中文 / 英文两种语言以及表演、影视剧、自然对话等多种风格的数据，在不进行目标数据微调的前提下，SenseVoice 能够在测试数据上达到和超过目前最佳情感识别模型的效果。

<div align="center">  
<img src="image/ser_table.png" width="1000" />
</div>

同时，我们还在测试集上对多个开源情感识别模型进行对比，结果表明，SenseVoice-Large 模型可以在几乎所有数据上都达到了最佳效果，而 SenseVoice-Small 模型同样可以在多数数据集上取得超越其他开源模型的效果。

<div align="center">  
<img src="image/ser_figure.png" width="500" />
</div>

## 事件检测

尽管 SenseVoice 只在语音数据上进行训练，它仍然可以作为事件检测模型进行单独使用。我们在环境音分类 ESC-50 数据集上与目前业内广泛使用的 BEATS 与 PANN 模型的效果进行了对比。SenseVoice 模型能够在这些任务上取得较好的效果，但受限于训练数据与训练方式，其事件分类效果专业的事件检测模型相比仍然有一定的差距。

<div align="center">  
<img src="image/aed_figure.png" width="500" />
</div>

## 推理效率

SenseVoice-small 模型采用非自回归端到端架构，推理延迟极低。在参数量与 Whisper-Small 模型相当的情况下，比 Whisper-Small 模型推理速度快 5 倍，比 Whisper-Large 模型快 15 倍。同时 SenseVoice-small 模型在音频时长增加的情况下，推理耗时也无明显增加。

<div align="center">  
<img src="image/inference.png" width="1000" />
</div>

<a name="环境安装"></a>

# 安装依赖环境 🐍

```shell
pip install -r requirements.txt
```

<a name="用法教程"></a>

# 用法 🛠️

## 推理

### 使用 funasr 推理

支持任意格式音频输入，支持任意时长输入

```python
from funasr import AutoModel
from funasr.utils.postprocess_utils import rich_transcription_postprocess

model_dir = "iic/SenseVoiceSmall"


model = AutoModel(
    model=model_dir,
    trust_remote_code=True,
    remote_code="./model.py",  
    vad_model="fsmn-vad",
    vad_kwargs={"max_single_segment_time": 30000},
    device="cuda:0",
)

# en
res = model.generate(
    input=f"{model.model_path}/example/en.mp3",
    cache={},
    language="auto",  # "zh", "en", "yue", "ja", "ko", "nospeech"
    use_itn=True,
    batch_size_s=60,
    merge_vad=True,
    merge_length_s=15,
)
text = rich_transcription_postprocess(res[0]["text"])
print(text)
```

<details><summary> 参数说明（点击展开）</summary>

- `model_dir`：模型名称，或本地磁盘中的模型路径。
- `trust_remote_code`：
  - `True` 表示 model 代码实现从 `remote_code` 处加载，`remote_code` 指定 `model` 具体代码的位置（例如，当前目录下的 `model.py`），支持绝对路径与相对路径，以及网络 url。
  - `False` 表示，model 代码实现为 [FunASR](https://github.com/modelscope/FunASR) 内部集成版本，此时修改当前目录下的 `model.py` 不会生效，因为加载的是 funasr 内部版本，模型代码 [点击查看](https://github.com/modelscope/FunASR/tree/main/funasr/models/sense_voice)。
- `vad_model`：表示开启 VAD，VAD 的作用是将长音频切割成短音频，此时推理耗时包括了 VAD 与 SenseVoice 总耗时，为链路耗时，如果需要单独测试 SenseVoice 模型耗时，可以关闭 VAD 模型。
- `vad_kwargs`：表示 VAD 模型配置，`max_single_segment_time`: 表示 `vad_model` 最大切割音频时长，单位是毫秒 ms。
- `use_itn`：输出结果中是否包含标点与逆文本正则化。
- `batch_size_s` 表示采用动态 batch，batch 中总音频时长，单位为秒 s。
- `merge_vad`：是否将 vad 模型切割的短音频碎片合成，合并后长度为 `merge_length_s`，单位为秒 s。
- `ban_emo_unk`：禁用 emo_unk 标签，禁用后所有的句子都会被赋与情感标签。默认 `False`

</details>

如果输入均为短音频（小于 30s），并且需要批量化推理，为了加快推理效率，可以移除 vad 模型，并设置 `batch_size`

```python
model = AutoModel(model=model_dir, trust_remote_code=True, device="cuda:0")

res = model.generate(
    input=f"{model.model_path}/example/en.mp3",
    cache={},
    language="auto", # "zh", "en", "yue", "ja", "ko", "nospeech"
    use_itn=True,
    batch_size=64, 
)
```

更多详细用法，请参考 [文档](https://github.com/modelscope/FunASR/blob/main/docs/tutorial/README.md)

### 直接推理

支持任意格式音频输入，输入音频时长限制在 30s 以下

```python
from model import SenseVoiceSmall
from funasr.utils.postprocess_utils import rich_transcription_postprocess

model_dir = "iic/SenseVoiceSmall"
m, kwargs = SenseVoiceSmall.from_pretrained(model=model_dir, device="cuda:0")
m.eval()

res = m.inference(
    data_in=f"{kwargs ['model_path']}/example/en.mp3",
    language="auto", # "zh", "en", "yue", "ja", "ko", "nospeech"
    use_itn=False,
    ban_emo_unk=False,
    **kwargs,
)

text = rich_transcription_postprocess(res [0][0]["text"])
print(text)
```
