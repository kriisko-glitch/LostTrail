"""
NeonPatrol Audio Bridge — STT via Groq Whisper + TTS via Windows SAPI
HTTP server on localhost:7777

Endpoints:
  POST /stt  — records mic for N seconds, transcribes via Groq Whisper, returns text
  POST /tts  — takes JSON {"text": "..."}, speaks it via Windows SAPI
  GET  /health — returns {"status": "ok"}

Start: python audio_bridge.py
Requires: pip install pyaudio (for mic recording)
"""

import http.server
import json
import os
import sys
import tempfile
import threading
import time
import urllib.request
import wave

# Try to import optional deps
try:
    import pyaudio
    HAS_PYAUDIO = True
except ImportError:
    HAS_PYAUDIO = False
    print("[audio_bridge] WARNING: pyaudio not installed. STT will not work.")
    print("[audio_bridge] Install with: pip install pyaudio")

# Groq API config
GROQ_KEY_PATH = os.path.join(os.path.dirname(__file__), "..", "..", "Kriisko-Studio", "tools", ".groq_key")
if not os.path.exists(GROQ_KEY_PATH):
    GROQ_KEY_PATH = "C:/Users/Kris/Kriisko-Studio/tools/.groq_key"

GROQ_API_KEY = ""
if os.path.exists(GROQ_KEY_PATH):
    with open(GROQ_KEY_PATH) as f:
        GROQ_API_KEY = f.read().strip()
    print(f"[audio_bridge] Groq API key loaded")
else:
    print(f"[audio_bridge] WARNING: Groq key not found at {GROQ_KEY_PATH}")

GROQ_STT_URL = "https://api.groq.com/openai/v1/audio/transcriptions"
WHISPER_MODEL = "whisper-large-v3-turbo"

# Audio recording config
RATE = 16000
CHANNELS = 1
CHUNK = 1024
RECORD_SECONDS = 5  # default recording duration


def record_audio(duration=RECORD_SECONDS):
    """Record audio from microphone, return WAV bytes."""
    if not HAS_PYAUDIO:
        return None

    pa = pyaudio.PyAudio()
    stream = pa.open(format=pyaudio.paInt16, channels=CHANNELS,
                     rate=RATE, input=True, frames_per_buffer=CHUNK)

    print(f"[audio_bridge] Recording {duration}s...")
    frames = []
    for _ in range(int(RATE / CHUNK * duration)):
        data = stream.read(CHUNK, exception_on_overflow=False)
        frames.append(data)

    stream.stop_stream()
    stream.close()
    pa.terminate()
    print(f"[audio_bridge] Recording complete")

    # Write to temp WAV file
    tmp = tempfile.NamedTemporaryFile(suffix=".wav", delete=False)
    wf = wave.open(tmp.name, 'wb')
    wf.setnchannels(CHANNELS)
    wf.setsampwidth(2)  # 16-bit
    wf.setframerate(RATE)
    wf.writeframes(b''.join(frames))
    wf.close()
    return tmp.name


def transcribe_audio(wav_path):
    """Send audio to Groq Whisper for transcription."""
    if not GROQ_API_KEY:
        return "(no API key)"

    boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW"

    with open(wav_path, 'rb') as f:
        wav_data = f.read()

    body = (
        f"--{boundary}\r\n"
        f'Content-Disposition: form-data; name="model"\r\n\r\n'
        f"{WHISPER_MODEL}\r\n"
        f"--{boundary}\r\n"
        f'Content-Disposition: form-data; name="file"; filename="audio.wav"\r\n'
        f"Content-Type: audio/wav\r\n\r\n"
    ).encode() + wav_data + f"\r\n--{boundary}--\r\n".encode()

    req = urllib.request.Request(
        GROQ_STT_URL,
        data=body,
        headers={
            "Authorization": f"Bearer {GROQ_API_KEY}",
            "Content-Type": f"multipart/form-data; boundary={boundary}",
            "User-Agent": "Kriisko-Studio/1.0",
        }
    )

    try:
        resp = urllib.request.urlopen(req, timeout=10)
        data = json.loads(resp.read())
        text = data.get("text", "").strip()
        print(f"[audio_bridge] Transcribed: {text}")
        return text
    except Exception as e:
        print(f"[audio_bridge] STT error: {e}")
        return f"(transcription failed: {e})"


def speak_text(text):
    """Speak text using Windows SAPI via PowerShell."""
    # Escape for PowerShell
    safe_text = text.replace("'", "''").replace('"', '`"')
    cmd = f'powershell.exe -Command "Add-Type -AssemblyName System.Speech; $s = New-Object System.Speech.Synthesis.SpeechSynthesizer; $s.Rate = 2; $s.Speak(\'{safe_text}\')"'
    os.system(cmd)
    print(f"[audio_bridge] Spoke: {text}")


class AudioHandler(http.server.BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        pass  # suppress default logging

    def do_GET(self):
        if self.path == "/health":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({
                "status": "ok",
                "stt": HAS_PYAUDIO,
                "tts": True
            }).encode())
        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        content_len = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_len) if content_len > 0 else b""

        if self.path == "/stt":
            # Record and transcribe
            try:
                params = json.loads(body) if body else {}
            except json.JSONDecodeError:
                params = {}
            duration = params.get("duration", RECORD_SECONDS)

            wav_path = record_audio(duration)
            if wav_path:
                text = transcribe_audio(wav_path)
                os.unlink(wav_path)
            else:
                text = "(microphone not available)"

            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"text": text}).encode())

        elif self.path == "/tts":
            try:
                params = json.loads(body)
                text = params.get("text", "")
            except json.JSONDecodeError:
                text = ""

            if text:
                # Speak in background thread so we don't block
                threading.Thread(target=speak_text, args=(text,), daemon=True).start()

            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"status": "speaking"}).encode())

        else:
            self.send_response(404)
            self.end_headers()


if __name__ == "__main__":
    port = 7777
    server = http.server.HTTPServer(("127.0.0.1", port), AudioHandler)
    print(f"[audio_bridge] Listening on http://127.0.0.1:{port}")
    print(f"[audio_bridge] STT: {'ready' if HAS_PYAUDIO else 'DISABLED (install pyaudio)'}")
    print(f"[audio_bridge] TTS: ready (Windows SAPI)")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[audio_bridge] Shutting down")
        server.shutdown()
