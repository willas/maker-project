/*
 * 智能桌面清洁小车 · ESP32
 *
 * 行为逻辑：
 *   1. 语音说"打扫" 或按下按钮 → 开始清扫，播放可爱开场语音
 *   2. 直线前进，底部旋转刷(280电机) + 吸尘鼓风机(5015离心风扇)同时工作
 *   3. 悬崖传感器检测到桌边 → 后退 + 随机转向 + 随机语音
 *   4. 超声波检测到前方障碍 → 后退 + 转向 + 随机语音
 *   5. 清扫过程中随机"哼歌"或说话
 *   6. 超过设定时间 → 自动停止，播放完成语音
 *   7. 语音说"停下" 或再按一次按钮 → 停止
 *
 * 语音播报：DFPlayer Mini (Serial2, GPIO 16/17) + Micro SD 卡 + 小喇叭
 * 语音识别：LU-ASR01 (Serial1 映射到 GPIO 21/22) — 离线识别"打扫""停下"
 *
 *   SD 卡文件夹结构（两个小朋友各一套录音）：
 *   SD 卡/
 *   ├── 01/          ← 小朋友 A 的声音
 *   │   ├── 001.mp3
 *   │   ├── 002.mp3
 *   │   └── ... 020.mp3
 *   └── 02/          ← 小朋友 B 的声音
 *       ├── 001.mp3
 *       └── ... 020.mp3
 *
 *   长按按钮 2 秒 → 切换声音（会播放开机语音确认是谁的声音）
 */

#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// ==================== 引脚定义 ====================

// 左电机（经 TB6612）
#define MOTOR_L_IN1  25
#define MOTOR_L_IN2  26
#define MOTOR_L_PWM  32

// 右电机（经 TB6612）
#define MOTOR_R_IN1  27
#define MOTOR_R_IN2  14
#define MOTOR_R_PWM  33

// TB6612 待机引脚：请用杜邦线将模块上的 STBY 直接连到 3.3V 或 5V，保持常开

// 清扫电机(280) & 吸尘鼓风机(5015, 5V)（经 MOS 管）
// 注意：避开导致死机的 12/15 引脚，将两个 MOS 管的信号线(SIG)都接在 13 引脚
#define CLEAN_PIN    13

// 悬崖传感器（TCRT5000 DO 输出，LOW = 有桌面，HIGH = 悬空）
#define CLIFF_L      34
#define CLIFF_M      35
#define CLIFF_R      36

// 超声波 HC-SR04
#define TRIG_PIN     18
#define ECHO_PIN     19

// 蜂鸣器 & LED & 按钮
#define BUZZER_PIN   23
#define LED_GREEN    2
#define LED_RED      4
#define BUTTON_PIN   5

// DFPlayer Mini（用 Serial2）
#define DFP_TX       16
#define DFP_RX       17

// LU-ASR01 语音识别（用 Serial1 映射到 GPIO 21/22）
#define ASR_TX       22    // ESP32 TX → LU-ASR01 RXD
#define ASR_RX       21    // ESP32 RX ← LU-ASR01 TXD

// ==================== LU-ASR01 语音指令 ID ====================
// 在 TIANWEN BLOCK 图形化工具里配置关键词，每个关键词对应一个 ID
#define VCMD_CLEAN   0x01  // "打扫" / "开始"
#define VCMD_STOP    0x02  // "停下" / "停"
#define VCMD_HELLO   0x03  // "你好"（彩蛋：回复开机语音）
#define VCMD_SWITCH  0x04  // "换声音"（切换另一个小朋友的语音）

// ==================== 参数 ====================

#define MOTOR_SPEED      120
#define BRUSH_SPEED      140
#define OBSTACLE_CM      8
#define BACK_TIME_MS     400
#define TURN_TIME_MS     500
#define CLEAN_DURATION   120   // 自动清扫最长时间（秒）
#define VOLUME           25    // 音量 0–30
#define CHAT_INTERVAL    15000 // 清扫中随机说话间隔（毫秒）
#define LONG_PRESS_MS    2000  // 长按切换声音的时间阈值（毫秒）
#define NUM_VOICES       2     // 两个小朋友的声音

