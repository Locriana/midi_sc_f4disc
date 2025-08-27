/*
* The MIT License (MIT)
* Copyright (c) 2019 Ada Brzoza-Zajęcka (Locriana)
* Permission is hereby granted, free of charge, to any person obtaining 
* a copy of this software and associated documentation files (the "Software"), 
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
* DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
* OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
* THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include  <errno.h>
#include  <sys/unistd.h>
#include  <string.h>

#include "term_io.h"
#include "dbgu.h"
#include "ansi.h"

#include "lcd.h"
#include "lcd_chr.h"

//this may be included only once
#include "font_lib.h"


//from lib
static int32_t lcdChrLib5(uint8_t chr, int32_t x, int32_t y);
static int32_t lcdChrLib6(uint8_t chr, int32_t x, int32_t y);
static int32_t lcdChrLib7(uint8_t chr, int32_t x, int32_t y);
static int32_t lcdChrLib9(uint8_t chr, int32_t x, int32_t y);
static int32_t lcdChrLib10(uint8_t chr, int32_t x, int32_t y);
static int32_t lcdChrLib15(uint8_t chr, int32_t x, int32_t y);
static int32_t lcdChrLib18(uint8_t chr, int32_t x, int32_t y);
static int32_t lcdChrLib40(uint8_t chr, int32_t x, int32_t y);
static int32_t lcdChrLib80(uint8_t chr, int32_t x, int32_t y);

static int32_t lcdChrLib5Width(uint8_t chr);
static int32_t lcdChrLib6Width(uint8_t chr);
static int32_t lcdChrLib7Width(uint8_t chr);
static int32_t lcdChrLib9Width(uint8_t chr);
static int32_t lcdChrLib10Width(uint8_t chr);
static int32_t lcdChrLib15Width(uint8_t chr);
static int32_t lcdChrLib18Width(uint8_t chr);
static int32_t lcdChrLib40Width(uint8_t chr);
static int32_t lcdChrLib80Width(uint8_t chr);


int32_t lcdChr(uint8_t chr, int32_t x, int32_t y)
{
	if(lcdWidthTestStatus())
	{
		return x+lcdChrWidth(chr);
	}
	
	switch(lcdGetFont())
	{
		case LCD_FONT_5: return(lcdChrLib5(chr,x,y));
		case LCD_FONT_6: return(lcdChrLib6(chr,x,y));
		case LCD_FONT_7: return(lcdChrLib7(chr,x,y));
		case LCD_FONT_9: return(lcdChrLib9(chr,x,y));
		case LCD_FONT_10: return(lcdChrLib10(chr,x,y));
		case LCD_FONT_15: return(lcdChrLib15(chr,x,y));
		case LCD_FONT_18: return(lcdChrLib18(chr,x,y));
		case LCD_FONT_40: return(lcdChrLib40(chr,x,y));
		case LCD_FONT_80: return(lcdChrLib80(chr,x,y));
		default:
		{
			xprintf("lcdChr: FAILED due to wrong font selected\n");
			return(-1);
		}
	}
}


int32_t lcdChrWidth(uint8_t chr)
{
	switch(lcdGetFont())
	{
		case LCD_FONT_5: return(lcdChrLib5Width(chr));
		case LCD_FONT_6: return(lcdChrLib6Width(chr));
		case LCD_FONT_7: return(lcdChrLib7Width(chr));
		case LCD_FONT_9: return(lcdChrLib9Width(chr));
		case LCD_FONT_10: return(lcdChrLib10Width(chr));
		case LCD_FONT_15: return(lcdChrLib15Width(chr));
		case LCD_FONT_18: return(lcdChrLib18Width(chr));
		case LCD_FONT_40: return(lcdChrLib40Width(chr));
		case LCD_FONT_80: return(lcdChrLib80Width(chr));
		default:
		{
			xprintf("lcdChrWidth: FAILED due to wrong font selected\n");
			return(-1);
		}
	}
}

/*#if 0
  uint8_t in[5] = {0xAA, 0x55, 0xCC, 0x13, 0x81};
  uint16_t out;
  for(int i=0;i<5;i++)
  {
    out = stretchOneToTwo(in[i]);
    xprintf("in=%02X, out=%04X\n",(unsigned int)in[i],(unsigned int)out);
  }
  vTaskDelay(5000);
#endif*/


