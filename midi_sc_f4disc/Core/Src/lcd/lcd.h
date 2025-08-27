#ifndef __LCD_H__
#define __LCD_H__

#include "main.h"
#include "lcd_chr.h"
#include "stm32f429i_discovery_lcd.h"


//=================================================================================================
//                LCD DEFS
//=================================================================================================




typedef uint32_t lcdTPixel;


#define LCD_LANDSCAPE       0

#ifndef LCD_LANDSCAPE_FROM_L
  #define LCD_LANDSCAPE_FROM_L  1
#endif


#if LCD_LANDSCAPE
  #define LCD_X_SIZE          320
  #define LCD_Y_SIZE          240
#else
  #define LCD_X_SIZE          240
  #define LCD_Y_SIZE          320
#endif
#define LCD_SHADOW_SIZE       (LCD_X_SIZE*LCD_Y_SIZE)


#define LCD_FONT_5          5
#define LCD_FONT_6          6
#define LCD_FONT_7          7
#define LCD_FONT_9          9
#define LCD_FONT_10         10
#define LCD_FONT_15         15
#define LCD_FONT_18         18
#define LCD_FONT_40         40
#define LCD_FONT_80         80

#define LCD_TRANSPARENT_INVERSION 2
#define LCD_TRANSPARENT           1
#define LCD_OPAQUE                0

#define LCD_OBJECT_FILL       1
#define LCD_OBJECT_NO_FILL    0
#define LCD_OBJECT_INVERT     1
#define LCD_OBJECT_NO_INVERT  0

#define LCD_MAX_STR_LEN       50


//clears the shadow
void lcdClear(void);
void lcdInvertPixel(int32_t x, int32_t y);
void lcdPixel(int32_t x, int32_t y, uint32_t color);
void lcdRectangle(int32_t x1, int32_t x2, int32_t y1, int32_t y2, uint8_t fill);
void lcdHorizontalByte(uint8_t data, int32_t x, int32_t y);
void lcdVerticalByte(uint8_t data, int32_t x, int32_t y);
void lcdVerticalLine(int32_t x, int32_t y1, int32_t y2);
void lcdHorizontalLine(int32_t x1, int32_t x2, int32_t y);
void lcdInvertArea(int32_t x1, int32_t x2, int32_t y1, int32_t y2);
void lcdClearArea(int32_t x1, int32_t x2, int32_t y1, int32_t y2);
void lcdSetTransparent(uint8_t transparent);
uint8_t lcdGetTransparent(void);
void lcdSetColor(uint32_t color);
uint32_t lcdGetColor(void);
void lcdSetFont(uint32_t font);
uint32_t lcdGetFont(void);
uint32_t lcdGetFontSize(uint32_t font);
void lcdWidthTest(uint8_t value);
uint8_t lcdWidthTestStatus(void);
int lcdSetTextCursor(int x, int y);
int lcdGetTextCursor(int* x, int* y);
void lcdSetCursorX(int x);
void lcdSetCursorY(int y);
void lcdMoveTextCursorX(int shift);
void lcdMoveTextCursorY(int shift);
int lcdSetTextLeftAlignX(int x);
void lcd(const char* str, ...);
void lcdSetBackgroundColor(uint32_t color);
uint32_t lcdGetBackgroundColor(void);
int32_t lcdCentered(const char* text, ...);
void lcdInit(void);
void lcdOff(void);
void lcdUpdate(void);

#endif

