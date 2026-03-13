/*
 * 第三阶段测试：悬崖传感器（TCRT5000 x3）
 * 用手遮挡/移开传感器，观察串口打印和 LED 变化
 * - 绿灯 = 安全（3 个传感器都看到桌面）
 * - 红灯 = 危险（至少一个传感器看到悬空）
 */

#define CLIFF_L   34
#define CLIFF_M   35
#define CLIFF_R   36
#define LED_GREEN  2
#define LED_RED    4

void setup() {
  Serial.begin(115200);
  pinMode(CLIFF_L, INPUT);
  pinMode(CLIFF_M, INPUT);
  pinMode(CLIFF_R, INPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  Serial.println("=== 第三阶段测试：悬崖传感器 ===");
  Serial.println("用手挡住传感器 = 模拟有桌面");
  Serial.println("移开手 = 模拟悬空(桌边)");
  Serial.println("格式: 左 中 右 (0=有桌面, 1=悬空)");
}

void loop() {
  bool left   = digitalRead(CLIFF_L) == HIGH;
  bool middle = digitalRead(CLIFF_M) == HIGH;
  bool right  = digitalRead(CLIFF_R) == HIGH;

  bool danger = left || middle || right;

  digitalWrite(LED_GREEN, !danger);
  digitalWrite(LED_RED, danger);

  if (danger) {
    Serial.printf("⚠️  悬崖!  左=%d  中=%d  右=%d  ", left, middle, right);
    if (left)   Serial.print("[左边悬空→应右转] ");
    if (middle) Serial.print("[中间悬空→应后退] ");
    if (right)  Serial.print("[右边悬空→应左转] ");
    Serial.println();
  } else {
    Serial.println("✅ 安全  左=0  中=0  右=0");
  }

  delay(300);
}
