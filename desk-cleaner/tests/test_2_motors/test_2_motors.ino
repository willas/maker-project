/*
 * 第二阶段测试：电机走路
 * 每按一次按钮，依次执行：前进 → 后退 → 左转 → 右转 → 停
 * 串口会打印当前动作
 *
 * ⚠️ 先把轮子架空测试！确认方向对了再放桌上。
 */

#define MOTOR_L_IN1  25
#define MOTOR_L_IN2  26
#define MOTOR_L_PWM  32
#define MOTOR_R_IN1  27
#define MOTOR_R_IN2  14
#define MOTOR_R_PWM  33
#define MOTOR_STBY   13
#define BUTTON_PIN   5

int step = 0;
const char* actions[] = {"前进", "后退", "左转", "右转", "停"};

void driveMotors(bool l1, bool l2, bool r1, bool r2, int speed) {
  digitalWrite(MOTOR_STBY, HIGH);
  digitalWrite(MOTOR_L_IN1, l1);
  digitalWrite(MOTOR_L_IN2, l2);
  digitalWrite(MOTOR_R_IN1, r1);
  digitalWrite(MOTOR_R_IN2, r2);
  ledcWrite(MOTOR_L_PWM, speed);
  ledcWrite(MOTOR_R_PWM, speed);
}

void stopAll() {
  ledcWrite(MOTOR_L_PWM, 0);
  ledcWrite(MOTOR_R_PWM, 0);
  digitalWrite(MOTOR_STBY, LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(MOTOR_L_IN1, OUTPUT);
  pinMode(MOTOR_L_IN2, OUTPUT);
  pinMode(MOTOR_R_IN1, OUTPUT);
  pinMode(MOTOR_R_IN2, OUTPUT);
  pinMode(MOTOR_STBY, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  ledcAttach(MOTOR_L_PWM, 5000, 8);
  ledcAttach(MOTOR_R_PWM, 5000, 8);

  digitalWrite(MOTOR_STBY, LOW);

  Serial.println("=== 第二阶段测试：电机走路 ===");
  Serial.println("按按钮依次测试：前进 → 后退 → 左转 → 右转 → 停");
  Serial.println("⚠️ 请先把轮子架空！");
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW) {
      stopAll();
      delay(300);

      int action = step % 5;
      Serial.printf("▶ %s\n", actions[action]);

      switch (action) {
        case 0: driveMotors(1,0,1,0, 150); break;  // 前进
        case 1: driveMotors(0,1,0,1, 150); break;  // 后退
        case 2: driveMotors(0,1,1,0, 150); break;  // 左转
        case 3: driveMotors(1,0,0,1, 150); break;  // 右转
        case 4: stopAll(); break;                    // 停
      }
      step++;

      while (digitalRead(BUTTON_PIN) == LOW) delay(10);
    }
  }
}