// ==================== 语音编号 ====================
// SD 卡 mp3 文件夹中：0001.mp3 ~ 0020.mp3
// 录音时用手机录好，格式转成 MP3，按下面编号命名

// --- 开机 / 待机 ---
#define SND_BOOT          1   // "你好呀！我是桌面小管家，按一下我就帮你打扫~"

// --- 开始清扫 ---
#define SND_START_1        2   // "出发咯！让我把桌子擦得亮亮的！"
#define SND_START_2        3   // "交给我吧！橡皮屑屑，我来收拾你们啦~"
#define SND_START_3        4   // "开工开工！嘟嘟嘟~"

// --- 清扫中随机说话 ---
#define SND_WORKING_1      5   // "呼~好多碎屑呀，不过难不倒我！"
#define SND_WORKING_2      6   // "嗡嗡嗡~我是勤劳的小蜜蜂~"
#define SND_WORKING_3      7   // "这里好脏哦，让我多扫扫~"
#define SND_WORKING_4      8   // "哼哼哼~打扫卫生真开心~"
#define SND_WORKING_5      9   // "加油加油，马上就干净啦！"

// --- 检测到桌边 ---
#define SND_EDGE_1        10   // "哎呀！差点掉下去，好险好险~"
#define SND_EDGE_2        11   // "前面没路了，我转个弯~"
#define SND_EDGE_3        12   // "呜呜，桌子边边好吓人，我换个方向！"

// --- 检测到障碍物 ---
#define SND_OBS_1         13   // "前面有东西挡住了，我绕过去~"
#define SND_OBS_2         14   // "哎呦，撞到东西了，换条路走~"
#define SND_OBS_3         15   // "这里有个大家伙，我还是躲开吧~"

// --- 清扫完成 ---
#define SND_DONE_1        16   // "打扫完啦！桌子干干净净，记得夸夸我哦~"
#define SND_DONE_2        17   // "任务完成！你的桌面已经闪闪发亮啦！"
#define SND_DONE_3        18   // "呼~累死我了，不过看到干净的桌子好开心！"

// --- 被手动停止 ---
#define SND_STOP          19   // "好的好的，我休息一下~"

// --- 收集盒满提醒（预留） ---
#define SND_FULL          20   // "我的肚子好像装满了，帮我倒一下垃圾吧~"

// ==================== PWM 参数 ====================

#define PWM_FREQ     5000
#define PWM_RES      8

// ==================== 全局变量 ====================

HardwareSerial dfpSerial(2);
DFRobotDFPlayerMini dfPlayer;
bool hasDFPlayer = false;

HardwareSerial asrSerial(1);
bool hasASR = false;

bool cleaning = false;
unsigned long cleanStartTime = 0;
unsigned long lastChatTime = 0;

// 状态机
enum CarState { IDLE, FORWARD, BACKING, TURNING };
CarState carState = IDLE;
unsigned long stateStartTime = 0;
int turnDirection = 0;
unsigned long turnDuration = 0;
unsigned long lastSpeakTime = 0;
#define SPEAK_COOLDOWN  3000  // 语音之间至少间隔 3 秒

// 双声音：文件夹 01 = 小朋友 A，文件夹 02 = 小朋友 B
uint8_t voiceFolder = 1;

// 按钮：短按 = 开始/停止，长按 = 切换声音
bool buttonIsDown = false;
unsigned long buttonDownTime = 0;
bool longPressHandled = false;

// ==================== DFPlayer ====================

void initDFPlayer() {
    dfpSerial.begin(9600, SERIAL_8N1, DFP_RX, DFP_TX);
    delay(500);

    if (dfPlayer.begin(dfpSerial)) {
        hasDFPlayer = true;
        dfPlayer.volume(VOLUME);
        Serial.println("DFPlayer Mini 就绪。");
    } else {
        hasDFPlayer = false;
        Serial.println("[提示] DFPlayer 未检测到，将只用蜂鸣器。");
    }
}

