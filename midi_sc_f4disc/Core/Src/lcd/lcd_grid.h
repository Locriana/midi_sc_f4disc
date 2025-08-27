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

#ifndef __LCD_GRID_H__
#define __LCD_GRID_H__


int lcdgrid_Set(uint32_t pTilesX, uint32_t pTilesY);
void lcdgrid_Draw(void);
void lcdgrid_InvertCell(uint16_t x, uint16_t y);
void lcdgrid_ColorCell(uint16_t x, uint16_t y, uint32_t color);
int32_t lcdgrid_CellX(uint16_t cellX);
int32_t lcdgrid_CellY(uint16_t cellY);
int32_t lcdgrid_CellCenterX(uint16_t cellX);
int32_t lcdgrid_CellCenterY(uint16_t cellY);
void lcdgrid_TextInCell(uint16_t cellX, uint16_t cellY, const char* text);
int32_t lcdgrid_GetTileSizeX(void);
int32_t lcdgrid_GetTileSizeY(void);
int lcdgrid_DetectTap(uint16_t cellX, uint16_t cellY, uint16_t tapX, uint16_t tapY);


#endif

