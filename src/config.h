#ifndef CONFIG_H
#define CONFIG_H


// Key pinout
#define BUTTON_DOWN   3
#define BUTTON_UP     4
#define BUTTON_LEFT  A1
#define BUTTON_RIGHT A0
#define K_FIRE       10
// TFT pinout
#define TFT_RST       9 
#define TFT_DC        8
#define TFT_CS        7
#define TFT_BL       A7
#define TFT_MOSI     11
#define TFT_SCLK     13

// episode level
#define E1M1
//#define DISPLAY_FPS  
//#define DISPLAY_DEBUG_POS
#define RENDER_WALLS
//#define RENDER_SPRITES
//#define USE_16BPP_BUFFER  

//#define GRADIENT_CEILING
#define TFT_16BPP_DARK_GREY  0x8410
#define TFT_16BPP_SILVER     0xC618
#define TFT_8BPP_DARK_GRAY   0x17
#define TFT_8BPP_SILVER      0x11

#define FLOOR_COLOR_16       TFT_16BPP_SILVER 
#define CEILING_COLOR_16     TFT_16BPP_DARK_GREY
#define FLOOR_COLOR          TFT_8BPP_SILVER
#define CEILING_COLOR        TFT_8BPP_DARK_GRAY
#define CRITICAL_HEALTH_THRESHOLD         128

#define HUD_HEIGHT 8

const float COLLISION_BUFFER = 0.3f;

// My aliexpress displays 

//#define ST7735_160X80_BLUE_PCB
//#define ST7735_160X80_BLACK_PCB
//#define ST7735_160X128
//#define ST7789_240X135  
//#define ST7789_240X240
//#define ST7789_240X240_240X150
//#define ST7735_128X128 //not 5V I/O tolerant 
//#define ST7789_170X320
//#define ILI9342_320X240 
//  O-LED Displays
#define SSD1331_96X64    
//#define SSD1351_128X128

/*  Bit Name  -	Value -	 Meaning
---------------------------------------------------------
    SWAP_XY	    0x20	Rotate 90° (X ↔ Y)
    MIRROR_X	0x40	Flip horizontally (X axis)
    MIRROR_Y	0x80	Flip vertically (Y axis)
    FLIP_X	    0x02	Reverses refresh direction 
    RGB	        0x00	Use RGB (not BGR) */

#define ADDRESS_MODE_MIRROR_Y      0x80
#define ADDRESS_MODE_MIRROR_X      0x40
#define ADDRESS_MODE_SWAP_XY       0x20
#define ADDRESS_MODE_BGR           0x08
#define ADDRESS_MODE_RGB           0x00
#define ADDRESS_MODE_FLIP_X        0x02


#ifdef ST7735_160X128
#define ST77XX
#undef  DISPLAY_INVERT
#define DISPLAY_ADDRESS_MODE  (ADDRESS_MODE_FLIP_X | ADDRESS_MODE_SWAP_XY| /*ADDRESS_MODE_MIRROR_X |*/ ADDRESS_MODE_RGB)
#define SCREEN_WIDTH    160
#define SCREEN_HEIGHT   128

#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0
#define FOV               0.70f
#define COLUMN_SKIP       0 // columns to skip
//#define LOW_RES_RENDER_2X  // For larger displays. Scale the image by 100% Looks better than column skipping ?

#elif defined(ST7735_128X128)
#define ST77XX
#define DISPLAY_ADDRESS_MODE  (ADDRESS_MODE_FLIP_X | ADDRESS_MODE_SWAP_XY| ADDRESS_MODE_MIRROR_X| ADDRESS_MODE_RGB)
#define DISPLAY_INVERT
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   128
#define DISPLAY_OFFSET_X  1
#define DISPLAY_OFFSET_Y  2
#define FOV               0.66f
#define COLUMN_SKIP       0 // columns to skip
//#define LOW_RES_RENDER_2X  // For larger displays. Scale the image by 100% Looks better than column skipping ?

#elif defined(ST7789_240X135)
#define ST77XX
#define DISPLAY_ADDRESS_MODE  ( ADDRESS_MODE_FLIP_X | ADDRESS_MODE_SWAP_XY | ADDRESS_MODE_MIRROR_X| ADDRESS_MODE_RGB)
//#define DISPLAY_ADDRESS_MODE 0xA0
//#define FLIPP_Y_IN_SOFTWARE // issues with my 135x240 displays 

#define DISPLAY_INVERT
#define SCREEN_WIDTH     240
#define SCREEN_HEIGHT    135
#define DISPLAY_OFFSET_X  40
#define DISPLAY_OFFSET_Y  53
#define FOV               0.80f
#define COLUMN_SKIP       0// columns to skip
//#define LOW_RES_RENDER_2X  // For "larger" displays. Use half display size,then scale up 100% (2x2)

#elif defined(ST7789_240X240)
#define ST77XX
#define DISPLAY_ADDRESS_MODE ADDRESS_MODE_RGB // or ADDRESS_MODE_BGR if your screen is BGR and not RGB
#define DISPLAY_INVERT                        // invert the display 
#define SCREEN_WIDTH      240
#define SCREEN_HEIGHT     240
#define VIEWPORT_WIDTH    240  // virtual screen - only use this area of the screen
#define VIEWPORT_HEIGHT   200  // ------  ""  ------
#define DISPLAY_OFFSET_X    0 // if the display needs X offsets 
#define DISPLAY_OFFSET_Y    0 // if the display needs Y offsets
#define FOV               0.55f // Field of view. Should be between .50f to 1.10f for most screens.
#define COLUMN_SKIP       3 // Columns to skip (draw one vertical line then skip "COLUMN_SKIP" lines  )
//#define LOW_RES_RENDER_2X  // For "larger" displays. Scale the image by 100% Looks better than column skipping ?

