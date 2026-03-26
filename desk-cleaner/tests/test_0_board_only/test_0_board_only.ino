/*
 * test_0：板子验证
 *
 * 只用 ESP32 + USB 线，不接任何东西
 * 板子上蓝色小灯会闪烁 = 开发环境 OK
 *
 * 预期：蓝灯 亮1秒 → 灭1秒 → 循环
 * Serial Monitor (115200) 会打印 "亮" / "灭"
 */

#define LED 2

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  Serial.println("\n=== test_0：板子验证 ===");
  Serial.println("蓝灯闪烁 = 一切正常\n");
}

void loop() {
  digitalWrite(LED, HIGH);
  Serial.println("💡 亮");
  delay(1000);
  digitalWrite(LED, LOW);
  Serial.println("   灭");
  delay(1000);
}
