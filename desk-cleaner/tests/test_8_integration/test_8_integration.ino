/*
 * test_8：全功能联调
 *
 * 把所有模块接好后跑这个测试，验证每个部分都能协同工作。
 * 使用按钮控制，不需要语音模块。
 *
 * 接线（全部）：
 *   TB6612: AIN1→25, AIN2→26, PWMA→32, BIN1→27, BIN2→14, PWMB→33, STBY→3.3V
 *   MOS管:  SIG→13
 *   悬崖:   L→34, M→35, R→36
 *   超声波: TRIG→18, ECHO→19
 *   蜂鸣器: →23
 *   LED:    绿→2, 红→4
 *   按钮:   COM→5, NO→GND
 *   DFPlayer: TX→21, RX←22
 *
 * 操作：
 *   短按 → 开始/停止（驱动电机+刷子+语音）
 *   长按 2 秒 → 切换声音（家 ↔ 何）
 *
 * 清扫时自动避障和防坠落
 */

#define MOTOR_L_IN1  25
#define MOTOR_L_IN2  26
#define MOTOR_L_PWM  32
#define MOTOR_R_IN1  27
#define MOTOR_R_IN2  14
#define MOTOR_R_PWM  33
#define CLEAN_PIN    13

#define CLIFF_L      34
#define CLIFF_M      35
#define CLIFF_R      36

#define TRIG_PIN     18
#define ECHO_PIN     19

#define BUZZER_PIN   23
#define LED_GREEN    2
#define LED_RED      4
#define BUTTON_PIN   5

#define DFP_RX       21
#define DFP_TX       22

#define MOTOR_SPEED     150
#define OBSTACLE_CM       8
#define LONG_PRESS_MS  2000
#define VOLUME           25

HardwareSerial dfpSerial(2);

bool cleaning = false;
unsigned long cleanStart = 0;
unsigned long lastChat = 0;

uint8_t voiceFolder = 1;

bool btnDown = false;
unsigned long btnTime = 0;
bool longDone = false;

// ---- DFPlayer 原始指令 ----

void sendCmd(uint8_t cmd, uint8_t p1, uint8_t p2) {
  uint8_t pkt[10] = {0x7E, 0xFF, 0x06, cmd, 0x00, p1, p2, 0x00, 0x00, 0xEF};
  uint16_t sum = -(pkt[1] + pkt[2] + pkt[3] + pkt[4] + pkt[5] + pkt[6]);
  pkt[7] = (uint8_t)(sum >> 8);
  pkt[8] = (uint8_t)(sum & 0xFF);
  dfpSerial.write(pkt, 10);
}

void playSound(uint8_t track) {
  sendCmd(0x0F, voiceFolder, track);
}

void playRandom(uint8_t from, uint8_t count) {
  playSound(from + random(count));
}

// ---- 电机 ----

void forward()  {
  digitalWrite(MOTOR_L_IN1,1); digitalWrite(MOTOR_L_IN2,0);
  digitalWrite(MOTOR_R_IN1,1); digitalWrite(MOTOR_R_IN2,0);
  ledcWrite(MOTOR_L_PWM, MOTOR_SPEED);
  ledcWrite(MOTOR_R_PWM, MOTOR_SPEED);
}

void backward() {
  digitalWrite(MOTOR_L_IN1,0); digitalWrite(MOTOR_L_IN2,1);
  digitalWrite(MOTOR_R_IN1,0); digitalWrite(MOTOR_R_IN2,1);
  ledcWrite(MOTOR_L_PWM, MOTOR_SPEED);
  ledcWrite(MOTOR_R_PWM, MOTOR_SPEED);
}

void turnLeft() {
  digitalWrite(MOTOR_L_IN1,0); digitalWrite(MOTOR_L_IN2,1);
  digitalWrite(MOTOR_R_IN1,1); digitalWrite(MOTOR_R_IN2,0);
  ledcWrite(MOTOR_L_PWM, MOTOR_SPEED);
  ledcWrite(MOTOR_R_PWM, MOTOR_SPEED);
}

void turnRight() {
  digitalWrite(MOTOR_L_IN1,1); digitalWrite(MOTOR_L_IN2,0);
  digitalWrite(MOTOR_R_IN1,0); digitalWrite(MOTOR_R_IN2,1);
  ledcWrite(MOTOR_L_PWM, MOTOR_SPEED);
  ledcWrite(MOTOR_R_PWM, MOTOR_SPEED);
}

void stopMotors() {
  ledcWrite(MOTOR_L_PWM, 0);
  ledcWrite(MOTOR_R_PWM, 0);
}

// ---- 传感器 ----

bool cliffDetected() {
  return digitalRead(CLIFF_L)==HIGH
      || digitalRead(CLIFF_M)==HIGH
      || digitalRead(CLIFF_R)==HIGH;
}

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

// ---- 按钮 ----