#elif defined(ST7735_160X80_BLUE_PCB)
#define ST77XX
#define DISPLAY_ADDRESS_MODE  (ADDRESS_MODE_FLIP_X | ADDRESS_MODE_SWAP_XY| ADDRESS_MODE_MIRROR_X| ADDRESS_MODE_BGR)
#define DISPLAY_INVERT
#define SCREEN_WIDTH      160
#define SCREEN_HEIGHT      80
#define DISPLAY_OFFSET_X    1
#define DISPLAY_OFFSET_Y   26
#define FOV               0.88f
#define COLUMN_SKIP       0 // columns to skip
//#define LOW_RES_RENDER_2X  // For "larger" displays. Scale the image by 100% Looks better than column skipping ?
#define BUFFER_WIDTH      1 // with LOW_RES_RENDER_2X, and a SCREEN_HEIGHT of 80 we can buffer 40 columns or 1/4th of the screen

#elif defined(ST7735_160X80_BLACK_PCB)
#define ST77XX
#define DISPLAY_ADDRESS_MODE  (ADDRESS_MODE_FLIP_X | ADDRESS_MODE_SWAP_XY| ADDRESS_MODE_MIRROR_X| ADDRESS_MODE_BGR)
//#define DISPLAY_INVERT
#define SCREEN_WIDTH      160
#define SCREEN_HEIGHT      80
#define DISPLAY_OFFSET_X    0
#define DISPLAY_OFFSET_Y   24
#define FOV               0.88f
#define COLUMN_SKIP       1 // columns to skip
//#define LOW_RES_RENDER_2X  // For "larger" displays. Scale the image by 100% Looks better than column skipping ?
#define BUFFER_WIDTH      1 // with LOW_RES_RENDER_2X, and a SCREEN_HEIGHT of 80 we can buffer 40 columns or 1/4th of the screen


#elif defined(ST7789_170X320)
#define ST77XX
#define DISPLAY_ADDRESS_MODE  (ADDRESS_MODE_FLIP_X | ADDRESS_MODE_SWAP_XY| ADDRESS_MODE_MIRROR_X| ADDRESS_MODE_RGB)

#define DISPLAY_INVERT
#define SCREEN_WIDTH     320
#define SCREEN_HEIGHT    170
#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  35

#elif defined(ILI9342_320X240)
#define DEFAULT_DISPLAY_TYPE ILI934X
//#define DISPLAY_ADDRESS_MODE  (ADDRESS_MODE_FLIP_X | ADDRESS_MODE_SWAP_XY| ADDRESS_MODE_MIRROR_X| ADDRESS_MODE_RGB)
#define    DISPLAY_ADDRESS_MODE ADDRESS_MODE_RGB | ADDRESS_MODE_SWAP_XY
#define DISPLAY_INVERT
#define SCREEN_WIDTH     320
#define SCREEN_HEIGHT    240
#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0

// O-LED

#elif defined(SSD1331_96X64)
#define SSD1331
#define SSD1331_COLOR_MODE_RGB
#define SCREEN_WIDTH      96
#define SCREEN_HEIGHT     64
#define DISPLAY_OFFSET_X   0
#define DISPLAY_OFFSET_Y   0
#define FOV               0.66f
#define COLUMN_SKIP       0 // columns to skip

#elif defined(SSD1351_128X128)
#define SSD1351
#define SCREEN_WIDTH      128
#define SCREEN_HEIGHT     128
#define VIEWPORT_WIDTH    128  // virtual screen - only use this area of the screen
#define VIEWPORT_HEIGHT   90  // ------  ""  ------
#define DISPLAY_OFFSET_X   0
#define DISPLAY_OFFSET_Y   0
#define FOV               0.66f
#define COLUMN_SKIP       0 // columns to skip
//#define LOW_RES_RENDER_2X  // For "larger" displays. Scale the image by 100% Looks better than column skipping ?
#else
#define DEFAULT_DISPLAY_TYPE 0
#error "No display defined "

#endif


#if defined(DISPLAY_FPS) || defined(DISPLAY_DEBUG_POS)
    #define FPS_BAR_HEIGHT HUD_HEIGHT
#else
    #define FPS_BAR_HEIGHT 0
#endif


#ifndef VIEWPORT_WIDTH
#define VIEWPORT_WIDTH  SCREEN_WIDTH
#endif 

#ifndef VIEWPORT_HEIGHT
  #define VIEWPORT_HEIGHT (SCREEN_HEIGHT - FPS_BAR_HEIGHT)
#endif 

#ifdef LOW_RES_RENDER_2X
  #define RAYCASTER_SCREEN_WIDTH  (VIEWPORT_WIDTH / 2)
  #define RAYCASTER_SCREEN_HEIGHT (VIEWPORT_HEIGHT / 2)
  #define DISPLAY_SCALE 2
#else
  #define RAYCASTER_SCREEN_WIDTH  VIEWPORT_WIDTH
  #define RAYCASTER_SCREEN_HEIGHT VIEWPORT_HEIGHT
  #define DISPLAY_SCALE 1
#endif

#define FPS_Y (SCREEN_HEIGHT - FPS_BAR_HEIGHT)

#ifndef BUFFER_WIDTH
#define BUFFER_WIDTH 1
#endif

#define LARGE_FLOAT                        1e30f  // Define a large float value
#define TEXTURE_SIZE                       32


//#define CS_LOW()   PORTB &= ~(1 << PB0)
//#define CS_HIGH()  PORTB |=  (1 << PB0)


#endif
