# DFPlayer Mini 语音模块

## 是什么

一个**MP3 播放器芯片模块**，能读 SD 卡里的 MP3 文件，通过小喇叭播放出来。让清洁小车能"说话"——开机打招呼、清扫时哼歌、到桌边说"好险"。

## 长什么样

```
大约 2cm × 2cm，和 TB6612 差不多大

    ┌──────────────┐
    │  DFPlayer    │
    │    Mini      │
    │  [SD卡槽]    │ ← 背面插 Micro SD 卡
    └──────────────┘
     VCC RX TX GND SPK+ SPK-
```

## 工作原理

```
SD 卡（装着 MP3 文件）
    ↓
DFPlayer Mini（读取 + 解码）
    ↓
小喇叭（播放声音）
    ↑
ESP32（通过串口发指令：播放第几首）
```

ESP32 只需要发一条串口指令（比如"播放第 3 首"），DFPlayer 就会自动从 SD 卡读取 `0003.mp3` 并通过喇叭播放。

## 怎么接线

```
DFPlayer Mini        ESP32 / 电源
┌──────────┐
│ VCC      │ ←── 5V（降压模块输出）
│ GND      │ ←── GND
│ RX       │ ←── GPIO 16（经 1kΩ 电阻）
│ TX       │ ──→ GPIO 17
│ SPK_1    │ ──→ 喇叭 +
│ SPK_2    │ ──→ 喇叭 -
└──────────┘
```

**为什么 RX 要接 1kΩ 电阻？** ESP32 输出 3.3V，DFPlayer 是 5V 器件，电阻做限流保护，防止电平不匹配烧模块。

## SD 卡怎么准备

1. Micro SD 卡格式化为 **FAT32**
2. 根目录建 `mp3` 文件夹
3. MP3 文件按 4 位数字命名：

```
SD 卡/
└── mp3/
    ├── 0001.mp3  ← 开机语音
    ├── 0002.mp3  ← 开始清扫
    ├── 0003.mp3
    └── ...
```

## 代码

```cpp
#include <DFRobotDFPlayerMini.h>

HardwareSerial dfpSerial(2);
DFRobotDFPlayerMini dfPlayer;

void setup() {
    dfpSerial.begin(9600, SERIAL_8N1, 17, 16);  // RX=17, TX=16
    dfPlayer.begin(dfpSerial);
    dfPlayer.volume(25);       // 音量 0–30
}

// 播放第 N 首
dfPlayer.play(1);   // 播放 0001.mp3
dfPlayer.play(10);  // 播放 0010.mp3
```

Arduino IDE 安装库：库管理器 → 搜索 `DFRobotDFPlayerMini` → 安装。

## 配套的小喇叭

直接接 DFPlayer 的 SPK+/SPK- 引脚，不需要功放。
- 规格：**8Ω 2W**，直径 28–40mm
- 淘宝搜 `8欧2瓦 小喇叭 28mm`，2–3 元
- 声音不大但桌面距离够用，比赛展示完全没问题

## 选购

- DFPlayer Mini：淘宝搜 `DFPlayer Mini MP3 模块`，6–8 元
- Micro SD 卡：`TF卡 2G` 或 `Micro SD 卡 1G`，8–10 元（1G 绰绰有余）
- 小喇叭：`8欧2瓦 小喇叭`，2–3 元

## 一句话总结

**DFPlayer Mini 就是一个 2cm 大的 MP3 播放器**，插 SD 卡、接小喇叭，ESP32 发一条指令就能播放对应语音。让清洁小车从"哑巴"变成会说话的小管家。
