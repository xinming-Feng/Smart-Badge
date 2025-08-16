#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// WiFi配置
const char* ssid = "UCL_IoT";
const char* password = "NhEXQPQzue";

// 静态IP配置
IPAddress staticIP(192, 168, 3, 159);
IPAddress gateway(192, 168, 3, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(223, 5, 5, 5);

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

// PN532 I2C引脚定义（ESP32默认SDA=21, SCL=22）
#define PN532_SDA 21
#define PN532_SCL 22
// 正确的ESP32 I2C初始化方式
Adafruit_PN532 nfc(PN532_SDA, PN532_SCL);

String nfcUrl = "";
String lastNfcUrl = "";
bool tagPresent = false;

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

void displayImageFromBuffer(uint8_t* buffer, size_t len) {
    if (len != IMAGE_SIZE) {
        Serial.println("图片数据长度错误");
        return;
    }
        EPD_Init_2in9_V2();
        for (int i = 0; i < IMAGE_SIZE; i++) {
        EPD_SendData(~buffer[i]); // 反转显示
    }
    EPD_2IN9_V2_Show();
    Serial.println("图片显示完成");
}

void downloadAndDisplayImage(String url) {
    Serial.print("开始下载图片: ");
    Serial.println(url);

    WiFiClientSecure client;
    client.setInsecure(); // 跳过证书校验

    HTTPClient http;
    http.begin(client, url); // 用带 client 的 begin
    int httpCode = http.GET();
    if (httpCode == 200) {
        WiFiClient* stream = http.getStreamPtr();
        size_t total = 0;
        while (stream->available() && total < IMAGE_SIZE) {
            int n = stream->read(&imageBuffer[total], IMAGE_SIZE - total);
            if (n <= 0) break;
            total += n;
        }
        if (total == IMAGE_SIZE) {
            Serial.println("图片下载成功，开始显示");
            displayImageFromBuffer(imageBuffer, IMAGE_SIZE);
        } else {
            Serial.print("图片下载不完整，已接收字节: ");
            Serial.println(total);
        }
    } else {
        Serial.print("HTTP下载失败，状态码: ");
        Serial.println(httpCode);
        Serial.print("错误信息: ");
        Serial.println(http.errorToString(httpCode));
    }
    http.end();
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("ESP32 WiFi Display Starting...");
    
    // 初始化SPI
    EPD_initSPI();
    
    // 配置静态IP
    // if (WiFi.config(staticIP, gateway, subnet, dns, dns) == false) {
    //     Serial.println("Static IP configuration failed.");
    // }
    
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
    
    // 初始化PN532
    Wire.begin(PN532_SDA, PN532_SCL);
    nfc.begin();
    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata) {
        Serial.println("Didn't find PN53x board");
        while (1); // halt
    }
    nfc.SAMConfig();
    Serial.println("PN532 NFC初始化完成");
}

