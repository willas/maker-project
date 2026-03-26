/*
 * test_1：LED + 蜂鸣器（自动，不需要按钮）
 *
 * 接线：
 *   绿色 LED 长脚 → GPIO 2，短脚 → GND
 *   红色 LED 长脚 → GPIO 4，短脚 → GND
 *   蜂鸣器 (+)   → GPIO 23，(-) → GND
 *
 * 自动流程（循环）：
 *   1. 绿灯闪 3 下
 *   2. 红灯闪 3 下
 *   3. 蜂鸣器 低→中→高 三个音
 *   4. 绿灯+高音 = "清扫模式"
 *   5. 红灯+低音 = "待机模式"
 *   6. 全部关闭，等 3 秒
 */

#define LED_GREEN   2
#define LED_RED     4
#define BUZZER_PIN  23

void blinkLed(int pin, const char* name, int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH);
    Serial.printf("  %s 亮\n", name);
    delay(300);
    digitalWrite(pin, LOW);
    delay(300);
  }
}

void beep(int freq, int ms, const char* desc) {
  Serial.printf("  🔊 %s (%dHz)\n", desc, freq);
  tone(BUZZER_PIN, freq, ms);
  delay(ms + 200);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.println("\n=== test_1：LED + 蜂鸣器 ===");
  Serial.println("自动测试，观察灯和声音\n");
  delay(1000);
}

int round_num = 0;

void loop() {
  round_num++;
  Serial.printf("\n====== 第 %d 轮 ======\n", round_num);

  Serial.println("\n1. 绿灯闪 3 下");
  blinkLed(LED_GREEN, "绿灯", 3);

  Serial.println("\n2. 红灯闪 3 下");
  blinkLed(LED_RED, "红灯", 3);

  Serial.println("\n3. 蜂鸣器三个音");
  beep(500,  300, "低音");
  beep(1000, 300, "中音");
  beep(2000, 300, "高音");

  Serial.println("\n4. 清扫模式 = 绿灯 + 高音");
  digitalWrite(LED_GREEN, HIGH);
  tone(BUZZER_PIN, 1500, 500);
  delay(800);
  digitalWrite(LED_GREEN, LOW);

  Serial.println("5. 待机模式 = 红灯 + 低音");
  digitalWrite(LED_RED, HIGH);
  tone(BUZZER_PIN, 500, 500);
  delay(800);
  digitalWrite(LED_RED, LOW);

  Serial.println("\n✅ 本轮完成，3 秒后重来\n");
  delay(3000);
}
