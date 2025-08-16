#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "你的WiFi名称";
const char* password = "你的WiFi密码";

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("ESP32 HTTP 测试程序启动");

    WiFi.begin(ssid, password);
    Serial.print("正在连接WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi已连接");
    Serial.print("IP地址: ");
    Serial.println(WiFi.localIP());

    Serial.println("开始测试 http://httpbin.org/get ...");
    HTTPClient httpTest;
    httpTest.begin("http://httpbin.org/get");
    int testCode = httpTest.GET();
    Serial.print("HTTP测试状态码: ");
    Serial.println(testCode);
    Serial.print("错误信息: ");
    Serial.println(httpTest.errorToString(testCode));
    httpTest.end();
    Serial.println("httpbin.org 测试结束");
}

void loop() {
    // 空循环
} 