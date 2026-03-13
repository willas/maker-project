# LED 指示灯 + 蜂鸣器 + 按钮

三个最简单的元件，直接接 ESP32 GPIO 就能用，不需要驱动板。

---

## LED 指示灯

### 是什么

就是发光二极管，用来显示小车状态：绿灯 = 正在清扫，红灯 = 待机/停止。

### 怎么接

```
ESP32 GPIO 2  ──→ [220Ω电阻] ──→ 绿色LED长脚(+) ──→ LED短脚(-) ──→ GND
ESP32 GPIO 4  ──→ [220Ω电阻] ──→ 红色LED长脚(+) ──→ LED短脚(-) ──→ GND
```

中间串一个 **220Ω 电阻**限流（防止烧 LED）。如果买的是「LED 模块」（小板子上已经焊好电阻），直接接 GPIO 和 GND 就行。

### 代码

```cpp
#define LED_GREEN 2
#define LED_RED   4

digitalWrite(LED_GREEN, HIGH);  // 绿灯亮
digitalWrite(LED_RED, LOW);     // 红灯灭
```

### 选购

淘宝搜 `LED 红绿 5mm` 或 `LED 模块 Arduino`，几毛钱一个。

---

## 有源蜂鸣器

### 是什么

给电就响的小喇叭（"有源"= 自带振荡电路，不需要 ESP32 产生音频信号）。用来开机提示、清扫完成时"嘀嘀"。

### 和"无源"蜂鸣器的区别

| | 有源蜂鸣器 | 无源蜂鸣器 |
|--|---|---|
| 使用方式 | 给电就响（HIGH/LOW） | 需要 ESP32 发不同频率脉冲 |
| 能不能调音调 | 不能，固定一个音 | **能**，可以播放简单旋律 |
| 难度 | **最简单** | 稍复杂 |

清洁小车已经有 DFPlayer 播放语音了，蜂鸣器只是备用提示，选**有源**的最简单。

### 怎么接

```
ESP32 GPIO 23  ──→  蜂鸣器 SIG(+)
                    蜂鸣器 GND(-) ──→ GND
```

如果是裸蜂鸣器（不是模块），长脚接 GPIO，短脚接 GND。

### 代码

```cpp
#define BUZZER_PIN 23

// 响 200 毫秒
tone(BUZZER_PIN, 1000, 200);

// 也可以直接：
digitalWrite(BUZZER_PIN, HIGH);  // 响
delay(200);
digitalWrite(BUZZER_PIN, LOW);   // 停
```

### 选购

淘宝搜 `有源蜂鸣器模块 Arduino`，2 元。选模块版（带 3 针排针：VCC/GND/SIG）更方便。

---

## 轻触按钮（启动/停止）

### 是什么

按一下开始清扫，再按一下停止。最基础的输入元件。

### 长什么样

```
    ┌───┐
    │   │  ← 按下去 = 接通
    │ ○ │     松开 = 断开
    └─┬─┘
    两根脚
```

### 怎么接

```
ESP32 GPIO 5  ──→  按钮一端
                   按钮另一端 ──→ GND
```

代码里开启 ESP32 的**内部上拉电阻**，不需要外接电阻：
- 没按下：GPIO 5 读到 HIGH（被内部电阻拉高）
- 按下了：GPIO 5 读到 LOW（被按钮接到 GND）

### 代码

```cpp
#define BUTTON_PIN 5

void setup() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // 开启内部上拉
}

bool buttonPressed() {
    if (digitalRead(BUTTON_PIN) == LOW) {  // LOW = 被按下
        delay(50);                          // 消抖（等 50ms 再读一次）
        if (digitalRead(BUTTON_PIN) == LOW) {
            return true;
        }
    }
    return false;
}
```

### 为什么要"消抖"

按钮是机械接触，按下的瞬间会快速弹跳几次（接通→断开→接通），如果不等一下，ESP32 会以为你按了好几次。delay(50) 就是等弹跳结束再确认。

### 选购

淘宝搜 `轻触开关 6x6` 或 `按钮模块 Arduino`，几毛钱。建议买**模块版**（焊好在小板上，带 3 针排针），插杜邦线更方便。

---

## 一句话总结

| 元件 | 作用 | 接法 | 价格 |
|------|------|------|------|
| LED | 状态指示（绿=清扫/红=停） | GPIO → 电阻 → LED → GND | 几毛钱 |
| 有源蜂鸣器 | 提示音 | GPIO → 蜂鸣器 → GND | 2 元 |
| 轻触按钮 | 一键启动/停止 | GPIO(上拉) → 按钮 → GND | 几毛钱 |