// 新增：解析NDEF URL记录
String parseNdefUrl(uint8_t* ndefData, uint32_t ndefLen) {
    if (ndefLen < 4) return "";
    
    Serial.print("开始解析NDEF数据，长度: ");
    Serial.println(ndefLen);
    
    // 打印完整的NDEF数据用于调试
    Serial.print("完整NDEF数据: ");
    for (int i = 0; i < ndefLen; i++) {
        Serial.print(ndefData[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    // 查找NDEF记录的开始位置
    uint32_t offset = 0;
    
    // 跳过可能的填充字节，查找NDEF记录
    while (offset < ndefLen && ndefData[offset] == 0) {
        offset++;
    }
    
    if (offset >= ndefLen) {
        Serial.println("未找到有效的NDEF数据");
        return "";
    }
    
    Serial.print("NDEF记录开始位置: ");
    Serial.println(offset);
    
    // 读取NDEF记录
    while (offset < ndefLen) {
        if (offset + 4 > ndefLen) {
            Serial.println("数据长度不足，退出解析");
            break;
        }
        
        uint8_t tnf = ndefData[offset] & 0x07;
        uint8_t typeLength = ndefData[offset + 1];
        uint8_t payloadLength = ndefData[offset + 2];
        
        Serial.print("TNF: 0x");
        Serial.print(tnf, HEX);
        Serial.print(", 类型长度: ");
        Serial.print(typeLength);
        Serial.print(", 载荷长度: ");
        Serial.println(payloadLength);
        
        if (offset + 3 + typeLength + payloadLength > ndefLen) {
            Serial.println("数据长度不足，退出解析");
            break;
        }
        
        // 检查是否是URL记录
        if (tnf == 0x01 && typeLength == 1 && ndefData[offset + 3] == 'U') {
            Serial.println("找到URL记录");
            // 找到URL记录，解析payload
            uint8_t* payload = &ndefData[offset + 3 + typeLength];
            
            // URL记录的第一个字节是前缀标识符
            uint8_t prefix = payload[0];
            String url = "";
            
            Serial.print("URL前缀: 0x");
            Serial.println(prefix, HEX);
            
            // 根据前缀添加协议
            switch (prefix) {
                case 0x01: url = "http://www."; break;
                case 0x02: url = "https://www."; break;
                case 0x03: url = "http://"; break;
                case 0x04: url = "https://"; break;
                default: url = ""; break;
            }
            
            // 添加URL内容
            for (int i = 1; i < payloadLength; i++) {
                url += (char)payload[i];
            }
            
            Serial.print("解析到URL: ");
            Serial.println(url);
            return url;
        } else {
            Serial.print("不是URL记录，类型: ");
            if (typeLength > 0) {
                Serial.print((char)ndefData[offset + 3]);
            }
            Serial.println();
        }
        
        offset += 3 + typeLength + payloadLength;
    }
    
    // 如果标准解析失败，尝试直接解析URL
    Serial.println("尝试直接解析URL...");
    String rawData = "";
    for (int i = 0; i < ndefLen; i++) {
        if (ndefData[i] >= 32 && ndefData[i] <= 126) { // 可打印字符
            rawData += (char)ndefData[i];
        }
    }
    
    Serial.print("原始数据: ");
    Serial.println(rawData);
    
    // 查找URL模式并清理
    if (rawData.indexOf("raw.githubusercontent.com") >= 0) {
        // 移除开头的"QU"（NDEF标识符）
        String cleanUrl = rawData;
        if (cleanUrl.startsWith("QU")) {
            cleanUrl = cleanUrl.substring(2);
        }
        
        // 移除结尾的多余字符
        while (cleanUrl.length() > 0 && 
               (cleanUrl.charAt(cleanUrl.length() - 1) < 32 || 
                cleanUrl.charAt(cleanUrl.length() - 1) > 126)) {
            cleanUrl = cleanUrl.substring(0, cleanUrl.length() - 1);
        }
        
        // 查找完整的URL路径
        int comPos = cleanUrl.indexOf(".com");
        if (comPos > 0) {
            // 查找.bin结尾
            int binPos = cleanUrl.indexOf(".bin");
            if (binPos > comPos) {
                // 找到完整的URL，包括.bin文件
                String url = "https://" + cleanUrl.substring(0, binPos + 4);
                Serial.print("清理后的URL: ");
                Serial.println(url);
                return url;
            } else {
                // 只找到.com，尝试查找其他可能的结尾
                int slashPos = cleanUrl.indexOf("/", comPos);
                if (slashPos > 0) {
                    // 查找最后一个斜杠后的内容
                    int lastSlash = cleanUrl.lastIndexOf("/");
                    if (lastSlash > slashPos) {
                        String url = "https://" + cleanUrl.substring(0, lastSlash + 1);
                        Serial.print("部分URL: ");
                        Serial.println(url);
                        return url;
                    }
                }
                
                // 如果都找不到，至少返回.com部分
                String url = "https://" + cleanUrl.substring(0, comPos + 4);
                Serial.print("基础URL: ");
                Serial.println(url);
                return url;
            }
        }
    }
    
    Serial.println("未找到URL记录");
    return "";
}

void loop() {
    // 检测NFC标签
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uidLength;
    
    boolean success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
    
    if (success) {
        if (!tagPresent) {
            Serial.println("检测到NFC标签");
            Serial.print("UID: ");
            for (int i = 0; i < uidLength; i++) {
                Serial.print(uid[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
            tagPresent = true;
        }
        
        // 对于NTAG标签，直接读取NDEF数据页面
        if (uidLength == 7) { // NTAG213/215/216
            // 只在第一次检测到标签时读取数据，避免重复读取
            static bool dataRead = false;
            if (!dataRead) {
                uint8_t ndefBuf[256];
                
                // 读取NDEF起始页（通常是第4页）
                success = nfc.ntag2xx_ReadPage(4, ndefBuf);
                if (success) {
                    // 解析NDEF TLV结构
                    uint8_t tlvType = ndefBuf[0];
                    uint8_t tlvLength = ndefBuf[1];
                    
                    Serial.print("TLV类型: 0x");
                    Serial.print(tlvType, HEX);
                    Serial.print(", 长度: ");
                    Serial.println(tlvLength);
                    
                    if (tlvType == 0x03) { // NDEF TLV
                        // 读取NDEF数据
                        uint8_t ndefData[256];
                        uint32_t dataLen = 0;
                        
                        // 计算需要读取的页面数量
                        int startPage = 5;
                        int endPage = startPage + (tlvLength + 3) / 4; // 向上取整
                        
                        Serial.print("读取页面范围: ");
                        Serial.print(startPage);
                        Serial.print(" 到 ");
                        Serial.println(endPage - 1);
                        
                        // 从第5页开始读取NDEF数据
                        for (int page = startPage; page < endPage; page++) {
                            uint8_t pageData[4];
                            if (nfc.ntag2xx_ReadPage(page, pageData)) {
                                Serial.print("页面 ");
                                Serial.print(page);
                                Serial.print(": ");
                                for (int i = 0; i < 4; i++) {
                                    Serial.print(pageData[i], HEX);
                                    Serial.print(" ");
                                }
                                Serial.println();
                                
                                // 计算当前页面应该读取的字节数
                                int bytesToRead = 4;
                                if (page == endPage - 1) {
                                    // 最后一页，只读取需要的字节数
                                    bytesToRead = tlvLength - (dataLen);
                                }
                                
                                for (int i = 0; i < bytesToRead && dataLen < tlvLength; i++) {
                                    ndefData[dataLen++] = pageData[i];
                                }
                            } else {
                                Serial.print("读取页面 ");
                                Serial.print(page);
                                Serial.println(" 失败，尝试重试...");
                                // 重试一次
                                delay(100);
                                if (nfc.ntag2xx_ReadPage(page, pageData)) {
                                    Serial.print("重试成功，页面 ");
                                    Serial.print(page);
                                    Serial.print(": ");
                                    for (int i = 0; i < 4; i++) {
                                        Serial.print(pageData[i], HEX);
                                        Serial.print(" ");
                                    }
                                    Serial.println();
                                    
                                    int bytesToRead = 4;
                                    if (page == endPage - 1) {
                                        bytesToRead = tlvLength - (dataLen);
                                    }
                                    
                                    for (int i = 0; i < bytesToRead && dataLen < tlvLength; i++) {
                                        ndefData[dataLen++] = pageData[i];
                                    }
                                } else {
                                    Serial.print("重试失败，跳过页面 ");
                                    Serial.println(page);
                                    // 继续读取下一页，不中断
                                }
                            }
                        }
                        
                        Serial.print("NDEF数据长度: ");
                        Serial.println(dataLen);
                        
                        // 打印NDEF数据的十六进制
                        Serial.print("NDEF数据: ");
                        for (int i = 0; i < (dataLen < 32 ? dataLen : 32); i++) { // 只打印前32字节
                            Serial.print(ndefData[i], HEX);
                            Serial.print(" ");
                        }
                        Serial.println();
                        
                        // 解析NDEF URL
                        String url = parseNdefUrl(ndefData, dataLen);
                        if (url.length() > 0 && url != lastNfcUrl) {
                            lastNfcUrl = url;
                            nfcUrl = url;
                            Serial.print("NFC读取到新URL: ");
                            Serial.println(nfcUrl);
                            
                            // 下载并显示图片
                            downloadAndDisplayImage(nfcUrl);
                        } else if (url.length() == 0) {
                            Serial.println("未能解析出URL");
                        }
                        
                        dataRead = true; // 标记已读取
                    } else {
                        Serial.println("未找到NDEF TLV");
                    }
                } else {
                    Serial.println("读取NDEF起始页失败");
                }
            }
        } else {
            Serial.println("不支持的标签类型");
        }
    } else {
        if (tagPresent) {
            Serial.println("NFC标签已离开");
            tagPresent = false;
            // 重置数据读取标志，为下次读取做准备
            static bool dataRead = false;
            dataRead = false;
        }
    }
    
    delay(1000); // 每秒检测一次
} 