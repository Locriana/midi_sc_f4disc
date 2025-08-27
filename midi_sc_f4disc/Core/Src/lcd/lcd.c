#include "main.h"
#include  <errno.h>
#include  <sys/unistd.h>
#include  <string.h>
#include <stdarg.h>
#include "lcd.h"
#include "lcd_chr.h"
#include "stm32f429i_discovery_lcd.h"


uint32_t lcdFont;
uint32_t lcdColor;
uint32_t lcdBackgroundColor;
uint8_t lcdTransparent;
static uint8_t widthTest;

char tempStr[LCD_MAX_STR_LEN];
int textCursorX = 0;
int textCursorY = 0;
int textLeftAlignX = 0;

#define USE_DOUBLE_BUFFER   0

//static uint32_t* lcdImage = (uint32_t*)(LCD_FRAME_BUFFER + LCD_X_SIZE*LCD_Y_SIZE*sizeof(uint32_t));
//static uint32_t* lcdImage = (uint32_t*)(img_foreground);
#if LCD_LANDSCAPE
  uint32_t lcdBuffer[LCD_X_SIZE][LCD_Y_SIZE] __attribute__((section(".sdram")));
#else
  uint32_t lcdImage[LCD_Y_SIZE][LCD_X_SIZE] __attribute__((section(".sdram")));
#endif
//static uint32_t img_background[LCD_Y_SIZE][LCD_X_SIZE] __attribute__((section(".sdram")));

//=================================================================================================
//                  DISPLAY TYPE DEPENDENT FUNCS
//=================================================================================================







void lcdInvertPixel(int32_t x, int32_t y)
{
  if(x>(LCD_X_SIZE-1)) return;
  if(y>(LCD_Y_SIZE-1)) return;
  if(x<0) return;
  if(y<0) return;
  #if LCD_LANDSCAPE
    uint32_t before = lcdImage[x][y];
    uint32_t after = 0xFF000000 | (~before);
    #if LCD_LANDSCAPE_FROM_L
      lcdImage[x][LCD_Y_SIZE-y-1] = after;
    #else
      lcdImage[LCD_X_SIZE-x-1][y] = after;
    #endif
    
  #else
    uint32_t before = lcdImage[y][x];
    uint32_t after = 0xFF000000 | (~before);
    lcdImage[y][x] = after;
  #endif
  //printf("*inv: %08X to %08X\n",(unsigned int)before,(unsigned int)after);
}


void lcdPixel(int32_t x, int32_t y, uint32_t color)
{
  if(x>(LCD_X_SIZE-1)) return;
  if(y>(LCD_Y_SIZE-1)) return;
  if(x<0) return;
  if(y<0) return;
  #if LCD_LANDSCAPE
    #if LCD_LANDSCAPE_FROM_L
      lcdImage[x][LCD_Y_SIZE-y-1] = color;
    #else
      lcdImage[LCD_X_SIZE-x-1][y] = color;
    #endif
  #else
    lcdImage[y][x] = color;
  #endif
}





//=================================================================================================
//                  DISPLAY INDEPENDENT FUNCS
//=================================================================================================



void lcdRectangle(int32_t x1, int32_t x2, int32_t y1, int32_t y2, uint8_t fill)
{
  lcdHorizontalLine(x1,x2,y1);
  lcdHorizontalLine(x1,x2,y2);
  lcdVerticalLine(x1,y1,y2);
  lcdVerticalLine(x2,y1,y2);
  
  if(fill)
  {
    int32_t x;
    for(x=x1+1;x<x2;x++)
    {
      lcdVerticalLine(x,y1,y2);
    }
  }
}

//draws horizontal byte contents (1 = color, 0 = background/unchanged, depending on transparency settings)
//coordinates x,y are coords of the first left pixel in given byte, so (0xFF,0,0) will place horizontal 8-pix line from pixels x=0 to x=7 and y=0
void lcdHorizontalByte(uint8_t data, int32_t x, int32_t y)
{
  if(x>LCD_X_SIZE) return;
  if(y>LCD_Y_SIZE) return;
  if(x<-8) return;
  if(y<0) return;
  int32_t curX=x;
  uint8_t i;
  switch(lcdTransparent)
  {
    case LCD_OPAQUE:
      for(i=0;i<8;i++)
      {
        if(data & 0x80) lcdPixel(curX,y,lcdColor); else lcdPixel(curX,y,lcdBackgroundColor);
        data <<= 1;
        curX++;
      }
      return;
    case LCD_TRANSPARENT:
      for(i=0;i<8;i++)
      {
        if(data & 0x80) lcdPixel(curX,y,lcdColor);
        data <<= 1;
        curX++;
      }
      return;
    case LCD_TRANSPARENT_INVERSION:
      for(i=0;i<8;i++)
      {
        if(data & 0x80) lcdInvertPixel(curX,y);
        data <<= 1;
        curX++;
      }
      return;
  }//switch(lcdTransparent)
}

