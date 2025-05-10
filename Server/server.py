import os
import socket
import numpy as np
import soundfile as sf
from openai import OpenAI
import json
import requests
import subprocess
import serial
import time
from collections import deque

# 加载配置文件
config = json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "config.json"), encoding='utf-8'))

# 初始化历史记录队列，用于存储对话历史
history = deque(maxlen=config["general"]["history_maxlen"])

class DeepSeekChatClient:
    """
    DeepSeek 大语言模型客户端，用于与 DeepSeek API 交互。
    """
    def __init__(self, api_key, base_url, model):
        """
        初始化 DeepSeekChatClient。

        Args:
            api_key (str): DeepSeek API 密钥。
            base_url (str): DeepSeek API 的基础 URL。
            model (str): 使用的 DeepSeek 模型名称。
        """
        self.client = OpenAI(api_key=api_key, base_url=base_url)
        self.model = model
        # 系统提示，指导 LLM 如何生成回复
        self.system_prompt = (
            "你是表情机器人的聊天机器人助手。\n"
            "根据用户的输入和历史聊天记录，生成恰当的回复并推荐合适的情绪。\n"
            "你的回复必须是 JSON 格式的字符串，包含以下三个字段：\n"
            "1. 'reply': (string) 你生成的回复内容，不超过100字。注意：不能出现emoji等非常规符号。\n"
            "2. 'emotion': (string) 你的回复作为语音输出时，需要表现的主要情绪 (为以下情绪之一，必须严格输出对应的字符串): 'happiness', 'sadness', 'anger', 'fear','disgust','surprise', 'neutral')。\n"
            "3. 'language': (string) 你生成的回复内容的语言 (为以下语言之一，必须严格输出对应的字符串): '中文', '英文', '日文', '韩文'。\n"
            "请确保输出严格符合 JSON 格式，例如：\n"
            "{\n"
            '  "reply": "好的，没问题！",\n'
            '  "emotion": "neutral",\n' # 添加了逗号，修正了原始示例中的JSON格式错误
            '  "language": "中文"\n'
            "}\n"
        )

    def send_message(self, user_message):
        """
        向 DeepSeek API 发送消息并获取回复。

        Args:
            user_message (str):用户的输入消息。

        Returns:
            str: LLM 生成的 JSON 格式的回复字符串。
        """
        # 如果用户输入过短，则返回预设回复
        if len(user_message) < 2:
            reply = {
                "reply": "请说完整的句子，我才能理解你的意思。",
                "emotion": "neutral",
                "language": "中文",
            }
            return json.dumps(reply, ensure_ascii=False)
        # 构建发送给 LLM 的消息列表，包含系统提示和历史记录
        messages = [
            {"role": "system", "content": self.system_prompt},
        ]
        for user_msg, bot_msg in history:
            messages.append({"role": "user", "content": user_msg})
            messages.append({"role": "assistant", "content": bot_msg})
        messages.append({"role": "user", "content": user_message})
        # 调用 DeepSeek API
        response = self.client.chat.completions.create(
            model=self.model, messages=messages, stream=False
        )
        return response.choices[0].message.content


# 初始化 DeepSeek 聊天客户端
chat_client = DeepSeekChatClient(
    api_key=config["deepseek_client"]["api_key"],
    base_url="https://api.deepseek.com",
    model="deepseek-chat",
)


def connect_sensevoice(host=config["sensevoice_server"]["host_socket"], port=config["sensevoice_server"]["port_socket"], max_retries=10, retry_delay=2):
    """
    连接到 SenseVoice ASR 服务器。

    Args:
        host (str): SenseVoice 服务器的主机地址。
        port (int): SenseVoice 服务器的端口号。
        max_retries (int): 最大重试次数。
        retry_delay (int): 重试间隔时间（秒）。

    Returns:
        socket.socket or None: 连接成功则返回 socket 对象，否则返回 None。
    """
    retries = 0
    while retries < max_retries:
        try:
            client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client_socket.connect((host, port))
            print(f"Successfully connected to SenseVoice server at {host}:{port}")
            return client_socket
        except socket.error as e:
            retries += 1
            print(
                f"Error connecting to SenseVoice server: {e}. Retrying ({retries}/{max_retries})..."
            )
            client_socket.close()  # 在重试前关闭套接字
            if retries < max_retries:
                time.sleep(retry_delay)
        except Exception as e:
            print(
                f"An unexpected error occurred during SenseVoice connection attempt: {e}"
            )
            # 确保在意外错误发生前部分创建的套接字已关闭
            if "client_socket" in locals() and isinstance(client_socket, socket.socket):
                client_socket.close()
            return None  # 发生意外错误时退出

    print(
        f"Failed to connect to SenseVoice server at {host}:{port} after {max_retries} attempts."
    )
    return None


