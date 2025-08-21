// 2.9寸V2墨水屏极简底层驱动，仅供esp32_ble_nfc_epaper.ino使用
// 作者：精简自Waveshare官方库，仅保留2.9寸V2相关内容

#ifndef _EPD_H_
#define _EPD_H_

#include <Arduino.h>

// SPI引脚定义
#define PIN_SPI_SCK  13
#define PIN_SPI_DIN  14
#define PIN_SPI_CS   15
#define PIN_SPI_BUSY 25
#define PIN_SPI_RST  26
#define PIN_SPI_DC   27

// SPI初始化
inline void EPD_initSPI() {
    pinMode(PIN_SPI_BUSY,  INPUT);
    pinMode(PIN_SPI_RST , OUTPUT);
    pinMode(PIN_SPI_DC  , OUTPUT);
    pinMode(PIN_SPI_SCK, OUTPUT);
    pinMode(PIN_SPI_DIN, OUTPUT);
    pinMode(PIN_SPI_CS , OUTPUT);
    digitalWrite(PIN_SPI_CS , HIGH);
    digitalWrite(PIN_SPI_SCK, LOW);
}

// SPI发送1字节
inline void EPD_SendData(uint8_t data) {
    digitalWrite(PIN_SPI_DC, HIGH);
    digitalWrite(PIN_SPI_CS, LOW);
    for (int i = 0; i < 8; i++) {
        digitalWrite(PIN_SPI_DIN, (data & 0x80) ? HIGH : LOW);
        data <<= 1;
        digitalWrite(PIN_SPI_SCK, HIGH);
        digitalWrite(PIN_SPI_SCK, LOW);
    }
    digitalWrite(PIN_SPI_CS, HIGH);
}

// SPI发送命令
inline void EPD_SendCommand(uint8_t command) {
    digitalWrite(PIN_SPI_DC, LOW);
    digitalWrite(PIN_SPI_CS, LOW);
    for (int i = 0; i < 8; i++) {
        digitalWrite(PIN_SPI_DIN, (command & 0x80) ? HIGH : LOW);
        command <<= 1;
        digitalWrite(PIN_SPI_SCK, HIGH);
        digitalWrite(PIN_SPI_SCK, LOW);
    }
    digitalWrite(PIN_SPI_CS, HIGH);
}

// 等待屏幕空闲
inline void EPD_WaitUntilIdle() {
    while(digitalRead(PIN_SPI_BUSY) == 0) delay(10);
}

// 等待屏幕busy=1时空闲
inline void EPD_WaitUntilIdle_high() {
    while(digitalRead(PIN_SPI_BUSY) == 1) delay(10);
}

// 复位
inline void EPD_Reset() {
    digitalWrite(PIN_SPI_RST, HIGH); delay(200);
    digitalWrite(PIN_SPI_RST, LOW);  delay(5);
    digitalWrite(PIN_SPI_RST, HIGH); delay(200);
}

// LUT表（2.9寸单色）
const uint8_t lut_full_mono[30] = {
    0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22, 
    0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88, 
    0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51, 
    0x35, 0x51, 0x51, 0x19, 0x01, 0x00
};

// 发送带参数命令
inline void EPD_Send_1(uint8_t c, uint8_t v1) {
    EPD_SendCommand(c);
    EPD_SendData(v1);
}
inline void EPD_Send_2(uint8_t c, uint8_t v1, uint8_t v2) {
    EPD_SendCommand(c);
    EPD_SendData(v1);
    EPD_SendData(v2);
}
inline void EPD_Send_3(uint8_t c, uint8_t v1, uint8_t v2, uint8_t v3) {
    EPD_SendCommand(c);
    EPD_SendData(v1);
    EPD_SendData(v2);
    EPD_SendData(v3);
}
inline void EPD_Send_4(uint8_t c, uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4) {
    EPD_SendCommand(c);
    EPD_SendData(v1);
    EPD_SendData(v2);
    EPD_SendData(v3);
    EPD_SendData(v4);
}

// 写LUT
inline void EPD_lut(uint8_t c, uint8_t l, const uint8_t* p) {
    EPD_SendCommand(c);
    for (int i = 0; i < l; i++) EPD_SendData(p[i]);
}

// 2.9寸V2墨水屏专用接口声明（实现见epd2in9.h）
int EPD_Init_2in9_V2();
void EPD_2IN9_V2_Show();

#endif // _EPD_H_
