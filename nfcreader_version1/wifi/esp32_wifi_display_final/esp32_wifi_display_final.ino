#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>

// WiFi配置
const char* ssid = "HUAWEI-T1EGY7";
const char* password = "ZXCVBNM123456";

// 静态IP配置
IPAddress staticIP(192, 168, 3, 159);
IPAddress gateway(192, 168, 3, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(223, 5, 5, 5);

// 服务器
WebServer server(80);

// 2.9寸墨水屏引脚定义
#define PIN_SPI_SCK  13
#define PIN_SPI_DIN  14
#define PIN_SPI_CS   15
#define PIN_SPI_BUSY 25
#define PIN_SPI_RST  26
#define PIN_SPI_DC   27

// 图像缓冲区
#define IMAGE_SIZE 4736
uint8_t imageBuffer[IMAGE_SIZE];
int uploadOffset = 0;

// 基础函数
void EPD_Reset() {
    digitalWrite(PIN_SPI_RST, LOW);
    delay(200);
    digitalWrite(PIN_SPI_RST, HIGH);
    delay(200);
}

void EPD_SendCommand(uint8_t command) {
    digitalWrite(PIN_SPI_DC, LOW);
    digitalWrite(PIN_SPI_CS, LOW);
    SPI.transfer(command);
    digitalWrite(PIN_SPI_CS, HIGH);
}

void EPD_SendData(uint8_t data) {
    digitalWrite(PIN_SPI_DC, HIGH);
    digitalWrite(PIN_SPI_CS, LOW);
    SPI.transfer(data);
    digitalWrite(PIN_SPI_CS, HIGH);
}

void EPD_Send_1(uint8_t cmd, uint8_t d1) {
    EPD_SendCommand(cmd);
    EPD_SendData(d1);
}

void EPD_Send_2(uint8_t cmd, uint8_t d1, uint8_t d2) {
    EPD_SendCommand(cmd);
    EPD_SendData(d1);
    EPD_SendData(d2);
}

void EPD_Send_3(uint8_t cmd, uint8_t d1, uint8_t d2, uint8_t d3) {
    EPD_SendCommand(cmd);
    EPD_SendData(d1);
    EPD_SendData(d2);
    EPD_SendData(d3);
}

void EPD_Send_4(uint8_t cmd, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4) {
    EPD_SendCommand(cmd);
    EPD_SendData(d1);
    EPD_SendData(d2);
    EPD_SendData(d3);
    EPD_SendData(d4);
}

void EPD_WaitUntilIdle() {
    while (digitalRead(PIN_SPI_BUSY) == LOW) {
        delay(100);
    }
}

void EPD_WaitUntilIdle_high() {
    while (digitalRead(PIN_SPI_BUSY) == HIGH) {
        delay(100);
    }
}

// 使用2.9寸V2墨水屏初始化序列
int EPD_Init_2in9_V2() {
    EPD_Reset();
    EPD_WaitUntilIdle_high();
   
    EPD_SendCommand(0x12); //SWRESET
    EPD_WaitUntilIdle_high();
    EPD_Send_3(0x01, 0x27, 0x01, 0x00);//Driver output control  
    EPD_Send_1(0x11, 0x03);//data entry mode
    EPD_Send_2(0x21, 0x00, 0x80);//  Display update control
   
    EPD_Send_2(0x44, 0x00, 0x0f);// SET_RAM_X_ADDRESS_START_END_POSITION
    EPD_Send_4(0x45, 0x00, 0x00, 0x27, 0x01);// SET_RAM_Y_ADDRESS_START_END_POSITION

    EPD_Send_1(0x4e, 0x00);// // SET_RAM_X_ADDRESS_COUNTER
    EPD_Send_2(0x4f, 0x00, 0x00);// SET_RAM_Y_ADDRESS_COUNTER
   
    EPD_WaitUntilIdle_high();
    EPD_SendCommand(0x24);//WRITE_RAM
    delay(2);
    return 0;
}

// 2.9寸V2墨水屏显示函数
void EPD_2IN9_V2_Show(void) {
    Serial.print("\r\nEPD_2IN9_V2_Show");
    EPD_Send_1(0x22, 0xF7); //Display Update Control
    EPD_SendCommand(0x20); //Activate Display Update Sequence
    EPD_WaitUntilIdle_high();   
}

// 初始化SPI
void EPD_initSPI() {
    pinMode(PIN_SPI_BUSY, INPUT);
    pinMode(PIN_SPI_RST, OUTPUT);
    pinMode(PIN_SPI_DC, OUTPUT);
    pinMode(PIN_SPI_CS, OUTPUT);
    pinMode(PIN_SPI_SCK, OUTPUT);
    pinMode(PIN_SPI_DIN, OUTPUT);

    digitalWrite(PIN_SPI_CS, HIGH);
    digitalWrite(PIN_SPI_SCK, LOW);
    
    SPI.begin(PIN_SPI_SCK, -1, PIN_SPI_DIN, PIN_SPI_CS);
    SPI.setFrequency(4000000); // 4MHz
}

// 处理根页面
void handleRoot() {
    server.send(200, "text/html", "<h1>ESP32 WiFi Display</h1><p>Use the Android App to upload images</p>");
}

// 处理图片上传
void handleUpload() {
    Serial.print("Bytes received: ");
    Serial.println(uploadOffset);
    if (uploadOffset == IMAGE_SIZE) {
        Serial.println("Starting display update...");
        
        // 分析接收到的数据
        int blackPixels = 0, whitePixels = 0;
        for (int i = 0; i < IMAGE_SIZE; i++) {
            if (imageBuffer[i] == 0x00) blackPixels++;
            else if (imageBuffer[i] == 0xFF) whitePixels++;
        }
        Serial.printf("Data analysis - Black: %d, White: %d, Other: %d\n", 
                      blackPixels, whitePixels, IMAGE_SIZE - blackPixels - whitePixels);
        
        // 使用V2初始化序列
        EPD_Init_2in9_V2();
        
        // 发送反转的图像数据（解决黑白相反问题）
        Serial.println("Sending inverted image data...");
        for (int i = 0; i < IMAGE_SIZE; i++) {
            EPD_SendData(~imageBuffer[i]);  // 反转所有位
        }
        
        // 刷新显示
        Serial.println("Refreshing display...");
        EPD_2IN9_V2_Show();
        
        Serial.println("Display update completed");
        server.send(200, "text/plain", "Image received and displayed!");
    } else {
        Serial.print("Expected 4736 bytes, got ");
        Serial.println(uploadOffset);
        server.send(400, "text/plain", "Invalid data size");
    }
    uploadOffset = 0; // 重置
}

// 处理上传数据
void uploadHandler() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        Serial.println("UPLOAD_FILE_START");
        uploadOffset = 0;
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        Serial.print("UPLOAD_FILE_WRITE, size: ");
        Serial.println(upload.currentSize);
        int len = upload.currentSize;
        if (upload.buf != nullptr && len > 0 && uploadOffset < IMAGE_SIZE) {
            int copyLen = len;
            if (uploadOffset + copyLen > IMAGE_SIZE) {
                copyLen = IMAGE_SIZE - uploadOffset;
            }
            memcpy(&imageBuffer[uploadOffset], upload.buf, copyLen);
            uploadOffset += copyLen;
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        Serial.println("UPLOAD_FILE_END");
    }
}

