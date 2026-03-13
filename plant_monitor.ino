/*
 * 智能水培植物监测器 — 主程序
 * 硬件：ESP32 + TDS + pH + DS18B20 + 水位 + 光敏 + OLED + LED + 蜂鸣器
 * 功能：实时监测水质参数，异常时本地报警 + 微信推送
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ==================== WiFi 配置 ====================
const char* WIFI_SSID     = "你的WiFi名";
const char* WIFI_PASSWORD = "你的WiFi密码";

// ==================== 推送配置（PushPlus） ====================
// 注册 https://www.pushplus.plus/ 获取 token
const char* PUSH_TOKEN = "你的pushplus_token";

// ==================== 引脚定义 ====================
#define TDS_PIN       34
#define PH_PIN        35
#define DS18B20_PIN   4
#define WATER_LVL_PIN 15
#define LIGHT_PIN     32
#define LED_GREEN     25
#define LED_RED       26
#define BUZZER_PIN    27

// ==================== OLED 配置 ====================
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ==================== 温度传感器 ====================
OneWire oneWire(DS18B20_PIN);
DallasTemperature tempSensor(&oneWire);

// ==================== 阈值配置（可根据植物调整） ====================
struct Thresholds {
  float tdsMin  = 300;   // TDS 最低（ppm）
  float tdsMax  = 800;   // TDS 最高
  float phMin   = 5.5;   // pH 最低
  float phMax   = 6.5;   // pH 最高
  float tempMin = 18.0;  // 水温最低（℃）
  float tempMax = 30.0;  // 水温最高
  int   lightMin = 300;  // 光照最低（ADC 值）
};

Thresholds thresholds;

// ==================== 传感器数据 ====================
struct SensorData {
  float tds;
  float ph;
  float waterTemp;
  bool  waterLevelOk;
  int   light;
};

SensorData data;

// ==================== 报警控制 ====================
unsigned long lastPushTime = 0;
const unsigned long PUSH_INTERVAL = 30 * 60 * 1000; // 30 分钟内不重复推送

// ==================== 初始化 ====================
void setup() {
  Serial.begin(115200);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(WATER_LVL_PIN, INPUT);

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  tempSensor.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED 初始化失败");
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Starting...");
  display.display();

  connectWiFi();
}

// ==================== 主循环 ====================
void loop() {
  readSensors();
  displayData();

  bool hasAlert = checkAlerts();

  if (hasAlert) {
    alertLocal();
    alertRemote();
  } else {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }

  delay(5000); // 每 5 秒读一次
}

// ==================== WiFi 连接 ====================
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("连接 WiFi");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" 已连接");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(" WiFi 连接失败，仅本地模式");
  }
}

// ==================== 读取传感器 ====================
void readSensors() {
  data.tds = readTDS();
  data.ph = readPH();

  tempSensor.requestTemperatures();
  data.waterTemp = tempSensor.getTempCByIndex(0);

  data.waterLevelOk = digitalRead(WATER_LVL_PIN) == HIGH;
  data.light = analogRead(LIGHT_PIN);

  Serial.printf("TDS: %.0f ppm | pH: %.1f | Temp: %.1f℃ | Water: %s | Light: %d\n",
    data.tds, data.ph, data.waterTemp,
    data.waterLevelOk ? "OK" : "LOW", data.light);
}

// ==================== TDS 读取 ====================
float readTDS() {
  int raw = analogRead(TDS_PIN);
  float voltage = raw * 3.3 / 4095.0;
  // 简化公式，实际使用时需根据传感器校准
  float tds = (133.42 * voltage * voltage * voltage
             - 255.86 * voltage * voltage
             + 857.39 * voltage) * 0.5;
  return tds;
}

// ==================== pH 读取 ====================
float readPH() {
  int raw = analogRead(PH_PIN);
  float voltage = raw * 3.3 / 4095.0;
  // 简化线性公式：pH = -5.7 * voltage + 21.34
  // 实际使用时需用标准液校准（pH 4.0 和 pH 6.86）
  float ph = -5.7 * voltage + 21.34;
  return ph;
}

// ==================== OLED 显示 ====================
void displayData() {
  display.clearDisplay();
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.printf("TDS:  %.0f ppm", data.tds);

  display.setCursor(0, 12);
  display.printf("pH:   %.1f", data.ph);

  display.setCursor(0, 24);
  display.printf("Temp: %.1f C", data.waterTemp);

  display.setCursor(0, 36);
  display.printf("Water: %s", data.waterLevelOk ? "OK" : "LOW!");

  display.setCursor(0, 48);
  display.printf("Light: %d", data.light);

  display.display();
}

// ==================== 检查是否异常 ====================
bool checkAlerts() {
  if (data.tds < thresholds.tdsMin)   return true;  // 营养不足
  if (data.tds > thresholds.tdsMax)   return true;  // 营养过浓
  if (data.ph < thresholds.phMin)     return true;  // 太酸
  if (data.ph > thresholds.phMax)     return true;  // 太碱
  if (data.waterTemp < thresholds.tempMin) return true;
  if (data.waterTemp > thresholds.tempMax) return true;
  if (!data.waterLevelOk)             return true;  // 水位低
  if (data.light < thresholds.lightMin) return true; // 光照不足
  return false;
}

// ==================== 本地报警 ====================
void alertLocal() {
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);

  // 蜂鸣器短响 3 次
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

// ==================== 微信推送 ====================
void alertRemote() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (millis() - lastPushTime < PUSH_INTERVAL) return;

  String title = "植物监测器报警";
  String content = buildAlertMessage();

  String url = "http://www.pushplus.plus/send";
  String body = "{\"token\":\"" + String(PUSH_TOKEN)
              + "\",\"title\":\"" + title
              + "\",\"content\":\"" + content
              + "\",\"template\":\"html\"}";

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);

  if (code == 200) {
    Serial.println("推送成功");
    lastPushTime = millis();
  } else {
    Serial.printf("推送失败: %d\n", code);
  }
  http.end();
}

// ==================== 构建报警消息 ====================
String buildAlertMessage() {
  String msg = "<h3>水培监测异常</h3><ul>";

  if (data.tds < thresholds.tdsMin)
    msg += "<li>TDS 偏低: " + String(data.tds, 0) + " ppm（建议加营养液）</li>";
  if (data.tds > thresholds.tdsMax)
    msg += "<li>TDS 偏高: " + String(data.tds, 0) + " ppm（建议换水稀释）</li>";
  if (data.ph < thresholds.phMin)
    msg += "<li>pH 偏酸: " + String(data.ph, 1) + "（建议加碱性调节剂）</li>";
  if (data.ph > thresholds.phMax)
    msg += "<li>pH 偏碱: " + String(data.ph, 1) + "（建议加酸性调节剂）</li>";
  if (data.waterTemp < thresholds.tempMin)
    msg += "<li>水温过低: " + String(data.waterTemp, 1) + "℃</li>";
  if (data.waterTemp > thresholds.tempMax)
    msg += "<li>水温过高: " + String(data.waterTemp, 1) + "℃</li>";
  if (!data.waterLevelOk)
    msg += "<li>水位过低，请加水</li>";
  if (data.light < thresholds.lightMin)
    msg += "<li>光照不足，建议移到光线好的位置</li>";

  msg += "</ul>";
  return msg;
}
