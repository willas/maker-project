/*
 * 第五阶段测试：DFPlayer Mini 语音播放
 * 自动依次播放 SD 卡里的音频，每隔 4 秒播下一首
 * 按按钮也能手动切到下一首
 *
 * SD 卡准备：
 *   根目录/mp3/0001.mp3 ~ 0020.mp3
 *   SD 卡格式化为 FAT32
 *
 * 接线：
 *   DFPlayer RX → GPIO 16（经 1kΩ 电阻）
 *   DFPlayer TX → GPIO 17
 *   DFPlayer VCC → 5V, GND → GND
 *   SPK+/SPK- → 喇叭
 */

#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>

#define DFP_TX     16
#define DFP_RX     17
#define BUTTON_PIN  5

HardwareSerial dfpSerial(2);
DFRobotDFPlayerMini dfPlayer;

int currentTrack = 1;
int totalTracks = 20;
unsigned long lastPlay = 0;

const char* trackNames[] = {
  "",
  "开机: 你好呀！我是桌面小管家~",
  "开始1: 出发咯！",
  "开始2: 交给我吧！",
  "开始3: 开工开工！",
  "工作1: 好多碎屑呀",
  "工作2: 嗡嗡嗡小蜜蜂",
  "工作3: 这里好脏哦",
  "工作4: 哼哼哼真开心",
  "工作5: 加油加油",
  "桌边1: 好险好险",
  "桌边2: 前面没路了",
  "桌边3: 桌子边边好吓人",
  "障碍1: 前面有东西",
  "障碍2: 撞到东西了",
  "障碍3: 大家伙,我躲开",
  "完成1: 打扫完啦",
  "完成2: 任务完成",
  "完成3: 累死了但好开心",
  "停止: 我休息一下",
  "盒满: 肚子装满了"
};

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("=== 第五阶段测试：DFPlayer Mini ===");
  Serial.println("初始化中...");

  dfpSerial.begin(9600, SERIAL_8N1, DFP_RX, DFP_TX);
  delay(1000);

  if (!dfPlayer.begin(dfpSerial)) {
    Serial.println("❌ DFPlayer 未检测到！请检查：");
    Serial.println("  1. 接线是否正确（RX 经 1kΩ 电阻）");
    Serial.println("  2. SD 卡是否插好");
    Serial.println("  3. SD 卡是否 FAT32 格式");
    while (true) delay(1000);
  }

  Serial.println("✅ DFPlayer 就绪！");
  dfPlayer.volume(25);

  Serial.println("自动播放所有台词，也可以按按钮跳到下一首。");
  Serial.println();

  playTrack(1);
}

void playTrack(int num) {
  currentTrack = num;
  dfPlayer.play(num);
  lastPlay = millis();

  if (num >= 1 && num <= totalTracks) {
    Serial.printf("🔊 第 %02d 首: %s\n", num, trackNames[num]);
  } else {
    Serial.printf("🔊 第 %02d 首\n", num);
  }
}

void loop() {
  // 按按钮：下一首
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW) {
      int next = (currentTrack % totalTracks) + 1;
      playTrack(next);
      while (digitalRead(BUTTON_PIN) == LOW) delay(10);
    }
  }

  // 自动播放：每 4 秒下一首
  if (millis() - lastPlay >= 4000) {
    int next = (currentTrack % totalTracks) + 1;
    playTrack(next);
  }
}