//draws a vertical byte starting at x,y and ending at x,y+7
void lcdVerticalByte(uint8_t data, int32_t x, int32_t y)
{
  if(x>LCD_X_SIZE) return;
  if(y>LCD_Y_SIZE) return;
  if(x<0) return;
  if(y<-8) return;
  int32_t curY=y;
  uint8_t i;
  switch(lcdTransparent)
  {
    case LCD_OPAQUE:
      for(i=0;i<8;i++)
      {
        if(data & 0x01) lcdPixel(x,curY,lcdColor); else lcdPixel(x,curY,lcdBackgroundColor);
        data >>= 1;
        curY++;
      }
      return;
    case LCD_TRANSPARENT:
      for(i=0;i<8;i++)
      {
        if(data & 0x01) lcdPixel(x,curY,lcdColor);
        data >>= 1;
        curY++;
      }
      return;
    case LCD_TRANSPARENT_INVERSION:
      for(i=0;i<8;i++)
      {
        if(data & 0x01) lcdInvertPixel(x,curY);
        data >>= 1;
        curY++;
      }
      return;
  }//switch(lcdTransparent)
}

//draws a vertical line
void lcdVerticalLine(int32_t x, int32_t y1, int32_t y2)
{
  int32_t currentY;
  if(lcdTransparent == LCD_TRANSPARENT_INVERSION)
    for(currentY = y1; currentY<=y2; currentY++) lcdInvertPixel(x,currentY);
  else
    for(currentY = y1; currentY<=y2; currentY++) lcdPixel(x,currentY,lcdColor);
}

//draws a horizontal line
void lcdHorizontalLine(int32_t x1, int32_t x2, int32_t y)
{
  int32_t currentX;
  if(lcdTransparent == LCD_TRANSPARENT_INVERSION)
  {
    for(currentX=x1;currentX<=x2;currentX++) lcdInvertPixel(currentX,y);
  }
  else
  {
    for(currentX=x1;currentX<=x2;currentX++) lcdPixel(currentX,y,lcdColor);
  }
}

//inverts colors in selected area
void lcdInvertArea(int32_t x1, int32_t x2, int32_t y1, int32_t y2)
{
  int32_t currentX, currentY;
  
  for(currentY=y1;currentY<=y2;currentY++)
    for(currentX=x1;currentX<=x2;currentX++)
      lcdInvertPixel(currentX,currentY);
}

//clears area to background color
void lcdClearArea(int32_t x1, int32_t x2, int32_t y1, int32_t y2)
{
  int32_t currentX, currentY;
  
  for(currentY=y1;currentY<=y2;currentY++)
    for(currentX=x1;currentX<=x2;currentX++)
      lcdPixel(currentX,currentY,lcdBackgroundColor);
  
}

//clears the shadow
void lcdClear(void)
{
  //BSP_LCD_Clear(lcdBackgroundColor);
  lcdClearArea(0,LCD_X_SIZE,0,LCD_Y_SIZE);
}



void lcdSetTransparent(uint8_t transparent)
{
  lcdTransparent = transparent;
}

uint8_t lcdGetTransparent(void)
{
  return(lcdTransparent);
}


void lcdSetColor(uint32_t color)
{
  lcdColor = color;
}


uint32_t lcdGetColor(void)
{
  return(lcdColor);
}


void lcdSetFont(uint32_t font)
{
  lcdFont = font;
}


uint32_t lcdGetFont(void)
{
  return(lcdFont);
}

uint32_t lcdGetFontSize(uint32_t font)
{
  switch(font)
  {
    case LCD_FONT_9: return(10);
    case LCD_FONT_5:
    case LCD_FONT_6:
    case LCD_FONT_7:
    case LCD_FONT_10:
    case LCD_FONT_15:
    case LCD_FONT_18:
    case LCD_FONT_40:
      return(font);
    default:
      return(0);
  }
}


void lcdWidthTest(uint8_t value)
{
  if(value) widthTest=1; else widthTest=0;
}

uint8_t lcdWidthTestStatus(void)
{
  return widthTest;
}


int lcdSetTextCursor(int x, int y)
{
  textCursorX = x;
  textCursorY = y;
  return 0;
}

int lcdGetTextCursor(int* x, int* y)
{
  *x = textCursorX;
  *y = textCursorY;
  return 0;
}

void lcdMoveTextCursorX(int shift)
{
  textCursorX += shift;
}