int checkButton() {
  bool pressed = (digitalRead(BUTTON_PIN) == LOW);
  if (pressed && !btnDown) {
    btnDown = true; btnTime = millis(); longDone = false;
    delay(50); return 0;
  }
  if (btnDown && pressed && !longDone && (millis()-btnTime >= LONG_PRESS_MS)) {
    longDone = true; return 2;
  }
  if (!pressed && btnDown) {
    btnDown = false;
    if (!longDone) return 1;
  }
  return 0;
}

// ---- 切换声音 ----

void switchVoice() {
  voiceFolder = (voiceFolder == 1) ? 2 : 1;
  Serial.printf("🔄 切换到小朋友 %c（文件夹 %02d）\n",
                'A'+voiceFolder-1, voiceFolder);
  uint8_t led = (voiceFolder==1) ? LED_GREEN : LED_RED;
  for (int i=0; i<3; i++) {
    digitalWrite(led, HIGH); delay(150);
    digitalWrite(led, LOW);  delay(150);
  }
  tone(BUZZER_PIN, voiceFolder==1 ? 1000 : 1500, 300);
  delay(400);
  playSound(1);  // 播放开机语音确认
}

// ---- 开始 / 停止 ----

void startCleaning() {
  cleaning = true;
  cleanStart = millis();
  lastChat = millis();
  ledcWrite(CLEAN_PIN, 200);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);
  playRandom(2, 3);  // 开场语音 002~004
  tone(BUZZER_PIN, 1000, 200);
  delay(250);
  tone(BUZZER_PIN, 1500, 200);
  Serial.println("▶ 开始清扫！");
}

void stopCleaning(bool finished) {
  cleaning = false;
  stopMotors();
  ledcWrite(CLEAN_PIN, 0);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);
  if (finished) {
    playRandom(16, 3);  // 完成语音 016~018
    Serial.println("✅ 清扫完成！");
  } else {
    playSound(19);      // 手动停止语音 019
    Serial.println("⏹ 手动停止");
  }
}

// ---- 避障 ----

void avoidEdge() {
  stopMotors();
  playRandom(10, 3);  // 桌边语音 010~012
  Serial.println("  ⚠️ 悬崖！后退+转向");
  backward(); delay(400); stopMotors();
  if (random(2)==0) turnLeft(); else turnRight();
  delay(500 + random(300)); stopMotors();
}

void avoidObstacle() {
  stopMotors();
  playRandom(13, 3);  // 障碍语音 013~015
  Serial.println("  ⚠️ 障碍物！后退+转向");
  backward(); delay(400); stopMotors();
  turnRight(); delay(500); stopMotors();
}

// ---- setup ----

void setup() {
  Serial.begin(115200);
  dfpSerial.begin(9600, SERIAL_8N1, DFP_RX, DFP_TX);

  pinMode(MOTOR_L_IN1, OUTPUT); pinMode(MOTOR_L_IN2, OUTPUT);
  pinMode(MOTOR_R_IN1, OUTPUT); pinMode(MOTOR_R_IN2, OUTPUT);
  ledcAttach(MOTOR_L_PWM, 5000, 8);
  ledcAttach(MOTOR_R_PWM, 5000, 8);
  ledcAttach(CLEAN_PIN, 5000, 8);

  pinMode(CLIFF_L, INPUT); pinMode(CLIFF_M, INPUT); pinMode(CLIFF_R, INPUT);
  pinMode(TRIG_PIN, OUTPUT); pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_GREEN, OUTPUT); pinMode(LED_RED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  stopMotors();
  ledcWrite(CLEAN_PIN, 0);
  digitalWrite(LED_RED, HIGH);

  randomSeed(analogRead(0));

  Serial.println("\n=== test_8：全功能联调 ===");
  Serial.println("短按 = 开始/停止清扫");
  Serial.println("长按 2 秒 = 切换声音");
  Serial.println("STBY 接 3.3V，DFPlayer 接 21/22\n");

  delay(2000);
  sendCmd(0x09, 0x00, 0x02);  // 挂载 SD 卡
  delay(1000);
  sendCmd(0x06, 0x00, VOLUME); // 音量
  delay(300);
  playSound(1);  // 开机语音

  Serial.printf("就绪（声音：小朋友 %c），等待按钮...\n\n",
                'A'+voiceFolder-1);
}

// ---- loop ----

void loop() {
  int btn = checkButton();
  if (btn == 1) {
    if (cleaning) stopCleaning(false);
    else startCleaning();
    delay(300);
    return;
  } else if (btn == 2 && !cleaning) {
    switchVoice();
    delay(300);
    return;
  }

  if (!cleaning) { delay(50); return; }

  // 120 秒超时
  if ((millis()-cleanStart)/1000 >= 120) {
    Serial.println("⏰ 清扫超时");
    stopCleaning(true);
    return;
  }

  // 悬崖检测
  if (cliffDetected()) {
    avoidEdge();
    return;
  }

  // 障碍物检测
  float dist = getDistanceCm();
  if (dist < OBSTACLE_CM) {
    avoidObstacle();
    return;
  }

  // 随机说话（每 15 秒）
  if (millis()-lastChat >= 15000) {
    lastChat = millis();
    playRandom(5, 5);  // 工作语音 005~009
  }

  forward();
  delay(50);
}
