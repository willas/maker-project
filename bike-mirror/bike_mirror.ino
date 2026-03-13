/*
 * 自行车智能盲区预警后视镜（车把双臂版）
 *
 * 硬件：ESP32 + HC-SR04×2 + SG90×2 + LD3320 + 6LED + 蜂鸣器
 *
 * 功能：
 *   1. 语音"打开" → 左右臂展开 90°，开始检测
 *   2. 语音"收起" → 左右臂折叠 0°，停止检测
 *   3. 左右独立检测后方来车，LED 分级预警
 *   4. 按钮备用切换
 */

#include <ESP32Servo.h>
#include <HardwareSerial.h>

// ─── 引脚定义 ───

// 左侧
#define L_TRIG      18
#define L_ECHO      19
#define L_SERVO     25
#define L_LED_G     12
#define L_LED_Y     14
#define L_LED_R     27

// 右侧
#define R_TRIG      32
#define R_ECHO      33
#define R_SERVO     26
#define R_LED_G     15
#define R_LED_Y      2
#define R_LED_R      4

// 公共
#define BUZZER_PIN  23
#define BUTTON_PIN   5
#define LD3320_TX   17    // ESP32 TX2 → LD3320 RXD
#define LD3320_RX   16    // ESP32 RX2 ← LD3320 TXD

// ─── 参数 ───
#define DIST_DANGER   200   // < 2m 危险
#define DIST_CAUTION  400   // < 4m 注意
#define FLASH_SLOW    500
#define FLASH_FAST    120
#define MEASURE_MS    100
#define SAMPLE_COUNT    3
#define APPROACH_THR   -5   // 距离减小 > 5cm 视为靠近

#define SERVO_FOLD      0   // 收起角度
#define SERVO_EXTEND   90   // 展开角度
#define SERVO_SPEED    2    // 每步旋转度数（控制速度）

// ─── LD3320 语音识别指令 ID ───
#define CMD_OPEN    0x01    // "打开" / "展开"
#define CMD_CLOSE   0x02    // "关闭" / "收起"

// ─── 状态 ───
enum AlertLevel { SAFE, CAUTION, DANGER };

struct Side {
    uint8_t trigPin, echoPin;
    uint8_t ledG, ledY, ledR;
    Servo servo;
    uint8_t servoPin;
    float prevDist;
    AlertLevel level;
    unsigned long lastFlash;
    bool flashState;
};

Side leftSide, rightSide;
HardwareSerial voiceSerial(2);

bool armsExtended = false;
int currentServoAngle = SERVO_FOLD;
int targetServoAngle = SERVO_FOLD;
unsigned long lastMeasure = 0;
unsigned long lastServoStep = 0;
bool lastButtonState = HIGH;
unsigned long lastDebounce = 0;

// ─── 初始化单侧 ───
void initSide(Side &s, uint8_t trig, uint8_t echo, uint8_t sPin,
              uint8_t g, uint8_t y, uint8_t r) {
    s.trigPin = trig;  s.echoPin = echo;  s.servoPin = sPin;
    s.ledG = g;  s.ledY = y;  s.ledR = r;
    s.prevDist = 999.0;
    s.level = SAFE;
    s.lastFlash = 0;
    s.flashState = false;

    pinMode(trig, OUTPUT);
    pinMode(echo, INPUT);
    pinMode(g, OUTPUT);
    pinMode(y, OUTPUT);
    pinMode(r, OUTPUT);

    s.servo.attach(sPin);
    s.servo.write(SERVO_FOLD);
}

// ─── 测距 ───
float measureOnce(uint8_t trigPin, uint8_t echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long dur = pulseIn(echoPin, HIGH, 30000);
    if (dur == 0) return 999.0;
    return dur * 0.034 / 2.0;
}

float measureDist(uint8_t trigPin, uint8_t echoPin) {
    float r[SAMPLE_COUNT];
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        r[i] = measureOnce(trigPin, echoPin);
        if (i < SAMPLE_COUNT - 1) delay(10);
    }
    for (int i = 0; i < SAMPLE_COUNT - 1; i++)
        for (int j = i + 1; j < SAMPLE_COUNT; j++)
            if (r[j] < r[i]) { float t = r[i]; r[i] = r[j]; r[j] = t; }
    return r[SAMPLE_COUNT / 2];
}

// ─── LED 控制 ───
void sideLedsOff(Side &s) {
    digitalWrite(s.ledG, LOW);
    digitalWrite(s.ledY, LOW);
    digitalWrite(s.ledR, LOW);
}

void showSideSafe(Side &s) {
    sideLedsOff(s);
    digitalWrite(s.ledG, HIGH);
}

void flashSide(Side &s, uint8_t pin, unsigned long interval, unsigned long now) {
    if (now - s.lastFlash >= interval) {
        s.lastFlash = now;
        s.flashState = !s.flashState;
        sideLedsOff(s);
        digitalWrite(pin, s.flashState ? HIGH : LOW);
    }
}

// ─── 更新单侧检测 ───
void updateSide(Side &s, unsigned long now) {
    float dist = measureDist(s.trigPin, s.echoPin);
    float delta = dist - s.prevDist;
    bool approaching = (delta < APPROACH_THR);

    if (dist < DIST_DANGER && approaching) {
        s.level = DANGER;
    } else if (dist < DIST_CAUTION && approaching) {
        s.level = CAUTION;
    } else {
        s.level = SAFE;
    }
    s.prevDist = dist;
}

