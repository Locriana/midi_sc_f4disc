/*
* The MIT License (MIT)
* Copyright (c) 2025 Ada Brzoza-Zajecka (Locriana)
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
#include "dbgu.h"
#include "nvstore.h"
#include <string.h>

#define WORKING_SECTOR		(FLASH_SECTOR_TOTAL-1)
#define WORKING_SECTOR_SIZE (128*1024)

static uint32_t working_addr_start = 0;
static uint32_t working_area_size = 0;

uint32_t nv_init(void){
	uint32_t sector_size = WORKING_SECTOR_SIZE;
	working_addr_start = FLASH_END + 1 - sector_size;
	working_area_size = sector_size;
	uint32_t flash_error_codes = HAL_FLASH_GetError();
	xprintf("nv_init: working sector: %d, st address=0x%08X size: %d KB,   \nInitial error codes: %08X\n",
			WORKING_SECTOR,(unsigned int)working_addr_start,(unsigned int)sector_size/1024,(unsigned int)flash_error_codes);
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);	//https://community.st.com/t5/stm32-mcus-products/stm32f413-flash-erase-doesn-t-work-first-time/td-p/224404
	return working_addr_start;
}

int nv_erase(void){
  HAL_FLASH_Unlock();
  xprintf("nvstore: nv_erase...\n");
  FLASH_EraseInitTypeDef EraseInitStruct;
  EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
  EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
  EraseInitStruct.Sector        = WORKING_SECTOR;
  EraseInitStruct.NbSectors     = 1;
  uint32_t SECTORError = 0;
  if(HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK){
	  xprintf("nvstore: nv_erase error: %08X\n",(unsigned int)SECTORError);
	  uint32_t error_code = HAL_FLASH_GetError();
	  //HAL_FLASH_ERROR_RD
	  xprintf("error_code = %08X\n",(unsigned int)error_code);
  }
  HAL_FLASH_Lock();
  xprintf("nvstore: nv_erase ends\n");
  return 0;
}

int nv_write(void* buf, uint16_t len){
  xprintf("nvstore: nv_write...\n");
	nv_erase();
	HAL_FLASH_Unlock();
	uint32_t *ptr = buf;
	uint32_t address_wr = working_addr_start;
	for(int i=0; i<len; i++){

		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address_wr, *ptr++) != HAL_OK ){
			xprintf(" - HAL_FLASH_Program error @ address %08X\n",(unsigned int)address_wr);
		}
		address_wr+=4;
	}
    HAL_FLASH_Lock();
    xprintf("nvstore: nv_write ends\n");
	return 0;
}


int nv_read(void*buf, uint16_t len){
	//xprintf("nv_read: from %08X to %08X, len %d bytes\n",(unsigned int)working_addr_start,(unsigned int)buf,(int)len);
	memcpy(buf,(void*)working_addr_start,len);
	return 0;
}