void playSound(int trackNum) {
    if (hasDFPlayer) {
        dfPlayer.playFolder(voiceFolder, trackNum);
    }
}

void playRandom(int from, int count) {
    playSound(from + random(count));
}

// ==================== LU-ASR01 语音识别 ====================

void initASR() {
    asrSerial.begin(9600, SERIAL_8N1, ASR_RX, ASR_TX);
    delay(300);
    hasASR = true;
    Serial.println("LU-ASR01 语音识别串口已初始化。");
    // LU-ASR01 上电后自动进入识别模式（关键词通过 TIANWEN BLOCK 烧录到模块）
}

// 返回识别到的指令 ID，没有则返回 0
// LU-ASR01 默认串口协议：识别成功后发送对应指令编号（具体格式以模块说明书为准）
uint8_t checkVoiceCommand() {
    if (!hasASR || !asrSerial.available()) return 0;
    uint8_t cmd = asrSerial.read();
    Serial.printf("[ASR 调试] 收到字节: 0x%02X (%d)\n", cmd, cmd);
    while (asrSerial.available()) {
        uint8_t extra = asrSerial.read();
        Serial.printf("[ASR 调试] 额外字节: 0x%02X (%d)\n", extra, extra);
    }
    return cmd;
}

// ==================== 电机控制 ====================

void motorsInit() {
    pinMode(MOTOR_L_IN1, OUTPUT);
    pinMode(MOTOR_L_IN2, OUTPUT);
    pinMode(MOTOR_R_IN1, OUTPUT);
    pinMode(MOTOR_R_IN2, OUTPUT);

    ledcAttach(MOTOR_L_PWM, PWM_FREQ, PWM_RES);
    ledcAttach(MOTOR_R_PWM, PWM_FREQ, PWM_RES);
    
    // 初始化清扫机构（刷子和风扇共用一个引脚调速）
    ledcAttach(CLEAN_PIN, PWM_FREQ, PWM_RES);
}

void rampMotors(int lSpd, int rSpd) {
    int curL = 0, curR = 0;
    for (int i = 1; i <= 10; i++) {
        ledcWrite(MOTOR_L_PWM, lSpd * i / 10);
        ledcWrite(MOTOR_R_PWM, rSpd * i / 10);
        delay(15);
    }
}

void forward(int speed) {
    digitalWrite(MOTOR_L_IN1, HIGH);
    digitalWrite(MOTOR_L_IN2, LOW);
    digitalWrite(MOTOR_R_IN1, HIGH);
    digitalWrite(MOTOR_R_IN2, LOW);
    ledcWrite(MOTOR_L_PWM, speed);
    ledcWrite(MOTOR_R_PWM, speed);
}

void backward(int speed) {
    digitalWrite(MOTOR_L_IN1, LOW);
    digitalWrite(MOTOR_L_IN2, HIGH);
    digitalWrite(MOTOR_R_IN1, LOW);
    digitalWrite(MOTOR_R_IN2, HIGH);
    rampMotors(speed, speed);
}

void turnLeft(int speed) {
    digitalWrite(MOTOR_L_IN1, LOW);
    digitalWrite(MOTOR_L_IN2, HIGH);
    digitalWrite(MOTOR_R_IN1, HIGH);
    digitalWrite(MOTOR_R_IN2, LOW);
    rampMotors(speed, speed);
}

void turnRight(int speed) {
    digitalWrite(MOTOR_L_IN1, HIGH);
    digitalWrite(MOTOR_L_IN2, LOW);
    digitalWrite(MOTOR_R_IN1, LOW);
    digitalWrite(MOTOR_R_IN2, HIGH);
    rampMotors(speed, speed);
}

void stopMotors() {
    ledcWrite(MOTOR_L_PWM, 0);
    ledcWrite(MOTOR_R_PWM, 0);
}

// ==================== 清扫机构 ====================

