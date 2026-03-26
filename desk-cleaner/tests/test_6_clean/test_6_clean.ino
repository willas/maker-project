/*
 * test_6：清扫机构测试（刷子电机 + 吸尘风扇，经 MOS 管）
 *
 * 接线：
 *   MOS 管模块 SIG → GPIO 13
 *   MOS 管模块 VCC → 5V（或电池电压）
 *   MOS 管模块 GND → GND
 *   MOS 管输出 → 130 刷子电机 / 3010 吸尘风扇
 *
 *   两个 MOS 管的 SIG 都接 GPIO 13（共用一个引脚，同时开关）
 *
 * 自动流程：
 *   1. 慢速启动（防止电流冲击）
 *   2. 中速运行
 *   3. 全速运行
 *   4. 减速停止
 *   5. 循环
 */

#define CLEAN_PIN  13

void setup() {
  Serial.begin(115200);

  ledcAttach(CLEAN_PIN, 5000, 8);  // 5kHz PWM, 8-bit
  ledcWrite(CLEAN_PIN, 0);

  Serial.println("\n=== test_6：清扫机构（刷子+风扇）===");
  Serial.println("MOS 管 SIG → GPIO 13");
  Serial.println("3 秒后开始...\n");
  delay(3000);
}

int round_num = 0;

void loop() {
  round_num++;
  Serial.printf("\n====== 第 %d 轮 ======\n", round_num);

  // 1. 缓慢加速
  Serial.println("\n--- 缓慢加速 ---");
  for (int spd = 0; spd <= 200; spd += 20) {
    Serial.printf("  速度：%3d / 255\n", spd);
    ledcWrite(CLEAN_PIN, spd);
    delay(300);
  }

  // 2. 中速保持
  Serial.println("\n--- 中速运行 (200) ---");
  ledcWrite(CLEAN_PIN, 200);
  delay(3000);

  // 3. 全速
  Serial.println("--- 全速运行 (255) ---");
  ledcWrite(CLEAN_PIN, 255);
  delay(3000);

  // 4. 缓慢减速
  Serial.println("\n--- 缓慢减速 ---");
  for (int spd = 255; spd >= 0; spd -= 20) {
    Serial.printf("  速度：%3d / 255\n", spd);
    ledcWrite(CLEAN_PIN, spd);
    delay(300);
  }

  // 5. 完全停止
  ledcWrite(CLEAN_PIN, 0);
  Serial.println("\n  停止");

  Serial.println("\n✅ 本轮完成，5 秒后重来\n");
  delay(5000);
}
