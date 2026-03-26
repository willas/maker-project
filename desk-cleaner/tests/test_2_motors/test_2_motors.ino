/*
 * test_2：电机速度 & 转向（自动，不需要按钮）
 *
 * 接线：
 *   TB6612 STBY → 直接接 3.3V（杜邦线跳，不经过 ESP32）
 *   AIN1 → GPIO 25    AIN2 → GPIO 26    PWMA → GPIO 32
 *   BIN1 → GPIO 27    BIN2 → GPIO 14    PWMB → GPIO 33
 *   TB6612 VM → 电池正极    VCC → 3.3V    GND → 共地
 *
 * ⚠️ 先把轮子架空！确认方向对了再放桌上。
 *
 * 自动流程：
 *   1. 前进：慢→中→快（测速度控制）
 *   2. 后退
 *   3. 左转、右转
 *   4. 单轮测试（排查左右是否接反）
 */

#define MOTOR_L_IN1  25
#define MOTOR_L_IN2  26
#define MOTOR_L_PWM  32
#define MOTOR_R_IN1  27
#define MOTOR_R_IN2  14
#define MOTOR_R_PWM  33

void drive(bool l1, bool l2, int lSpd, bool r1, bool r2, int rSpd) {
  digitalWrite(MOTOR_L_IN1, l1);
  digitalWrite(MOTOR_L_IN2, l2);
  digitalWrite(MOTOR_R_IN1, r1);
  digitalWrite(MOTOR_R_IN2, r2);
  ledcWrite(MOTOR_L_PWM, lSpd);
  ledcWrite(MOTOR_R_PWM, rSpd);
}

void stopAll() {
  ledcWrite(MOTOR_L_PWM, 0);
  ledcWrite(MOTOR_R_PWM, 0);
}

void rampUp(bool l1, bool l2, int lSpd, bool r1, bool r2, int rSpd) {
  digitalWrite(MOTOR_L_IN1, l1);
  digitalWrite(MOTOR_L_IN2, l2);
  digitalWrite(MOTOR_R_IN1, r1);
  digitalWrite(MOTOR_R_IN2, r2);
  int steps = 20;
  for (int i = 1; i <= steps; i++) {
    ledcWrite(MOTOR_L_PWM, lSpd * i / steps);
    ledcWrite(MOTOR_R_PWM, rSpd * i / steps);
    delay(15);
  }
}

void runTest(const char* name, bool l1, bool l2, int lSpd,
             bool r1, bool r2, int rSpd, int ms) {
  Serial.printf("  ▶ %-10s  左=%3d  右=%3d  (%d ms)\n",
                name, lSpd, rSpd, ms);
  rampUp(l1, l2, lSpd, r1, r2, rSpd);
  delay(ms);
  stopAll();
  delay(800);
}

void setup() {
  Serial.begin(115200);
  pinMode(MOTOR_L_IN1, OUTPUT);
  pinMode(MOTOR_L_IN2, OUTPUT);
  pinMode(MOTOR_R_IN1, OUTPUT);
  pinMode(MOTOR_R_IN2, OUTPUT);

  ledcAttach(MOTOR_L_PWM, 5000, 8);
  ledcAttach(MOTOR_R_PWM, 5000, 8);
  stopAll();

  Serial.println("\n=== test_2：电机速度 & 转向 ===");
  Serial.println("⚠️  轮子要架空！STBY 直接接 3.3V");
  Serial.println("3 秒后开始...\n");
  delay(3000);
}

int round_num = 0;

void loop() {
  round_num++;
  Serial.printf("\n====== 第 %d 轮 ======\n", round_num);

  Serial.println("\n--- 前进（三档速度）---");
  runTest("前进-慢",  1,0,70,   1,0,70,   1500);
  runTest("前进-中",  1,0,100,  1,0,100,  1500);
  runTest("前进-快",  1,0,140,  1,0,140,  1500);

  Serial.println("\n--- 后退 ---");
  runTest("后退-中",  0,1,100,  0,1,100,  1500);

  Serial.println("\n--- 转向 ---");
  runTest("左转",     0,1,100,  1,0,100,  1000);
  runTest("右转",     1,0,100,  0,1,100,  1000);

  Serial.println("\n--- 单轮（确认左右）---");
  runTest("只有左轮",  1,0,100,  0,0,0,    1500);
  runTest("只有右轮",  0,0,0,    1,0,100,  1500);

  Serial.println("\n✅ 本轮完成，5 秒后重来\n");
  delay(5000);
}