void lcdMoveTextCursorY(int shift)
{
  textCursorY += shift;
}

int lcdGetCursorX(void)
{
  return textCursorX;
}

int lcdGetCursorY(void)
{
  return textCursorY;
}

void lcdSetCursorX(int x)
{
  textCursorX = x;
}

void lcdSetCursorY(int y)
{
  textCursorY = y;
}



int lcdSetTextLeftAlignX(int x)
{
  textLeftAlignX = x;
  return 0;
}

static void lcd_chr_raw(char c)
{
  if(c=='\n')
  {
    textCursorY=textCursorY + (float)lcdGetFont()*1.2;
    textCursorX = textLeftAlignX;
  }
  else
  {
    textCursorX = lcdChr(c,textCursorX,textCursorY);
  }
  
}

static void lcd_string_raw(const char* str)
{
  //display string on lcd here
  while (*str)
    lcd_chr_raw(*str++);
}


static void lcdxitoa (long val, int radix, int len)
{
  uint8_t c, r, sgn = 0, pad = ' ';
  uint8_t s[20], i = 0;
  uint32_t v;


  if (radix < 0) {
    radix = -radix;
    if (val < 0) {
      val = -val;
      sgn = '-';
    }
  }
  v = val;
  r = radix;
  if (len < 0) {
    len = -len;
    pad = '0';
  }
  if (len > 20) return;
  do {
    c = (uint8_t)(v % r);
    if (c >= 10) c += 7;
    c += '0';
    s[i++] = c;
    v /= r;
  } while (v);
  if (sgn) s[i++] = sgn;
  while (i < len)
    s[i++] = pad;
  do
    lcd_chr_raw(s[--i]);
  while (i);
}



// Core function: works like vprintf
void lcd_va(const char* str, va_list arp) {
    int d, r, w, s, l;

    while ((d = *str++) != 0) {
        if (d != '%') {
            lcd_chr_raw(d);
            continue;
        }
        d = *str++;
        w = r = s = l = 0;
        if (d == '0') {
            d = *str++;
            s = 1;
        }
        while ((d >= '0') && (d <= '9')) {
            w += w * 10 + (d - '0');
            d = *str++;
        }
        if (s) w = -w;
        if (d == 'l') {
            l = 1;
            d = *str++;
        }
        if (!d) break;
        if (d == 's') {
            lcd_string_raw(va_arg(arp, char*));
            continue;
        }
        if (d == 'c') {
            lcd_chr_raw((char)va_arg(arp, int));
            continue;
        }
        if (d == 'u') r = 10;
        if (d == 'd') r = -10;
        if (d == 'X' || d == 'x') r = 16;
        if (d == 'b') r = 2;
        if (!r) break;
        if (l) {
            lcdxitoa((long)va_arg(arp, long), r, w);
        } else {
            if (r > 0)
                lcdxitoa((unsigned long)va_arg(arp, int), r, w);
            else
                lcdxitoa((long)va_arg(arp, int), r, w);
        }
    }
}

// Your public function: like printf
void lcd(const char* str, ...) {
  va_list arp;
  va_start(arp, str);
  lcd_va(str, arp);  // call core with va_list
  va_end(arp);
}



int32_t lcdCentered(const char* text, ...)
{
  va_list arp;
  va_start(arp, text);

  lcdWidthTest(1);
  //textCursorX = x;
  //textCursorY = y;
  int x_start = textCursorX;

  lcd_va(text, arp);

  int32_t len = textCursorX - x_start;

  lcdWidthTest(0);

  textCursorX = x_start -(len/2);

  lcd_va(text, arp);

  textCursorX = x_start;

  va_end(arp);
  return(textCursorX);
}




void lcdSetBackgroundColor(uint32_t color)
{
  lcdBackgroundColor = color;
}


uint32_t lcdGetBackgroundColor(void)
{
  return(lcdBackgroundColor);
}


void lcdInit(void)
{
  BSP_LCD_Init();

#if USE_DOUBLE_BUFFER
  BSP_LCD_LayerDefaultInit(1, (uint32_t)lcdBuffer);
#else
  BSP_LCD_LayerDefaultInit(1, (uint32_t)lcdImage);
#endif


  lcdSetBackgroundColor(LCD_COLOR_BLACK);
  lcdSetColor(LCD_COLOR_WHITE);
  lcdSetTransparent(LCD_OPAQUE);
  lcdSetFont(LCD_FONT_7);
  lcdClear();
}


void lcdOff(void)
{
  
}

void lcdUpdate(void)
{
  #if USE_DOUBLE_BUFFER
    memcpy(lcdBuffer,lcdImage,LCD_X_SIZE*LCD_Y_SIZE*sizeof(uint32_t));
  #endif
}
