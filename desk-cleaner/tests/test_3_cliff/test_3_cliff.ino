/*
 * test_3：悬崖传感器（TCRT5000 x3，自动，不需要按钮）
 *
 * 接线：
 *   左传感器 DO  → GPIO 34    VCC → 5V    GND → GND
 *   中传感器 DO  → GPIO 35    VCC → 5V    GND → GND
 *   右传感器 DO  → GPIO 36    VCC → 5V    GND → GND
 *
 * 测试方法：
 *   传感器朝下放在桌面上 → 应该全显示"桌面"
 *   用手挡住某个传感器 / 拿到桌边悬空 → 那个显示"悬空！"
 *
 * 说明：
 *   TCRT5000 DO 输出：LOW = 检测到桌面，HIGH = 悬空
 *   绿灯 = 安全（全部检测到桌面）
 *   红灯 = 危险（有传感器检测到悬空）
 */

#define CLIFF_L    34
#define CLIFF_M    35
#define CLIFF_R    36
#define LED_GREEN   2
#define LED_RED     4

void setup() {
  Serial.begin(115200);
  pinMode(CLIFF_L, INPUT);
  pinMode(CLIFF_M, INPUT);
  pinMode(CLIFF_R, INPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  Serial.println("\n=== test_3：悬崖传感器 ===");
  Serial.println("放在桌面上 = 全绿，悬空 = 红灯报警");
  Serial.println("每 0.5 秒刷新一次\n");
  Serial.println("  左    中    右    状态");
  Serial.println("  ----  ----  ----  --------");
  delay(1000);
}

void loop() {
  bool L = digitalRead(CLIFF_L) == HIGH;  // HIGH = 悬空
  bool M = digitalRead(CLIFF_M) == HIGH;
  bool R = digitalRead(CLIFF_R) == HIGH;

  bool danger = L || M || R;

  Serial.printf("  %s  %s  %s  ",
    L ? "悬空" : " OK ",
    M ? "悬空" : " OK ",
    R ? "悬空" : " OK ");

  if (!danger) {
    Serial.println("✅ 安全");
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
  } else {
    // 给出建议动作
    if (M) {
      Serial.println("⚠️  正前方悬空 → 应该后退！");
    } else if (L && !R) {
      Serial.println("⚠️  左边悬空 → 应该右转！");
    } else if (R && !L) {
      Serial.println("⚠️  右边悬空 → 应该左转！");
    } else {
      Serial.println("⚠️  多个悬空 → 应该后退！");
    }
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);
  }

  delay(500);
}
