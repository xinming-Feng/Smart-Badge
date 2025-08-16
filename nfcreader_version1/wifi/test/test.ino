#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// ==== 配置区域 =========================================

// 本机广播名称，确保两块 ESP32 不同，例如 "ESP32_PEER_01"、"ESP32_PEER_02"
#define DEVICE_NAME "ESP32_PEER_01"

// 只扫描并处理名称以此前缀开头的设备（即对方广播的名称）
#define NAME_PREFIX "ESP32_PEER_02"

// 每次扫描时长（秒）
const uint32_t SCAN_TIME_SECONDS = 2;

// 振动马达连接的 GPIO 引脚（支持 analogWrite 的任意 PWM 引脚）
const int VIBRATION_PIN = 33;

// RSSI 阈值数组，分 5 档（从近到远）：
//   RSSI >= -40        -> 档位 5
//   -55 <= RSSI < -40  -> 档位 4
//   -65 <= RSSI < -55  -> 档位 3
//   -75 <= RSSI < -65  -> 档位 2
//   -85 <= RSSI < -75  -> 档位 1
//   否则（< -85）     -> 档位 0（不振动）
const int RSSI_THRESHOLDS[5] = { -40, -55, -65, -75, -85 };

// 振动强度（占空比 0~255 之间固定一个值）
const int VIBRATION_DUTY = 200;

// =========================================================

// 全局标记：在本轮扫描中是否已执行过一次振动
static bool didVibrateThisScan = false;

// 将单次 RSSI 转换为 0~5 档：0 表示不振动，1~5 分别对应 1~5 次振动
int rssiToLevel(int rssi) {
  for (int i = 0; i < 5; i++) {
    if (rssi >= RSSI_THRESHOLDS[i]) {
      return 5 - i;  // i=0 -> 5, i=1 -> 4, …, i=4 -> 1
    }
  }
  return 0;
}

// 扫描回调类：每当 BLE 扫描到一个广告包就调用 onResult()
class MyScanCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    // 如果本轮已经振动过，就直接返回，避免重复多次振动
    if (didVibrateThisScan) {
      return;
    }

    // 1) 必须带有 Local Name
    if (!advertisedDevice.haveName()) {
      return;
    }

    // 2) 取出名称并判断前缀是否匹配 NAME_PREFIX
    std::string advName = advertisedDevice.getName().c_str();
    if (advName.rfind(NAME_PREFIX, 0) != 0) {
      return;
    }

    // 3) 读取 RSSI 并转换档位
    int rssi = advertisedDevice.getRSSI();
    int level = rssiToLevel(rssi);

    // 如果档位 > 0，则立即振动对应次数
    if (level > 0) {
      Serial.printf("[SCAN_CB] 发现节点: %s | RSSI: %d dBm → 档位: %d\n",
                    advName.c_str(), rssi, level);
      // 做 "level 次振动" 脉冲，每次 200ms 打开 + 200ms 关闭
      for (int i = 0; i < level; i++) {
        analogWrite(VIBRATION_PIN, VIBRATION_DUTY);
        delay(200);
        analogWrite(VIBRATION_PIN, 0);
        delay(200);
      }
      // 标记这一轮已经振动过，避免重复
      didVibrateThisScan = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("==== ESP32_PEER_01: BLE 五档 RSSI → 振动次数 示例 ====");

  // 1) 初始化振动引脚
  pinMode(VIBRATION_PIN, OUTPUT);
  analogWrite(VIBRATION_PIN, 0);  // 默认关闭

  // 2) 初始化 BLE 广播，让对方能扫描到自己
  BLEDevice::init(DEVICE_NAME);
  BLEServer* pServer = BLEDevice::createServer();  // 主要为了启动 BLE 广播
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setScanResponse(false);
  pAdvertising->start();
  Serial.printf("开始广播：%s\n", DEVICE_NAME);

  // 3) 初始化 BLE 扫描（不立即 start，等待 loop() 控制时机）
  BLEScan* pScan = BLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new MyScanCallbacks());
  pScan->setActiveScan(true);  // 主动扫描，以便能获取 Local Name 和 RSSI
  pScan->setInterval(100);     // 100 ms 扫描间隔
  pScan->setWindow(99);        // 99 ms 扫描窗口
}

void loop() {
  static BLEScan* pScan = BLEDevice::getScan();

  // 1) 停止广播 → 开始扫描 SCAN_TIME_SECONDS 秒 → 清扫缓存 → 重启广播
  BLEDevice::getAdvertising()->stop();
  Serial.printf(">> 广播停止，开始扫描 %u 秒...\n", SCAN_TIME_SECONDS);

  // 本次扫描只要 onResult() 检测到对端，就会执行振动并将 didVibrateThisScan 置 true
  didVibrateThisScan = false;          // 每一轮扫描前都重置为 false
  pScan->start(SCAN_TIME_SECONDS, false);
  pScan->clearResults();

  BLEDevice::getAdvertising()->start();
  Serial.println(">> 广播重启");

  // 2) 如果本轮扫描期间从未检测到符合条件的对端（didVibrateThisScan 仍然是 false），则不开启任何振动
  if (!didVibrateThisScan) {
    analogWrite(VIBRATION_PIN, 0);
    Serial.println("[RESULT] 本轮未检测到对端或 RSSI 过低，无振动");
  }

  // 3) 延时 500 ms，再进入下一轮循环
  delay(500);
}
