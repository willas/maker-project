/*
 * test_5：DFPlayer 双声音播放（自动，不需要按钮）
 *
 * 接线：
 *   DFPlayer TX  → GPIO 21（ESP32 接收）
 *   DFPlayer RX  ← GPIO 22（ESP32 发送）
 *   DFPlayer VCC → 5V（降压模块输出）
 *   DFPlayer GND → GND
 *   SPK_1/SPK_2  → 喇叭
 *
 * SD 卡结构：
 *   01/001.mp3 ~ 020.mp3  ← 家（小朋友 A）
 *   02/001.mp3 ~ 020.mp3  ← 何（小朋友 B）
 *
 * 自动流程：
 *   1. 播放 家（01）的前 5 首
 *   2. 播放 何（02）的前 5 首
 *   3. 循环
 *
 * 使用原始指令，兼容 V1.6 魔改芯片
 */

#define DFP_RX  21
#define DFP_TX  22

#define PLAY_TRACKS  5
#define TRACK_GAP    4000

HardwareSerial dfpSerial(2);

void sendCmd(uint8_t cmd, uint8_t p1, uint8_t p2) {
  uint8_t pkt[10] = {0x7E, 0xFF, 0x06, cmd, 0x00, p1, p2, 0x00, 0x00, 0xEF};
  uint16_t sum = -(pkt[1] + pkt[2] + pkt[3] + pkt[4] + pkt[5] + pkt[6]);
  pkt[7] = (uint8_t)(sum >> 8);
  pkt[8] = (uint8_t)(sum & 0xFF);
  dfpSerial.write(pkt, 10);
}

void setVolume(uint8_t vol) {
  sendCmd(0x06, 0x00, vol);
  delay(200);
}

void playFolder(uint8_t f, uint8_t t) {
  sendCmd(0x0F, f, t);
  Serial.printf("  ▶ 文件夹 %02d / 第 %03d 首\n", f, t);
}

void playSet(uint8_t folder, const char* name) {
  Serial.printf("\n--- %s（文件夹 %02d）---\n", name, folder);
  for (int i = 1; i <= PLAY_TRACKS; i++) {
    playFolder(folder, i);
    delay(TRACK_GAP);
  }
}

void setup() {
  Serial.begin(115200);
  dfpSerial.begin(9600, SERIAL_8N1, DFP_RX, DFP_TX);

  Serial.println("\n=== test_5：DFPlayer 双声音 ===");
  Serial.println("自动轮流播放两个小朋友的声音\n");

  delay(2000);

  sendCmd(0x09, 0x00, 0x02);  // 挂载 SD 卡
  delay(1000);
  setVolume(25);
}

int round_num = 0;

void loop() {
  round_num++;
  Serial.printf("\n====== 第 %d 轮 ======\n", round_num);

  playSet(1, "家");
  delay(2000);
  playSet(2, "何");

  Serial.println("\n✅ 本轮完成，5 秒后重来\n");
  delay(5000);
}