void renderSide(Side &s, unsigned long now) {
    switch (s.level) {
        case SAFE:    showSideSafe(s);                          break;
        case CAUTION: flashSide(s, s.ledY, FLASH_SLOW, now);   break;
        case DANGER:  flashSide(s, s.ledR, FLASH_FAST, now);   break;
    }
}

// ─── 舵机平滑旋转 ───
void updateServos(unsigned long now) {
    if (currentServoAngle == targetServoAngle) return;
    if (now - lastServoStep < 20) return;  // 每 20ms 一步
    lastServoStep = now;

    if (currentServoAngle < targetServoAngle) {
        currentServoAngle = min(currentServoAngle + SERVO_SPEED, targetServoAngle);
    } else {
        currentServoAngle = max(currentServoAngle - SERVO_SPEED, targetServoAngle);
    }
    leftSide.servo.write(currentServoAngle);
    rightSide.servo.write(currentServoAngle);
}

void extendArms() {
    targetServoAngle = SERVO_EXTEND;
    armsExtended = true;
    Serial.println("臂展开 → 开始检测");
    tone(BUZZER_PIN, 1500, 100);
}

void foldArms() {
    targetServoAngle = SERVO_FOLD;
    armsExtended = false;
    sideLedsOff(leftSide);
    sideLedsOff(rightSide);
    noTone(BUZZER_PIN);
    Serial.println("臂收起 → 停止检测");
    tone(BUZZER_PIN, 800, 100);
}

// ─── 语音识别处理 ───
void checkVoiceCommand() {
    if (!voiceSerial.available()) return;

    uint8_t cmd = voiceSerial.read();
    switch (cmd) {
        case CMD_OPEN:
            Serial.println("[语音] 识别到：打开");
            if (!armsExtended) extendArms();
            break;
        case CMD_CLOSE:
            Serial.println("[语音] 识别到：收起");
            if (armsExtended) foldArms();
            break;
        default:
            Serial.printf("[语音] 未知指令: 0x%02X\n", cmd);
            break;
    }
}

// ─── 按钮（带消抖） ───
void checkButton() {
    bool reading = digitalRead(BUTTON_PIN);
    if (reading != lastButtonState) {
        lastDebounce = millis();
    }
    if ((millis() - lastDebounce) > 50 && reading == LOW && lastButtonState == HIGH) {
        if (armsExtended) foldArms(); else extendArms();
    }
    lastButtonState = reading;
}

// ─── 蜂鸣器（任一侧危险则响） ───
void updateBuzzer() {
    if (leftSide.level == DANGER || rightSide.level == DANGER) {
        // 蜂鸣由 flashSide 的节奏驱动，这里只在两侧都安全时关
    } else {
        noTone(BUZZER_PIN);
    }
}

// ─── setup ───
void setup() {
    Serial.begin(115200);
    Serial.println("自行车盲区预警系统 v2（双臂+语音）");

    voiceSerial.begin(9600, SERIAL_8N1, LD3320_RX, LD3320_TX);

    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    initSide(leftSide,  L_TRIG, L_ECHO, L_SERVO, L_LED_G, L_LED_Y, L_LED_R);
    initSide(rightSide, R_TRIG, R_ECHO, R_SERVO, R_LED_G, R_LED_Y, R_LED_R);

    // 开机自检
    Serial.println("自检中...");
    for (int i = 0; i < 2; i++) {
        Side &s = (i == 0) ? leftSide : rightSide;
        digitalWrite(s.ledG, HIGH); delay(150); digitalWrite(s.ledG, LOW);
        digitalWrite(s.ledY, HIGH); delay(150); digitalWrite(s.ledY, LOW);
        digitalWrite(s.ledR, HIGH); delay(150); digitalWrite(s.ledR, LOW);
    }
    tone(BUZZER_PIN, 1000, 200);
    delay(300);

    // 默认收起状态，绿灯亮
    showSideSafe(leftSide);
    showSideSafe(rightSide);
    Serial.println("就绪 — 说\"打开\"或按按钮展开传感器臂");
}

// ─── loop ───
void loop() {
    unsigned long now = millis();

    checkVoiceCommand();
    checkButton();
    updateServos(now);

    if (!armsExtended) {
        // 收起状态：绿灯常亮，不检测
        showSideSafe(leftSide);
        showSideSafe(rightSide);
        delay(50);
        return;
    }

    // 展开状态：定时检测
    if (now - lastMeasure >= MEASURE_MS) {
        lastMeasure = now;
        updateSide(leftSide, now);
        updateSide(rightSide, now);

        Serial.printf("左: %.0fcm [%s]  右: %.0fcm [%s]\n",
            leftSide.prevDist,
            leftSide.level == DANGER ? "危险" : leftSide.level == CAUTION ? "注意" : "安全",
            rightSide.prevDist,
            rightSide.level == DANGER ? "危险" : rightSide.level == CAUTION ? "注意" : "安全");
    }

    renderSide(leftSide, now);
    renderSide(rightSide, now);

    // 任一侧危险 → 蜂鸣
    if (leftSide.level == DANGER || rightSide.level == DANGER) {
        if ((now / FLASH_FAST) % 2 == 0) {
            tone(BUZZER_PIN, 2000, FLASH_FAST - 20);
        }
    } else {
        noTone(BUZZER_PIN);
    }
}
