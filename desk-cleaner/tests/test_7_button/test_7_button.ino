/*
 * test_7：金属按钮测试（短按 / 长按）
 *
 * 接线：
 *   按钮 COM → GPIO 5
 *   按钮 NO  → GND
 *   （如果是 4 脚小按钮，对角两脚接 GPIO 5 和 GND）
 *
 * 功能：
 *   短按 (< 2 秒) → 绿灯闪 + 蜂鸣器高音
 *   长按 (≥ 2 秒) → 红灯闪 + 蜂鸣器低音
 *   Serial Monitor 实时显示按压时间
 */

#define BUTTON_PIN   5
#define LED_GREEN    2
#define LED_RED      4
#define BUZZER_PIN  23

#define LONG_PRESS_MS 2000

bool btnDown = false;
unsigned long btnTime = 0;
bool longDone = false;
int shortCount = 0;
int longCount = 0;

int checkButton() {
  bool pressed = (digitalRead(BUTTON_PIN) == LOW);

  if (pressed && !btnDown) {
    btnDown = true;
    btnTime = millis();
    longDone = false;
    Serial.println("  [按下]");
    delay(50);
    return 0;
  }

  if (btnDown && pressed) {
    unsigned long held = millis() - btnTime;
    // 实时显示按住时长
    if (held > 500 && held % 500 < 60) {
      Serial.printf("  按住中... %lu ms\n", held);
    }
    if (!longDone && held >= LONG_PRESS_MS) {
      longDone = true;
      return 2;
    }
  }

  if (!pressed && btnDown) {
    unsigned long held = millis() - btnTime;
    btnDown = false;
    Serial.printf("  [松开] 总时长 %lu ms\n", held);
    if (!longDone) return 1;
  }

  return 0;
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.println("\n=== test_7：按钮测试 ===");
  Serial.println("短按 = 绿灯 + 高音");
  Serial.println("长按 2 秒 = 红灯 + 低音");
  Serial.println("等待按键...\n");
}

void loop() {
  int btn = checkButton();

  if (btn == 1) {
    shortCount++;
    Serial.printf("\n🟢 短按！（第 %d 次）\n\n", shortCount);
    for (int i = 0; i < 2; i++) {
      digitalWrite(LED_GREEN, HIGH);
      delay(150);
      digitalWrite(LED_GREEN, LOW);
      delay(150);
    }
    tone(BUZZER_PIN, 1500, 300);
    delay(500);

  } else if (btn == 2) {
    longCount++;
    Serial.printf("\n🔴 长按！（第 %d 次）\n\n", longCount);
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_RED, HIGH);
      delay(150);
      digitalWrite(LED_RED, LOW);
      delay(150);
    }
    tone(BUZZER_PIN, 500, 500);
    delay(700);
  }

  delay(20);
}
