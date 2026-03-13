/*
 * 第一阶段测试：LED + 蜂鸣器 + 按钮
 * 按一下按钮 → 绿灯亮 + 高音提示
 * 再按一下   → 红灯亮 + 低音提示
 * 开机时有彩虹灯秀
 */

#define LED_GREEN  2
#define LED_RED    4
#define BUZZER_PIN 23
#define BUTTON_PIN 5

bool greenOn = false;

void setup() {
  Serial.begin(115200);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("=== 第一阶段测试：LED + 蜂鸣器 + 按钮 ===");
  Serial.println("按按钮试试！绿灯/红灯会交替亮。");

  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_GREEN, HIGH);
    tone(BUZZER_PIN, 1000 + i * 500, 200);
    delay(300);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);
    delay(300);
    digitalWrite(LED_RED, LOW);
  }

  digitalWrite(LED_RED, HIGH);
  Serial.println("红灯亮 = 待机。按按钮切换！");
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW) {
      greenOn = !greenOn;
      digitalWrite(LED_GREEN, greenOn);
      digitalWrite(LED_RED, !greenOn);
      tone(BUZZER_PIN, greenOn ? 1500 : 800, 200);

      Serial.println(greenOn ? "绿灯亮！(清扫模式)" : "红灯亮！(待机模式)");

      while (digitalRead(BUTTON_PIN) == LOW) delay(10);
    }
  }
}
