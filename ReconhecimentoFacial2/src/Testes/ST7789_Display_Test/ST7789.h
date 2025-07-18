#ifndef ST7789_H
#define ST7789_H

#include <bcm2835.h>
#include <cstdint>

class ST7789 {
public:
    ST7789(uint8_t dc, uint8_t rst);
    void init();
    void drawPixel(uint16_t x, uint16_t y, uint16_t color);
    void fillScreen(uint16_t color);

private:
    uint8_t _dc, _rst;
    void sendCommand(uint8_t cmd);
    void sendData(uint8_t data);
    void sendDataBuffer(uint8_t* buff, int len);
    void reset();
};

#endif