void brushOn() {
    for (int i = 1; i <= 10; i++) {
        ledcWrite(CLEAN_PIN, BRUSH_SPEED * i / 10);
        delay(20);
    }
}
void brushOff() { ledcWrite(CLEAN_PIN, 0); }
void fanOn()    { /* 风扇和刷子已经绑定在 CLEAN_PIN，这里留空或合并都可以 */ }
void fanOff()   { /* 同上 */ }

// ==================== 悬崖检测 ====================

bool cliffDetected() {
    return digitalRead(CLIFF_L) == HIGH
        || digitalRead(CLIFF_M) == HIGH
        || digitalRead(CLIFF_R) == HIGH;
}

// ==================== 超声波测距 ====================

float getDistanceCm() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    if (duration == 0) return 999.0;
    return duration * 0.034 / 2.0;
}

// ==================== 避障：进入后退状态 ====================

void startBacking(int dir, int soundFrom, int soundCount) {
    stopMotors();
    if (millis() - lastSpeakTime >= SPEAK_COOLDOWN) {
        playRandom(soundFrom, soundCount);
        lastSpeakTime = millis();
    }
    carState = BACKING;
    stateStartTime = millis();
    turnDirection = dir;
    turnDuration = TURN_TIME_MS + random(300);
    backward(MOTOR_SPEED);
}

// ==================== 开始 / 停止清扫 ====================

void startCleaning() {
    cleaning = true;
    cleanStartTime = millis();
    lastChatTime = millis();
    brushOn();
    fanOn();
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);

    playRandom(SND_START_1, 3);

    tone(BUZZER_PIN, 1000, 200);
    delay(250);
    tone(BUZZER_PIN, 1500, 200);

    Serial.println("开始清扫！");
    delay(2500);  // 等开场语音说完再动
    carState = FORWARD;
    rampMotors(MOTOR_SPEED, MOTOR_SPEED);
}

void stopCleaning() {
    cleaning = false;
    stopMotors();
    brushOff();
    fanOff();
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);

    carState = IDLE;
    Serial.println("清扫结束。");
}

void finishCleaning() {
    playRandom(SND_DONE_1, 3);
    stopCleaning();
}

void manualStop() {
    playSound(SND_STOP);
    stopCleaning();
}

// ==================== 清扫中随机说话 ====================

void maybeChatWhileCleaning() {
    unsigned long now = millis();
    if (now - lastChatTime >= CHAT_INTERVAL && now - lastSpeakTime >= SPEAK_COOLDOWN) {
        lastChatTime = now;
        lastSpeakTime = now;
        playRandom(SND_WORKING_1, 5);
    }
}

// ==================== 切换声音 ====================

void switchVoice() {
    voiceFolder = (voiceFolder % NUM_VOICES) + 1;
    Serial.printf("切换到小朋友 %c 的声音（文件夹 %02d）\n",
                  'A' + voiceFolder - 1, voiceFolder);

    // 闪灯提示：绿灯 = A，红灯 = B
    for (int i = 0; i < 3; i++) {
        digitalWrite(voiceFolder == 1 ? LED_GREEN : LED_RED, HIGH);
        delay(150);
        digitalWrite(voiceFolder == 1 ? LED_GREEN : LED_RED, LOW);
        delay(150);
    }

    tone(BUZZER_PIN, voiceFolder == 1 ? 1000 : 1500, 300);
    delay(400);

    playSound(SND_BOOT);  // 播放该小朋友的开机语音，家长一听就知道是谁
}

// ==================== 按钮检测（短按 / 长按）====================
// 返回: 0 = 无操作, 1 = 短按, 2 = 长按

int checkButton() {
    bool pressed = (digitalRead(BUTTON_PIN) == LOW);

    if (pressed && !buttonIsDown) {
        buttonIsDown = true;
        buttonDownTime = millis();
        longPressHandled = false;
        delay(50);
        return 0;
    }

    if (buttonIsDown && pressed && !longPressHandled
        && (millis() - buttonDownTime >= LONG_PRESS_MS)) {
        longPressHandled = true;
        return 2;
    }

    if (!pressed && buttonIsDown) {
        buttonIsDown = false;
        if (!longPressHandled) {
            return 1;
        }
    }

    return 0;
}

