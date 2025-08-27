/*
 * curves.h
 *
 *  Created on: Aug 10, 2025
 *      Author: ada
 */


#ifndef INC_SC_CURVES_H_
#define INC_SC_CURVES_H_

#include <sc_if.h>
#include <inttypes.h>

#define SC_CURVE_LEN    	16
#define SC_CURVE_NAME_LEN	10

#define SC_CURVE_NB     4

uint8_t sc_curve_get_item(uint8_t curveidx,uint8_t itemidx);
const char* sc_curve_get_name(uint8_t idx);

#endif /* INC_SC_CURVES_H_ */
