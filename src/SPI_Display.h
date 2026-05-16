#ifndef SSD13xx_H
#define SSD13xx_H
#include <Arduino.h>
#include <SPI.h>
#include "config.h"


//Some RGB565 colors
#define TFT_BLACK   0x0000  
#define TFT_WHITE   0xFFFF  
#define TFT_RED     0xF800  
#define TFT_GREEN   0x07E0  
#define TFT_BLUE    0x001F  
#define TFT_YELLOW  0xFFE0  
#define TFT_CYAN    0x07FF  
#define TFT_MAGENTA 0xF81F  
#define TFT_SILVER  0xC618  
#define TFT_GREY    0x8410
#define TFT_DARKGREY 0x7BEF   
#define TFT_ORANGE  0xFD20  
#define TFT_PURPLE  0x8010  

#define TFT_8_DARKGREY 253
#define TFT_8_SILVER   255
#define TFT_8_GREY     254


class SPI_DISPLAY {
public:
    SPI_DISPLAY();
    void begin();
    void setViewport(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
    void clearRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
    void clearScreen(void){clearRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);}
// #ifdef DISPLAY_FPS ??     
    void drawPixel(int x, int y, uint16_t color);
    void setCursor(int16_t x, int16_t y);
    void setTextColor(uint16_t color);
    void setTextSize(uint8_t size);
    void drawChar(int16_t x, int16_t y, char c, uint16_t color);
    void print(const char *str);
    void print(int num);
    void print(unsigned long num);
    void print(float num);

   int16_t cursorX, cursorY; 
    uint16_t textColor = TFT_WHITE;
    uint8_t textSize = 1;

private:
    void DisplayWriteCMD(uint8_t c);
    void DisplayWriteData(const uint8_t *data, int length);
// #ifdef DISPLAY_FPS ??      
 

};

#endif