def recv_exact(client_socket_sensevoice, length):
    """
    从 socket 接收指定长度的数据。

    Args:
        client_socket_sensevoice (socket.socket): SenseVoice 服务器的 socket 连接。
        length (int): 要接收的数据长度。

    Returns:
        bytearray: 接收到的数据。
    """
    data = bytearray()
    while len(data) < length:
        chunk = client_socket_sensevoice.recv(length - len(data))
        if not chunk: # 检查连接是否已关闭
            raise ConnectionError("Socket connection closed prematurely")
        data.extend(chunk)
    return data


def voice_to_text(client_socket_sensevoice):
    """
    通过 SenseVoice 服务将语音转换为文本。

    Args:
        client_socket_sensevoice (socket.socket): SenseVoice 服务器的 socket 连接。

    Returns:
        str: 识别出的文本。
    """
    command = 0x01  # SenseVoice ASR 命令
    client_socket_sensevoice.send(command.to_bytes(2, byteorder="little"))
    # 接收文本长度
    text_length_bytes = recv_exact(client_socket_sensevoice, 4)
    text_length = int.from_bytes(text_length_bytes, byteorder="little")
    # 接收文本内容
    text_bytes = recv_exact(client_socket_sensevoice, text_length)
    text = text_bytes.decode("utf-8")
    return text


def esp32_connect(host=config["esp32"]["host"], port=config["esp32"]["port"]):
    """
    启动服务器等待 ESP32 的连接。

    Args:
        host (str): 服务器主机地址。
        port (int): 服务器端口号。

    Returns:
        socket.socket: 与 ESP32 客户端建立的 socket 连接。
    """
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((host, port))
    print(f"Server started on {host}:{port}")
    server_socket.listen(1)
    print("Waiting for connection...")
    client_socket, addr = server_socket.accept()
    print(f"Connection from {addr}")
    return client_socket


def arduino_connect(
    port=config["arduino"]["port"], baudrate=config["arduino"]["baudrate"], timeout=1, max_retries=5, retry_delay=2
):
    """
    连接到 Arduino。

    Args:
        port (str): Arduino 连接的串口号。
        baudrate (int): 串口波特率。
        timeout (int): 串口读取超时时间。
        max_retries (int): 最大重试次数。
        retry_delay (int): 重试间隔时间（秒）。

    Returns:
        serial.Serial or None: 连接成功则返回 serial 对象，否则返回 None。
    """
    retries = 0
    while retries < max_retries:
        try:
            ser_connection = serial.Serial(port, baudrate, timeout=timeout)
            print(f"Successfully connected to Arduino on {port}")
            return ser_connection
        except serial.SerialException as e:
            retries += 1
            print(
                f"Error connecting to Arduino on {port}: {e}. Retrying ({retries}/{max_retries})..."
            )
            if retries < max_retries:
                time.sleep(retry_delay)
        except Exception as e:
            print(
                f"An unexpected error occurred during Arduino connection attempt: {e}"
            )
            return None  # 发生意外错误时退出

    print(f"Failed to connect to Arduino on {port} after {max_retries} attempts.")
    return None


def receive_sample(
    client_socket,
    voice_path=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "voice.wav"),
):
    """
    从 ESP32 接收音频采样数据。

    Args:
        client_socket (socket.socket): 与 ESP32 客户端的 socket 连接。
        voice_path (str): 保存接收到的音频文件的路径。
    """
    received_sample = bytearray()
    while True:
        # 接收数据类型
        type_byte = client_socket.recv(1)
        if not type_byte: # 检查连接是否已关闭
            print("ESP32 connection closed while receiving type.")
            break
        type = type_byte[0]
        # 接收数据长度
        length_bytes = client_socket.recv(4)
        if not length_bytes or len(length_bytes) < 4: # 检查连接是否已关闭或数据不完整
            print("ESP32 connection closed or incomplete length received.")
            break
        length = int.from_bytes(length_bytes, byteorder="little")

        if type == 0x01:  # 指令类型
            instruct_bytes = client_socket.recv(length)
            if not instruct_bytes or len(instruct_bytes) < length: # 检查连接是否已关闭或数据不完整
                print("ESP32 connection closed or incomplete instruction received.")
                break
            instruct = int.from_bytes(instruct_bytes, byteorder="little")
            if instruct == 0x0002:  # 音频结束指令
                break
        if type == 0x02:  # 音频数据类型
            sample_chunk = client_socket.recv(length)
            if not sample_chunk: # 检查连接是否已关闭
                print("ESP32 connection closed while receiving sample chunk.")
                break
            # 循环接收直到收满足够长度的数据
            while len(sample_chunk) < length:
                remaining_data = client_socket.recv(length - len(sample_chunk))
                if not remaining_data: # 检查连接是否已关闭
                    print("ESP32 connection closed while receiving remaining sample data.")
                    # 如果连接在接收数据中途关闭，可能需要处理不完整的数据
                    received_sample.extend(sample_chunk) # 添加已接收的部分
                    # 根据具体需求决定是否抛出异常或返回
                    return # 或者 raise ConnectionError("Connection closed prematurely")
                sample_chunk += remaining_data
            received_sample.extend(sample_chunk)
    print(f"接收音频数据长度: {len(received_sample)}")
    # 将接收到的字节数据转换为 NumPy 数组
    voice_sample = np.frombuffer(received_sample, dtype=np.int16)
    # 如果音频文件已存在，则删除
    if os.path.exists(voice_path):
        os.remove(voice_path)
    # 将音频数据写入 WAV 文件
    sf.write(voice_path, voice_sample, samplerate=16000)


