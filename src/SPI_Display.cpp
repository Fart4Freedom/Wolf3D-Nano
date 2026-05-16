#include "SPI_Display.h"
#include "config.h"



SPI_DISPLAY::SPI_DISPLAY() {}

void SPI_DISPLAY::begin() {

    pinMode(TFT_CS, OUTPUT);
    pinMode(TFT_DC, OUTPUT);
    pinMode(TFT_RST, OUTPUT);
    pinMode(SS, OUTPUT);
    pinMode(SCK, OUTPUT);
    pinMode(MOSI, OUTPUT);

    digitalWrite(TFT_CS, HIGH);
    DDRB |= (1 << PB3) | (1 << PB5);
    SPCR = (1 << SPE) | (1 << MSTR);                
    SPSR |= (1 << SPI2X);     
    
    
    digitalWrite(TFT_RST, LOW);  delay(50);
    digitalWrite(TFT_RST, HIGH); delay(50);
#if defined(SSD1331)  
    
    DisplayWriteCMD(0xAE); // Display Off

    DisplayWriteCMD(0xA0); // Set Re-map & Color Depth
    DisplayWriteCMD(0x72); // RGB Color (or 0x76 for BGR)

    DisplayWriteCMD(0xA1); // Set Display Start Line
    DisplayWriteCMD(0x00);

    DisplayWriteCMD(0xA2); // Set Display Offset
    DisplayWriteCMD(0x00);

    DisplayWriteCMD(0xA4); // Normal Display (resume to RAM content)

    DisplayWriteCMD(0xA8); // Set Multiplex Ratio
    DisplayWriteCMD(0x3F); // 0x3F = 63 (for 64 lines)

    DisplayWriteCMD(0xAF); // Display ON
#elif defined(SSD1351)
 
    uint8_t data1[1], data3[3];

    // These are needed on many panels!
    DisplayWriteCMD(0xFD); data1[0]=0x12; DisplayWriteData(data1,1); // COMMANDLOCK
    DisplayWriteCMD(0xFD); data1[0]=0xB1; DisplayWriteData(data1,1); // COMMANDLOCK
    DisplayWriteCMD(0xAE); // DISPLAYOFF
    DisplayWriteCMD(0xB3); data1[0]=0xF1; DisplayWriteData(data1,1); // CLOCKDIV
    DisplayWriteCMD(0xCA); data1[0]=0x7F; DisplayWriteData(data1,1); // MUXRATIO
    DisplayWriteCMD(0xA1); data1[0]=0x00; DisplayWriteData(data1,1); // STARTLINE
    DisplayWriteCMD(0xA2); data1[0]=0x00; DisplayWriteData(data1,1); // OFFSET
    DisplayWriteCMD(0xB5); data1[0]=0x00; DisplayWriteData(data1,1); // GPIO
    DisplayWriteCMD(0xAB); data1[0]=0x01; DisplayWriteData(data1,1); // FUNCSELECT
    DisplayWriteCMD(0xB1); data1[0]=0x32; DisplayWriteData(data1,1); // PHASELEN
    DisplayWriteCMD(0xBE); data1[0]=0x05; DisplayWriteData(data1,1); // VCOMH
    DisplayWriteCMD(0xA6); // NORMALDISP
    DisplayWriteCMD(0xC1); data3[0]=0xC8; data3[1]=0x80; data3[2]=0xC8; DisplayWriteData(data3,3); // CONTRASTABC
    DisplayWriteCMD(0xC7); data1[0]=0x0F; DisplayWriteData(data1,1); // CONTRASTMASTER
    DisplayWriteCMD(0xB4); data3[0]=0xA0; data3[1]=0xB5; data3[2]=0x55; DisplayWriteData(data3,3); // SETVSL
    DisplayWriteCMD(0xB6); data1[0]=0x01; DisplayWriteData(data1,1); // PRECHARGE2
    DisplayWriteCMD(0xAF); // DISPLAYON
    DisplayWriteCMD(0xA0); data1[0]=0b01100100 | 0b00010000; DisplayWriteData(data1,1); // SETREMAP

#elif defined(ST77XX) || defined(ILI934X)
    pinMode(TFT_BL, OUTPUT);

    DisplayWriteCMD(0x01); // SOFT_RESET
    delay(200);

    DisplayWriteCMD(0x36); // MADCTL - ADDRESS_MODE
#ifdef DISPLAY_ADDRESS_MODE
    uint8_t mode1 = DISPLAY_ADDRESS_MODE;
    DisplayWriteData(&mode1, 1);
#endif 
    DisplayWriteCMD(0x3A); // PIXEL_FORMAT
    uint8_t mode2 = 0x55;  // 16bpp
    DisplayWriteData(&mode2, 1);

    DisplayWriteCMD(0x51); // WRITE_DISPLAY_BRIGHTNESS
    uint8_t brightness = 0xFF;
    DisplayWriteData(&brightness, 1);

#ifdef DISPLAY_INVERT
    DisplayWriteCMD(0x21); //ENTER_INVERT_MODE)
#else
    DisplayWriteCMD(0x20); //EXIT_INVERT_MODE
#endif

    DisplayWriteCMD(0x11); //EXIT_SLEEP_MODE
    delay(200);

    DisplayWriteCMD(0x29); //SET_DISPLAY_ON
    delay(200);

    digitalWrite(TFT_BL, HIGH); // Turn on the backlight


#endif 
   setViewport(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1); 
}

