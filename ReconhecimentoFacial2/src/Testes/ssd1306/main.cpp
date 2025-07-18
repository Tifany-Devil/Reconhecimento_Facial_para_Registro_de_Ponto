#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include "glcdfont.h"

#define SSD1306_ADDR 0x3C
int fd;
uint8_t buffer[8][128];

void cmd(uint8_t c) { wiringPiI2CWriteReg8(fd, 0x00, c); }
void data(uint8_t d) { wiringPiI2CWriteReg8(fd, 0x40, d); }

void initDisplay() {
  cmd(0xAE); cmd(0x20); cmd(0x00); cmd(0xB0);
  cmd(0xC8); cmd(0x00); cmd(0x10); cmd(0x40);
  cmd(0x81); cmd(0x7F); cmd(0xA1); cmd(0xA6);
  cmd(0xA8); cmd(0x3F); cmd(0xA4); cmd(0xD3); cmd(0x00);
  cmd(0xD5); cmd(0x80); cmd(0xD9); cmd(0xF1);
  cmd(0xDA); cmd(0x12); cmd(0xDB); cmd(0x40);
  cmd(0x8D); cmd(0x14); cmd(0xAF);
}

void clearBuffer() { memset(buffer, 0, sizeof(buffer)); }

void updateDisplay() {
  for (int page = 0; page < 8; page++) {
    cmd(0xB0 + page); cmd(0x00); cmd(0x10);
    for (int col = 0; col < 128; col++) data(buffer[page][col]);
  }
}

void drawPixel(int x, int y) {
  if (x<0||x>=128||y<0||y>=64) return;
  buffer[y/8][x] |= (1<<(y%8));
}

void drawChar(int x, int y, char c) {
  if(c<32||c>127) c='?';
  for(int i=0;i<5;i++){
    uint8_t line=glcdfont[(c-32)*5+i];
    for(int j=0;j<8;j++) if(line&(1<<j)) drawPixel(x+i,y+j);
  }
}

void drawString(int x,int y,const char* s){while(*s){drawChar(x,y,*s++);x+=6;if(x+6>128){x=0;y+=8;}}}

int main(){
  wiringPiSetup(); fd=wiringPiI2CSetup(SSD1306_ADDR);
  if(fd<0) return -1;
  initDisplay(); clearBuffer();
  drawString(0,0,"Ola, Raspberry Pi!");
  drawString(0,8,"Linha 2 de texto");
  drawString(0,16,"SSD1306 + C++");
  updateDisplay(); sleep(10); cmd(0xAE);
  return 0;
}
