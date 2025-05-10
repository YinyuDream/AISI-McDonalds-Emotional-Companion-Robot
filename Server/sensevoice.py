import os
import socket
import json
from funasr import AutoModel
from funasr.utils.postprocess_utils import rich_transcription_postprocess

model_dir = "iic/SenseVoiceSmall"
config = json.load(os.path.join(os.path.dirname(os.path.abspath(__file__)), "config.json"))

print("Loading model...")

model = AutoModel(
    model=model_dir,
    trust_remote_code=True,
    remote_code=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "SenseVoice", "model.py"),
    device="cuda:0",
)

print("Model loaded successfully")

audio_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", "voice.wav")

def asr_process():
    res = model.generate(
        input=audio_path,
        cache={},
        language="auto",  # "zn", "en", "yue", "ja", "ko", "nospeech"
        use_itn=True,
        batch_size_s=60,
    )
    text = rich_transcription_postprocess(res[0]["text"])
    return text

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((config["sensevoice_server"]["host_server"], config["sensevoice_server"]["port_server"]))
server_socket.listen(1)
print("Waiting for connection...")
conn, addr = server_socket.accept()
print(f"Connected by {addr}")

def send_string(conn, message):
    """发送字符串（小端序长度头）"""
    encoded = message.encode('utf-8')
    conn.send(len(encoded).to_bytes(4, 'little'))  # 小端序长度头
    conn.sendall(encoded)

def main():
    while True:
        command = int.from_bytes(conn.recv(2), 'little')
        text = asr_process()
        send_string(conn, text)

if __name__ == "__main__":
    main()