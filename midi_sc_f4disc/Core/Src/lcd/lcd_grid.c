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

#include "main.h"
#include "cmsis_os.h"

#include  <errno.h>
#include  <sys/unistd.h>
#include  <string.h>

//#include "stm32746g_discovery_lcd.h"

#include "term_io.h"
#include "dbgu.h"
#include "ansi.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lcd.h"
#include "lcd_grid.h"

static uint32_t tilesX, tilesY;
static uint32_t tile_xSize, tile_ySize;

int lcdgrid_Set(uint32_t pTilesX, uint32_t pTilesY)
{
  tilesX = pTilesX;
  tilesY = pTilesY;
  tile_xSize = LCD_X_SIZE / tilesX;
  tile_ySize = LCD_Y_SIZE / tilesY;
  
  return 0;
}

void lcdgrid_Draw(void)
{
  for(int x=1; x<tilesX; x++)
  {
    lcdVerticalLine(tile_xSize*x,0,LCD_Y_SIZE-1);
  }
  
  for(int y=1; y<tilesY; y++)
  {
    lcdHorizontalLine(0,LCD_X_SIZE-1,y*tile_ySize);
  }
}


void lcdgrid_InvertCell(uint16_t x, uint16_t y)
{
  lcdInvertArea( tile_xSize*x, tile_xSize*(x+1)-1, tile_ySize*y, tile_ySize*(y+1)-1);
}

void lcdgrid_ColorCell(uint16_t x, uint16_t y, uint32_t color)
{
  uint32_t bg_copy = lcdGetBackgroundColor();
  lcdSetBackgroundColor(color);
  lcdClearArea( tile_xSize*x, tile_xSize*(x+1)-1, tile_ySize*y, tile_ySize*(y+1)-1);
  lcdSetBackgroundColor(bg_copy);
}

int32_t lcdgrid_CellCenterX(uint16_t cellX)
{
  return ( tile_xSize * cellX + tile_xSize / 2 );
}

int32_t lcdgrid_CellCenterY(uint16_t cellY)
{
  return ( tile_ySize * cellY + tile_ySize / 2 );
}

int32_t lcdgrid_CellX(uint16_t cellX)
{
  return ( tile_xSize * cellX);
}

int32_t lcdgrid_CellY(uint16_t cellY)
{
  return ( tile_ySize * cellY );
}


int32_t lcdgrid_GetTileSizeX(void)
{
  return(tile_xSize);
}

int32_t lcdgrid_GetTileSizeY(void)
{
  return(tile_ySize);
}

int lcdgrid_DetectTap(uint16_t cellX, uint16_t cellY, uint16_t tapX, uint16_t tapY){
	if( (tapX >= tile_xSize*cellX) && (tapX < tile_xSize*(cellX+1) ) && (tapY >= tile_ySize*cellY) && (tapY < tile_ySize*(cellY+1) ) ){
		return 1;
	}
	else{
		return 0;
	}
}


void lcdgrid_TextInCell(uint16_t cellX, uint16_t cellY, const char* text)
{
  char* ret = strchr(text,'\n');
  if(ret != NULL)
  {
    *ret = 0;
    int32_t x = lcdgrid_CellCenterX(cellX);
    int32_t y = lcdgrid_CellCenterY(cellY) - (lcdGetFont() / 2);
    const char* upperLine = text;
    const char* lowerLine = ret+1;
    //xprintf("Text in cell 2 line: upper=%s , lower=%s\n",upperLine,lowerLine);
    lcdSetTextCursor(x,y - 1.4*(lcdGetFont() / 2));
    lcdCentered(upperLine);
    lcdSetTextCursor(x,y + 1.4*(lcdGetFont() / 2));
    lcdCentered(lowerLine);
  }
  else
  {
    int32_t x = lcdgrid_CellCenterX(cellX);
    int32_t y = lcdgrid_CellCenterY(cellY) - (lcdGetFont() / 2);
    lcdSetTextCursor(x,y);
    lcdCentered(text);
    //xprintf("Text in cell 1 line: %s\n",text);
  }
}


