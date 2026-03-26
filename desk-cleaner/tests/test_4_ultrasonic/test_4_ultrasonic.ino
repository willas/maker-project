/*
 * test_4：超声波测距（HC-SR04，自动，不需要按钮）
 *
 * 接线：
 *   TRIG → GPIO 18
 *   ECHO → GPIO 19
 *   VCC  → 5V
 *   GND  → GND
 *
 * 测试方法：
 *   用手在传感器前面移动，Serial Monitor 会显示距离和图形条
 *   距离 < 8cm 时会标红报警
 */

#define TRIG_PIN  18
#define ECHO_PIN  19
#define WARN_CM    8

float getDistanceCm() {
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

  Serial.println("\n=== test_4：超声波测距 ===");
  Serial.println("用手在传感器前面移动，观察距离变化");
  Serial.printf("警戒距离：< %d cm\n\n", WARN_CM);
  delay(1000);
}

void loop() {
  float d = getDistanceCm();

  // 距离条（每 2cm 一个方块，最多 50cm）
  int bars = constrain((int)(d / 2), 0, 25);

  Serial.printf("%6.1f cm  ", d);

  for (int i = 0; i < bars; i++) Serial.print("█");
  for (int i = bars; i < 25; i++) Serial.print("░");

  if (d < WARN_CM) {
    Serial.print("  ⚠️ 障碍物！");
  } else if (d > 400) {
    Serial.print("  (超出范围)");
  }

  Serial.println();
  delay(200);
}