static uint16_t stretchOneToTwo(uint8_t b)
{
  uint16_t res = 0;
  //int8_t tempb = (int8_t)b;
  for(int i=0;i<8;i++)
  {
    if( b & (1<<i) ) {
      res |= (1<<(2*i)) | (1<<(2*i+1));
    }
    
  }
  return res;
}

static int32_t lcdChrLib80(uint8_t chr, int32_t x, int32_t y)
{
	const uint8_t size = 4*40;	//size in bytes of one glyph
	const uint8_t starting_char = '-'; //first glyph char
	const uint8_t columns = 4;
	
	uint16_t address = size * (chr-starting_char);
	uint8_t width = 2*(forty_dot_width_table[chr-starting_char]-2);
	int32_t curX;
	
	uint8_t i,j;
	for(i=0;i<(size/columns);i++)
	{
		curX = x;
		for(j=0;j<columns;j++)
		{
      uint16_t twobytes = stretchOneToTwo(forty_dot_glyph_table[address]);
			lcdHorizontalByte(twobytes>>8,curX,y);
			lcdHorizontalByte(twobytes>>8,curX,y+1);
			curX +=8;
			lcdHorizontalByte(twobytes,curX,y);
			lcdHorizontalByte(twobytes,curX,y+1);
			curX +=8;
			address++;
		}
		y+=2;
	}
	return(x+width);
}

static int32_t lcdChrLib80Width(uint8_t chr)
{
	const uint8_t starting_char = '-'; //first glyph char
	return 2*(forty_dot_width_table[chr-starting_char]-2);
}

static int32_t lcdChrLib40(uint8_t chr, int32_t x, int32_t y)
{
	const uint8_t size = 4*40;	//size in bytes of one glyph
	const uint8_t starting_char = '-'; //first glyph char
	const uint8_t columns = 4;
	
	uint16_t address = size * (chr-starting_char);
	uint8_t width = forty_dot_width_table[chr-starting_char]-2;
	int32_t curX;
	
	uint8_t i,j;
	for(i=0;i<(size/columns);i++)
	{
		curX = x;
		for(j=0;j<columns;j++)
		{
			lcdHorizontalByte(forty_dot_glyph_table[address],curX,y);
			address++;
			curX +=8;
		}
		y++;
	}
	return(x+width);
}

static int32_t lcdChrLib40Width(uint8_t chr)
{
	const uint8_t starting_char = '-'; //first glyph char
	return (forty_dot_width_table[chr-starting_char]-2);
}



static int32_t lcdChrLib18(uint8_t chr, int32_t x, int32_t y)
{
	const uint8_t size = 3*18;	//size in bytes of one glyph
	const uint8_t starting_char = ' '; //first glyph char
	const uint8_t columns = 3;
	
	uint16_t address = size * (chr-starting_char);
	uint8_t width = eighteen_dot_width_table[chr-starting_char];
	int32_t curX;
	
	uint8_t i,j;
	for(i=0;i<(size/columns);i++)
	{
		curX = x;
		for(j=0;j<columns;j++)
		{
			lcdHorizontalByte(eighteen_dot_glyph_table[address],curX,y);
			address++;
			curX +=8;
		}
		y++;
	}
	return(x+width);
}


static int32_t lcdChrLib18Width(uint8_t chr)
{
	const uint8_t starting_char = ' '; //first glyph char
	return (eighteen_dot_width_table[chr-starting_char]);
}



static int32_t lcdChrLib15(uint8_t chr, int32_t x, int32_t y)
{
	const uint8_t size = 3*18;	//size in bytes of one glyph
	const uint8_t starting_char = ' '; //first glyph char
	const uint8_t columns = 3;
	
	uint16_t address = size * (chr-starting_char);
	uint8_t width = fifteen_dot_width_table[chr-starting_char];
	int32_t curX;
	
	uint8_t i,j;
	for(i=0;i<(size/columns);i++)
	{
		curX = x;
		for(j=0;j<columns;j++)
		{
			lcdHorizontalByte(fifteen_dot_glyph_table[address],curX,y);
			address++;
			curX +=8;
		}
		y++;
	}
	return(x+width);
}

static int32_t lcdChrLib15Width(uint8_t chr)
{
	const uint8_t starting_char = ' '; //first glyph char
	return(fifteen_dot_width_table[chr-starting_char]);
}