def llm_process(text):
    """
    使用 LLM 处理文本并获取回复、情绪和语言。

    Args:
        text (str): 用户输入的文本。

    Returns:
        tuple: 包含回复文本 (str)、情绪 (str) 和语言 (str) 的元组。
    """
    response_json_str = chat_client.send_message(text)
    # 清理 LLM 可能返回的 markdown 代码块标记
    if response_json_str.startswith("```json"):
        response_json_str = response_json_str[len("```json") :]
    if response_json_str.endswith("```"):
        response_json_str = response_json_str[: -len("```")]
    response_json_str = response_json_str.strip()
    print(f"Response JSON: {response_json_str}")
    # 解析 JSON 字符串
    response_data = json.loads(response_json_str)
    reply = response_data["reply"]
    emotion = response_data["emotion"]
    language = response_data["language"]
    # 将当前对话添加到历史记录
    history.append((text, reply))
    return reply, emotion, language


def tts_process(text, language, url="http://127.0.0.1:9880/"):
    """
    使用 TTS 服务将文本转换为语音。

    Args:
        text (str): 要转换为语音的文本。
        language (str): 文本的语言。
        url (str): TTS 服务的 URL。

    Returns:
        bytes: PCM 格式的语音数据。
    """
    # GPT-soVITS 用于 TTS
    # 请求参数（JSON 格式）
    voice = config["tts_service"]["ref_voice_name"]
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "ref", voice + ".wav")

    params = {
        "refer_wav_path": path,
        "prompt_text": config["tts_service"]["ref_prompt_text"],
        "prompt_language": config["tts_service"]["ref_prompt_language"],
        "text": text,
        "text_language": language,
        "top_k": 15,
        "top_p": 1,
        "temperature": 1,
        "speed": 1,
    }
    # 发送 POST 请求到 TTS 服务
    response = requests.post(url, json=params)
    reply_voice = response.content
    # 使用 ffmpeg 将 TTS 输出转换为 16kHz 单声道 PCM S16LE 格式
    ffmpeg_cmd = [
        "ffmpeg",
        "-i",
        "pipe:0",  # 从 stdin 读取输入
        "-ar",
        "16000",  # 采样率 16kHz
        "-ac",
        "1",  # 单声道
        "-c:a",
        "pcm_s16le",  # 编码格式 PCM 16-bit
        "-f",
        "s16le",  # 输出 raw PCM S16LE 格式 (无文件头)
        "pipe:1",  # 输出到 stdout
    ]
    process = subprocess.Popen(
        ffmpeg_cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    reply_voice_pcm, _ = process.communicate(input=reply_voice)
    return reply_voice_pcm


def send_reply(client_socket, reply_voice, reply):
    """
    向 ESP32 发送回复语音和文本。

    Args:
        client_socket (socket.socket): 与 ESP32 客户端的 socket 连接。
        reply_voice (bytes): PCM 格式的回复语音数据。
        reply (str): 回复的文本内容。
    """
    # 发送语音数据长度
    length = len(reply_voice)
    client_socket.sendall(length.to_bytes(4, byteorder="little"))
    # 发送语音数据
    client_socket.sendall(reply_voice)
    # 将回复文本编码为 UTF-8
    reply_bytes = reply.encode("utf-8")
    # 发送文本数据长度
    length = len(reply_bytes)
    client_socket.sendall(length.to_bytes(4, byteorder="little"))
    # 发送文本数据
    client_socket.sendall(reply_bytes)
    print(f"发送回复长度: {len(reply_bytes)}") # 注意：这里打印的是文本长度，不是语音长度


# 情绪到 Arduino 控制指令的映射
emotion_dir = {
    "happiness": 0x11,
    "sadness": 0x12,
    "anger": 0x12,  # 注意：anger 和 sadness 映射到相同的值
    "fear": 0x13,
    "disgust": 0x10,
    "surprise": 0x13, # 注意：surprise 和 fear 映射到相同的值
    "neutral": 0x10,  # 注意：neutral 和 disgust 映射到相同的值
}


def map_value(value, from_min, from_max, to_min, to_max):
    """
    将一个值从一个范围映射到另一个范围。

    Args:
        value (float or int): 要映射的值。
        from_min (float or int): 原始范围的最小值。
        from_max (float or int): 原始范围的最大值。
        to_min (float or int): 目标范围的最小值。
        to_max (float or int): 目标范围的最大值。

    Returns:
        int: 映射后的整数值。
    """
    # 将值从一个范围映射到另一个范围
    val = (value - from_min) * (to_max - to_min) / (from_max - from_min) + to_min
    return int(val)


def main():
    """
    主函数，运行整个交互流程。
    """
    # 连接 ESP32
    client_socket = esp32_connect()
    if client_socket is None:
        print("Failed to connect to ESP32. Exiting.")
        return
    # 连接 Arduino
    arduino_serial = arduino_connect()
    if arduino_serial is None:
        print("Failed to connect to Arduino. Exiting.")
        if client_socket:
            client_socket.close()
        return
    # 连接 SenseVoice ASR 服务器
    client_socket_sensevoice = connect_sensevoice()
    if client_socket_sensevoice is None:
        print("Failed to connect to SenseVoice server. Exiting.")
        if client_socket:
            client_socket.close()
        if arduino_serial:
            arduino_serial.close()
        return

    try:
        while True:
            # 1. 从 ESP32 接收音频样本
            receive_sample(client_socket)

            # 2. 控制 Arduino 进入聆听状态 (示例性控制，具体含义需参考 Arduino 代码)
            arduino_serial.write(0x02.to_bytes(1, byteorder="little")) # 指令头
            arduino_serial.write(
                map_value(1, -1, 1, 80, 120).to_bytes(1, byteorder="little") # 舵机1角度
            )
            arduino_serial.write(
                map_value(0, -1, 1, 160, 180).to_bytes(1, byteorder="little") # 舵机2角度
            )

            # 3. 语音转文本
            text = voice_to_text(client_socket_sensevoice)
            print(f"识别文本: {text}")

            # 4. LLM 处理文本，生成回复和情绪
            reply, emotion, language = llm_process(text)

            # 5. 根据情绪控制 Arduino 表情 (示例性控制)
            emotion_value = emotion_dir.get(emotion, emotion_dir["neutral"]) # 如果情绪不存在，默认为 neutral
            arduino_serial.write(0x02.to_bytes(1, byteorder="little")) # 指令头
            arduino_serial.write(
                map_value(0, -1, 1, 60, 100).to_bytes(1, byteorder="little") # 舵机1角度
            )
            arduino_serial.write(
                map_value(0, -1, 1, 160, 180).to_bytes(1, byteorder="little") # 舵机2角度
            )
            arduino_serial.write(emotion_value.to_bytes(1, byteorder="little")) # 情绪指令

            # 6. 文本转语音
            reply_voice = tts_process(text=reply, language=language)
            print(f"回复语音长度: {len(reply_voice)}")

            # 7. 计算语音时长
            duration_ms = len(reply_voice) / 2 / 16000 * 1000 # PCM S16LE 每个采样点2字节，采样率16000Hz

            # 8. 向 ESP32 发送回复语音和文本
            send_reply(client_socket, reply_voice, reply)
            print("回复语音发送完成")

            # 9. 控制 Arduino 进入说话状态并等待语音播放完毕
            arduino_serial.write(0x21.to_bytes(1, byteorder="little")) # 开始说话指令
            time.sleep(duration_ms / 1000) # 等待语音播放
            arduino_serial.write(0x22.to_bytes(1, byteorder="little")) # 结束说话指令
            print("说话完成")

            # 10. 控制 Arduino 恢复默认表情
            arduino_serial.write(0x10.to_bytes(1, byteorder="little")) # 默认表情指令

            print(f"发送情绪: {emotion_value}")

    except KeyboardInterrupt:
        print("Server shutting down...")
    except ConnectionError as e:
        print(f"Connection error: {e}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
    finally:
        # 清理资源
        if client_socket:
            client_socket.close()
            print("ESP32 connection closed.")
        if arduino_serial:
            arduino_serial.close()
            print("Arduino connection closed.")
        if client_socket_sensevoice:
            client_socket_sensevoice.close()
            print("SenseVoice connection closed.")


if __name__ == "__main__":
    main()