// 测试显示 - 显示清晰的测试图案
void handleTest() {
    Serial.println("Testing display with clear pattern...");
    
    // 使用V2初始化序列
    EPD_Init_2in9_V2();
    
    // 发送清晰的测试图案（上半部分白色，下半部分黑色）
    Serial.println("Sending test pattern...");
    for (int i = 0; i < IMAGE_SIZE; i++) {
        if (i < IMAGE_SIZE / 2) {
            EPD_SendData(0xFF);  // 上半部分白色
        } else {
            EPD_SendData(0x00);  // 下半部分黑色
        }
    }
    
    // 刷新显示
    EPD_2IN9_V2_Show();
    
    Serial.println("Test pattern displayed");
    server.send(200, "text/plain", "Test pattern displayed");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("ESP32 WiFi Display Starting...");
    
    // 初始化SPI
    EPD_initSPI();
    
    // 配置静态IP
    if (WiFi.config(staticIP, gateway, subnet, dns, dns) == false) {
        Serial.println("Static IP configuration failed.");
    }
    
    // 连接WiFi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // 设置服务器路由
    server.on("/", HTTP_GET, handleRoot);
    server.on("/upload", HTTP_POST, handleUpload, uploadHandler);
    server.on("/test", HTTP_GET, handleTest);
    
    // 启动服务器
    server.begin();
    Serial.println("HTTP server started");
    Serial.println("Ready to receive images!");
}

void loop() {
    server.handleClient();
} 