static int32_t lcdChrLib10Width(uint8_t chr)
{
	return 9;
}

static int32_t lcdChrLib10(uint8_t chr, int32_t x, int32_t y)
{
	const uint8_t size = 24;	//size in bytes of one glyph
	const uint8_t starting_char = ' '; //first glyph char
	const uint8_t columns = 2;
	
	uint16_t address = size * (chr-starting_char);
	//uint8_t width = nine_dot_width_table[chr-starting_char];
	uint8_t width = 9;
	int32_t curX;
	
	uint8_t i,j;
	for(i=0;i<(size/columns);i++)
	{
		curX = x;
		for(j=0;j<columns;j++)
		{
			lcdHorizontalByte(ten_dot_glyph_table[address],curX,y);
			address++;
			curX +=8;
		}
		y++;
	}
	return(x+width);
}





static int32_t lcdChrLib9Width(uint8_t chr)
{
	return 8;
}


static int32_t lcdChrLib9(uint8_t chr, int32_t x, int32_t y)
{
	const uint8_t size = 12;	//size in bytes of one glyph
	const uint8_t starting_char = ' '; //first glyph char
	const uint8_t columns = 1;
	
	uint16_t address = size * (chr-starting_char);
	//uint8_t width = nine_dot_width_table[chr-starting_char];
	uint8_t width = 8;
	int32_t curX;
	
	uint8_t i,j;
	for(i=0;i<(size/columns);i++)
	{
		curX = x;
		for(j=0;j<columns;j++)
		{
			lcdHorizontalByte(nine_dot_glyph_table[address],curX,y);
			address++;
			curX +=8;
		}
		y++;
	}
	return(x+width);
}


static int32_t lcdChrLib7Width(uint8_t chr)
{
	const uint8_t starting_char = ' '; //first glyph char
	return (seven_dot_width_table[chr-starting_char]);
}

static int32_t lcdChrLib7(uint8_t chr, int32_t x, int32_t y)
{
	const uint8_t size = 16;	//size in bytes of one glyph
	const uint8_t starting_char = ' '; //first glyph char
	const uint8_t columns = 2;
	
	uint16_t address = size * (chr-starting_char);
	uint8_t width = seven_dot_width_table[chr-starting_char];
	int32_t curX;
	
	uint8_t i,j;
	for(i=0;i<(size/columns);i++)
	{
		curX = x;
		for(j=0;j<columns;j++)
		{
			lcdHorizontalByte(seven_dot_glyph_table[address],curX,y);
			address++;
			curX +=8;
		}
		y++;
	}
	return(x+width);
}

static int32_t lcdChrLib6Width(uint8_t chr)
{
	const uint8_t starting_char = ' '; //first glyph char
	return (six_dot_width_table[chr-starting_char]);
}


static int32_t lcdChrLib6(uint8_t chr, int32_t x, int32_t y)
{
	const uint8_t size = 16;	//size in bytes of one glyph
	const uint8_t starting_char = ' '; //first glyph char
	const uint8_t columns = 2;
	
	uint16_t address = size * (chr-starting_char);
	uint8_t width = six_dot_width_table[chr-starting_char];
	int32_t curX;
	
	uint8_t i,j;
	for(i=0;i<(size/columns);i++)
	{
		curX = x;
		for(j=0;j<columns;j++)
		{
			lcdHorizontalByte(six_dot_glyph_table[address],curX,y);
			address++;
			curX +=8;
		}
		y++;
	}
	return(x+width);
}



static int32_t lcdChrLib5Width(uint8_t chr)
{
	const uint8_t starting_char = ' '; //first glyph char
	return (five_dot_width_table[chr-starting_char]);
}

//ok
static int32_t lcdChrLib5(uint8_t chr, int32_t x, int32_t y)
{
	const uint8_t size = 7;	//size in bytes of one glyph
	const uint8_t starting_char = ' '; //first glyph char
	
	uint16_t address = size * (chr-starting_char);
	uint8_t width = five_dot_width_table[chr-starting_char];
	
	uint8_t i;
	for(i=0;i<size;i++)
	{
		lcdHorizontalByte(five_dot_glyph_table[address],x,y);
		if(i!=0) y++;
		address++;
	}
	return(x+width);
}

