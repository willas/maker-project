/*
 * 第四阶段测试：超声波测距（HC-SR04）
 * 把手放在传感器前面来回移动，串口打印距离 + 字符"雷达"
 *
 * 🎮 游戏：先猜桌上某个东西离多远，再看传感器量出来的数字！
 */

#define TRIG_PIN 18
#define ECHO_PIN 19

float measure() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long dur = pulseIn(ECHO_PIN, HIGH, 30000);
  if (dur == 0) return 999.0;
  return dur * 0.034 / 2.0;
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.println("=== 第四阶段测试：超声波测距 ===");
  Serial.println("把手放在传感器前面，来回移动看距离变化！");
  Serial.println();
}

void loop() {
  float d = measure();

  if (d > 400) {
    Serial.println("距离: 太远了/没测到");
  } else {
    Serial.printf("距离: %5.1f cm  ", d);

    int bars = constrain((int)(d / 2), 0, 40);
    Serial.print("|");
    for (int i = 0; i < bars; i++) Serial.print("=");
    Serial.print(">");

    if (d < 8) Serial.print("  ← 太近了! 小车会避开");
    Serial.println();
  }

  delay(300);
}
