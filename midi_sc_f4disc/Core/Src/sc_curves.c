/*
 * sc_curves.c
 *
 *  Created on: Aug 13, 2025
 *      Author: ada
 */
#include "main.h"
#include "sc_curves.h"


const char sc_curve_names[SC_CURVE_NB][SC_CURVE_LEN] = {
    {"Linear"},
    {"C2"},
    {"C3"},
    {"C4"}
};

const uint8_t sc_curve[SC_CURVE_NB][SC_CURVE_LEN] =
{ {
0	,
8	,
16	,
24	,
32	,
40	,
48	,
56	,
64	,
72	,
80	,
88	,
96	,
104	,
112	,
120	},

{
0	,
12	,
30	,
60	,
90	,
105	,
115	,
120	,
123	,
125	,
126	,
127	,
127	,
127	,
127	,
127	},

{
0	,
1	,
3	,
10	,
18	,
30	,
50	,
70	,
90	,
105	,
118	,
123	,
125	,
127	,
127	,
127	},

{
0	,
24	,
60	,
85	,
105	,
115	,
122	,
125	,
127	,
127	,
127	,
127	,
127	,
127	,
127	,
127	} };

uint8_t sc_curve_get_item(uint8_t curveidx,uint8_t itemidx){
	return sc_curve[curveidx][itemidx];
}

const char* sc_curve_get_name(uint8_t idx){
	return &sc_curve_names[idx][0];
}

