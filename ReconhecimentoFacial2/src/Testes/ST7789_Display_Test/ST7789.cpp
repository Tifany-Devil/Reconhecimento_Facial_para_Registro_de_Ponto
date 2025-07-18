#include "ST7789.h"
#include <unistd.h>

ST7789::ST7789(uint8_t dc, uint8_t rst) : _dc(dc), _rst(rst) {
    bcm2835_gpio_fsel(_dc, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(_rst, BCM2835_GPIO_FSEL_OUTP);
}

void ST7789::sendCommand(uint8_t cmd) {
    bcm2835_gpio_write(_dc, LOW);
    bcm2835_spi_transfer(cmd);
}

void ST7789::sendData(uint8_t data) {
    bcm2835_gpio_write(_dc, HIGH);
    bcm2835_spi_transfer(data);
}

void ST7789::sendDataBuffer(uint8_t* buff, int len) {
    bcm2835_gpio_write(_dc, HIGH);
    bcm2835_spi_writenb((char*)buff, len);
}

void ST7789::reset() {
    bcm2835_gpio_write(_rst, HIGH);
    usleep(100000);
    bcm2835_gpio_write(_rst, LOW);
    usleep(100000);
    bcm2835_gpio_write(_rst, HIGH);
    usleep(100000);
}

void ST7789::init() {
    reset();
    sendCommand(0x36); sendData(0x70); // Memory access control
    sendCommand(0x3A); sendData(0x05); // Interface Pixel Format (16bit)

    sendCommand(0xB2); // Porch control
    sendData(0x0C); sendData(0x0C); sendData(0x00); sendData(0x33); sendData(0x33);

    sendCommand(0xB7); sendData(0x35); // Gate control
    sendCommand(0xBB); sendData(0x19);
    sendCommand(0xC0); sendData(0x2C);
    sendCommand(0xC2); sendData(0x01);
    sendCommand(0xC3); sendData(0x12);
    sendCommand(0xC4); sendData(0x20);
    sendCommand(0xC6); sendData(0x0F);
    sendCommand(0xD0); sendData(0xA4); sendData(0xA1);

    sendCommand(0xE0); // Positive voltage gamma control
    uint8_t pos_gamma[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
    sendDataBuffer(pos_gamma, 14);

    sendCommand(0xE1); // Negative voltage gamma control
    uint8_t neg_gamma[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
    sendDataBuffer(neg_gamma, 14);

    sendCommand(0x21); // Inversion ON
    sendCommand(0x11); // Sleep out
    usleep(120000);
    sendCommand(0x29); // Display ON
}

void ST7789::drawPixel(uint16_t x, uint16_t y, uint16_t color) {
    sendCommand(0x2A); // column
    sendData(x >> 8); sendData(x & 0xFF);
    sendData(x >> 8); sendData(x & 0xFF);

    sendCommand(0x2B); // row
    sendData(y >> 8); sendData(y & 0xFF);
    sendData(y >> 8); sendData(y & 0xFF);

    sendCommand(0x2C); // write RAM
    sendData(color >> 8); sendData(color & 0xFF);
}

void ST7789::fillScreen(uint16_t color) {
    sendCommand(0x2A); sendData(0); sendData(0); sendData(0); sendData(239);
    sendCommand(0x2B); sendData(0); sendData(0); sendData(0); sendData(239);
    sendCommand(0x2C);
    uint8_t high = color >> 8;
    uint8_t low = color & 0xFF;
    bcm2835_gpio_write(_dc, HIGH);
    for (int i = 0; i < 240 * 240; ++i) {
        bcm2835_spi_transfer(high);
        bcm2835_spi_transfer(low);
    }
}
