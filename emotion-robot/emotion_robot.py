#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
情绪安慰小机器人 · 树莓派版
用摄像头识别人脸表情，用喇叭播放安慰语音。
"""

import os
import sys
import time
import subprocess
from pathlib import Path

import cv2
import numpy as np

# 摄像头分辨率（树莓派可改小一点，如 320 240）
CAMERA_WIDTH = 640
CAMERA_HEIGHT = 480
# 每隔多少秒检测一次情绪并可能说话（避免一直说）
CHECK_INTERVAL_SEC = 12
# 同一情绪至少隔多久才再播放
SAY_COOLDOWN_SEC = 20

# 情绪 → 中文安慰语（没有预录 WAV 时用 TTS 读这些）
COMFORT_MESSAGES = {
    "angry": "别生气啦，深呼吸一下，会好起来的。",
    "sad": "难过了就休息一下，你已经很棒了。",
    "fear": "不怕不怕，我在这里陪着你。",
    "disgust": "没事的，放松一下。",
    "surprise": "哈哈，被吓到了吗？",
    "neutral": "你好呀，今天怎么样？",
    "happy": "看到你开心我也开心！",
}

# 情绪 → 预录音文件名（放在 sounds/ 下）
EMOTION_TO_WAV = {
    "angry": "angry.wav",
    "sad": "sad.wav",
    "fear": "fear.wav",
    "disgust": "neutral.wav",
    "surprise": "neutral.wav",
    "neutral": "neutral.wav",
    "happy": "happy.wav",
}

# 需要「安慰」的情绪（这些才触发说话）
COMFORT_EMOTIONS = {"angry", "sad", "fear"}

# 项目根目录
PROJECT_DIR = Path(__file__).resolve().parent
SOUNDS_DIR = PROJECT_DIR / "sounds"


def try_import_fer():
    try:
        from fer import FER
        return FER(mt=False)  # 单脸更快
    except Exception as e:
        print(f"[提示] 未安装或无法加载 FER 表情识别库: {e}", file=sys.stderr)
        print("       将使用「检测到人脸就打招呼」的演示模式。安装: pip install fer", file=sys.stderr)
        return None


def play_wav(path: Path) -> bool:
    """用 aplay 播放 WAV（树莓派/Linux 常见）。"""
    if not path.exists():
        return False
    try:
        subprocess.run(
            ["aplay", "-q", str(path)],
            check=True,
            timeout=30,
            capture_output=True,
        )
        return True
    except (subprocess.CalledProcessError, FileNotFoundError, subprocess.TimeoutExpired):
        return False


def speak_tts(text: str) -> bool:
    """用 espeak 或 pyttsx3 读中文。"""
    # 先试 espeak（树莓派常带）
    for cmd in [
        ["espeak", "-v", "zh", text],
        ["espeak", "-v", "zh+f3", text],
        ["espeak", text],
    ]:
        try:
            subprocess.run(cmd, check=True, timeout=15, capture_output=True)
            return True
        except (FileNotFoundError, subprocess.CalledProcessError, subprocess.TimeoutExpired):
            continue
    # 再试 pyttsx3
    try:
        import pyttsx3
        engine = pyttsx3.init()
        engine.say(text)
        engine.runAndWait()
        return True
    except Exception:
        pass
    return False


def say_emotion(emotion: str, last_say_time: float) -> float:
    """根据情绪播放语音；返回本次若已播放则为当前时间，否则为 last_say_time。"""
    if time.time() - last_say_time < SAY_COOLDOWN_SEC:
        return last_say_time

    wav_name = EMOTION_TO_WAV.get(emotion, "neutral.wav")
    wav_path = SOUNDS_DIR / wav_name
    if play_wav(wav_path):
        return time.time()
    text = COMFORT_MESSAGES.get(emotion, COMFORT_MESSAGES["neutral"])
    if speak_tts(text):
        return time.time()
    return last_say_time


def get_top_emotion(emotions_dict: dict) -> str:
    if not emotions_dict:
        return "neutral"
    return max(emotions_dict, key=emotions_dict.get)


def main():
    SOUNDS_DIR.mkdir(exist_ok=True)

    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("无法打开摄像头，请检查是否已连接并启用。", file=sys.stderr)
        sys.exit(1)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, CAMERA_WIDTH)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, CAMERA_HEIGHT)

    detector = try_import_fer()
    use_fer = detector is not None
    if not use_fer:
        face_cascade = cv2.CascadeClassifier(
            cv2.data.haarcascades + "haarcascade_frontalface_default.xml"
        )

    last_check_time = 0
    last_say_time = 0
    current_emotion = "neutral"

    print("情绪机器人已启动。请面对摄像头，按 Q 退出。")
    print("当识别到难过/生气/害怕时会播放安慰语。\n")

    while True:
        ret, frame = cap.read()
        if not ret:
            break
        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

        now = time.time()
        if now - last_check_time >= CHECK_INTERVAL_SEC:
            last_check_time = now
            if use_fer:
                try:
                    results = detector.detect_emotions(rgb)
                    if results:
                        emotions = results[0].get("emotions", {})
                        current_emotion = get_top_emotion(emotions)
                        # 需要安慰的情绪才说话
                        if current_emotion in COMFORT_EMOTIONS or current_emotion == "neutral":
                            last_say_time = say_emotion(current_emotion, last_say_time)
                except Exception as e:
                    print(f"[检测异常] {e}", file=sys.stderr)
            else:
                gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
                faces = face_cascade.detectMultiScale(gray, 1.1, 5)
                if len(faces) > 0:
                    last_say_time = say_emotion("neutral", last_say_time)

        # 在画面上显示当前情绪
        label = f"Emotion: {current_emotion}"
        cv2.putText(
            frame, label, (20, 40),
            cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2
        )
        cv2.imshow("Emotion Robot", frame)

        if cv2.waitKey(1) & 0xFF == ord("q"):
            break

    cap.release()
    cv2.destroyAllWindows()
    print("已退出。")


if __name__ == "__main__":
    main()
