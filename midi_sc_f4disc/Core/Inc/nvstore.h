/*
 * nvstore.h
 *
 *  Created on: Aug 12, 2025
 *      Author: ada
 */

#ifndef INC_NVSTORE_H_
#define INC_NVSTORE_H_

uint32_t nv_init(void);
int nv_erase(void);
int nv_write(void* buf, uint16_t len);
int nv_read(void*buf, uint16_t len);



#endif /* INC_NVSTORE_H_ */