// ==================== setup ====================

void setup() {
    Serial.begin(115200);
    Serial.println("桌面清洁小车启动中...");

    motorsInit();
    initDFPlayer();
    initASR();

    pinMode(CLIFF_L, INPUT);
    pinMode(CLIFF_M, INPUT);
    pinMode(CLIFF_R, INPUT);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    stopMotors();
    brushOff();
    fanOff();
    digitalWrite(LED_RED, HIGH);

    randomSeed(analogRead(0));

    delay(1000);
    playSound(SND_BOOT);  // "你好呀！我是桌面小管家~"

    Serial.printf("就绪（当前声音：小朋友 %c），短按开始清扫，长按 2 秒切换声音。\n",
                  'A' + voiceFolder - 1);
}

// ==================== loop ====================

void loop() {
    // --- 按钮（任何时候都检测，清扫中按下就停）---
    if (cleaning && digitalRead(BUTTON_PIN) == LOW) {
        delay(30);
        if (digitalRead(BUTTON_PIN) == LOW) {
            manualStop();
            while (digitalRead(BUTTON_PIN) == LOW) delay(10);
            delay(200);
            return;
        }
    }
    if (!cleaning) {
        if (digitalRead(BUTTON_PIN) == LOW) {
            delay(50);
            if (digitalRead(BUTTON_PIN) == LOW) {
                unsigned long pressStart = millis();
                // 等松手或等超过 2 秒
                while (digitalRead(BUTTON_PIN) == LOW) {
                    if (millis() - pressStart >= LONG_PRESS_MS) {
                        // 长按：切换声音
                        switchVoice();
                        while (digitalRead(BUTTON_PIN) == LOW) delay(10);
                        delay(300);
                        return;
                    }
                    delay(10);
                }
                // 松手了且不到 2 秒：短按开始清扫
                startCleaning();
                delay(300);
                return;
            }
        }
    }

    // --- 语音（任何时候都检测）---
    uint8_t vcmd = checkVoiceCommand();
    if (vcmd == VCMD_CLEAN && !cleaning) {
        Serial.println("[语音] 识别到：打扫");
        startCleaning();
        return;
    } else if (vcmd == VCMD_STOP && cleaning) {
        Serial.println("[语音] 识别到：停下");
        manualStop();
        return;
    } else if (vcmd == VCMD_HELLO) {
        Serial.println("[语音] 识别到：你好");
        playSound(SND_BOOT);
        return;
    } else if (vcmd == VCMD_SWITCH && !cleaning) {
        Serial.println("[语音] 识别到：换声音");
        switchVoice();
        return;
    }

    if (!cleaning) {
        delay(50);
        return;
    }

    // --- 超时自动停止 ---
    if ((millis() - cleanStartTime) / 1000 >= CLEAN_DURATION) {
        Serial.println("清扫时间到。");
        finishCleaning();
        return;
    }

    // --- 状态机 ---
    unsigned long now = millis();

    switch (carState) {
        case FORWARD:
            if (cliffDetected()) {
                Serial.println("检测到桌边！");
                int dir = random(2) == 0 ? -1 : 1;
                startBacking(dir, SND_EDGE_1, 3);
            } else if (getDistanceCm() < OBSTACLE_CM) {
                Serial.println("前方有障碍物。");
                startBacking(1, SND_OBS_1, 3);
            } else {
                forward(MOTOR_SPEED);
                maybeChatWhileCleaning();
            }
            break;

        case BACKING:
            if (now - stateStartTime >= BACK_TIME_MS) {
                stopMotors();
                carState = TURNING;
                stateStartTime = now;
                if (turnDirection < 0) turnLeft(MOTOR_SPEED);
                else turnRight(MOTOR_SPEED);
            }
            break;

        case TURNING:
            if (now - stateStartTime >= turnDuration) {
                stopMotors();
                carState = FORWARD;
            }
            break;

        default:
            break;
    }

    delay(20);
}