void SPI_DISPLAY::DisplayWriteCMD(uint8_t cmd) {
    digitalWrite(TFT_DC, LOW);
    digitalWrite(TFT_CS, LOW);
    SPDR = cmd;                     // send cmd to the SPI bus
    while (!(SPSR & (1 << SPIF))); // wait for the SPIF flag to clear
    digitalWrite(TFT_CS, HIGH);
}

void SPI_DISPLAY::DisplayWriteData(const uint8_t *data, int length) {
    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_CS, LOW);

    for (int i = 0; i < length; i++) {
        SPDR = data[i]; while (!(SPSR & (1 << SPIF)));
    }

    digitalWrite(TFT_CS, HIGH);
}

void SPI_DISPLAY::setViewport(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {

#if defined(SSD1331)
    // SSD1331: each address byte is a command
    DisplayWriteCMD(0x15);      // Set Column Address
    DisplayWriteCMD(x1);
    DisplayWriteCMD(x2);
    DisplayWriteCMD(0x75);      // Set Row Address
    DisplayWriteCMD(y1);
    DisplayWriteCMD(y2);
    // No RAM write command
#elif defined(SSD1351)
    // SSD1351: send command, then two data bytes
   
    DisplayWriteCMD(0x15);  // Set Column Address
    uint8_t colData[] = { (uint8_t)x1, (uint8_t)x2 };
    DisplayWriteData(colData, 2);
    
    DisplayWriteCMD(0x75); // Set Row Address
    uint8_t rowData[] = { (uint8_t)y1, (uint8_t)y2 };
    DisplayWriteData(rowData, 2);
    
    DisplayWriteCMD(0x5C); // Start RAM Write

#elif defined(ST77XX) || defined(ILI934X)
    static uint16_t prev_x1 = 0xFFFF, prev_x2 = 0xFFFF;
    static uint16_t prev_y1 = 0xFFFF, prev_y2 = 0xFFFF;

    x1 += DISPLAY_OFFSET_X;
    y1 += DISPLAY_OFFSET_Y;
    x2 += DISPLAY_OFFSET_X;
    y2 += DISPLAY_OFFSET_Y;
    // ST77xx, ST7789, ILI9341 etc. use 16-bit addressing
    if (prev_x1 != x1 || prev_x2 != x2) {
        DisplayWriteCMD(0x2A); //SET_COLUMN_ADDRESS
        uint8_t data[4] = {
            (uint8_t)(x1 >> 8), (uint8_t)(x1 & 0xFF),
            (uint8_t)(x2 >> 8), (uint8_t)(x2 & 0xFF)
        };
    
        DisplayWriteData(data, 4);
        prev_x1 = x1;
        prev_x2 = x2;
    }

    if (prev_y1 != y1 || prev_y2 != y2) {
        DisplayWriteCMD(0x2B); // SET_PAGE_ADDRESS
        uint8_t data[4] = {
            (uint8_t)(y1 >> 8), (uint8_t)(y1 & 0xFF),
            (uint8_t)(y2 >> 8), (uint8_t)(y2 & 0xFF)
        };
        DisplayWriteData(data, 4);
        prev_y1 = y1;
        prev_y2 = y2;
    }
       DisplayWriteCMD(0x2C); //WRITE_MEMORY_START
#endif
}


void SPI_DISPLAY::clearRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {


    if ((x >= SCREEN_WIDTH) || (y >= SCREEN_HEIGHT)) return;
    if ((x + w - 1) >= SCREEN_WIDTH) w = SCREEN_WIDTH - x;
    if ((y + h - 1) >= SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;

    setViewport(x, y, x + w - 1, y + h - 1);

    PORTB |= (1 << PB0); 
    PORTD &= ~(1 << PD7); 
  //  digitalWrite(TFT_DC, HIGH);  // Set to data mode
  //  digitalWrite(TFT_CS, LOW);   // Select the display
    uint16_t n = w * h;
    while (n-- > 0) {
        SPDR = 0x00; while (!(SPSR & (1 << SPIF))); 
        SPDR = 0x00; while (!(SPSR & (1 << SPIF))); 
    }

    PORTD |= (1 << PD7); 
 //   digitalWrite(TFT_CS, HIGH);  // Deselect the display 
}


//#ifdef DISPLAY_FPS

void SPI_DISPLAY::drawPixel(int x, int y, uint16_t color) {
    if ((x < 0) || (x >= SCREEN_WIDTH) || (y < 0) || (y >= SCREEN_HEIGHT)) return;

    setViewport(x, y, x, y);
    digitalWrite(TFT_DC, HIGH);  
    digitalWrite(TFT_CS, LOW);       
    
    SPDR = color >> 8; 
    while (!(SPSR & (1 << SPIF)));
    SPDR = color & 0xFF;
    while (!(SPSR & (1 << SPIF)));
    
    digitalWrite(TFT_CS, HIGH); 
}

void SPI_DISPLAY::setCursor(int16_t x, int16_t y) {
    cursorX = x;
    cursorY = y;
}

void SPI_DISPLAY::setTextColor(uint16_t color) {

    textColor = color;
}

void SPI_DISPLAY::setTextSize(uint8_t size) {
    textSize = size;
}
#include "5x7_fonts.h"
void SPI_DISPLAY::drawChar(int16_t x, int16_t y, char c, uint16_t color) {
    if (c < 32 || c > 126) {
        c = '?'; // Replace non-printable characters with '?'
    }
    c -= 32; // Convert ASCII to index in font array

    for (int8_t i = 0; i < 5; i++ ) {
        uint8_t line = pgm_read_byte(font5x7 + (c * 5) + i);
        for (int8_t j = 0; j < 8; j++) {
            if (line & 0x01) {
                if (textSize == 1) {
                    drawPixel(x + i, y + j, color);
                } else {
                    // Scale the font
                    for (uint8_t dx = 0; dx < textSize; dx++) {
                        for (uint8_t dy = 0; dy < textSize; dy++) {
                            drawPixel(x + i * textSize + dx, y + j * textSize + dy, color);
                        }
                    }
                }
            }
            line >>= 1;
        }
    }
}

void SPI_DISPLAY::print(const char *str) {
    while (*str) {
        drawChar(cursorX, cursorY, *str++, textColor);
        cursorX += 6 * textSize; 
    }
}

void SPI_DISPLAY::print(int num) {
    char buffer[12]; 
    itoa(num, buffer, 10); 
    print(buffer); 
}

void SPI_DISPLAY::print(unsigned long num) {
    char buffer[21]; 
    ultoa(num, buffer, 10); 
    print(buffer); 
}

void SPI_DISPLAY::print(float num) {
    char buf[32];  
    dtostrf(num, 1, 2, buf); 
    print(buf);
}
//#endif