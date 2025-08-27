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

#ifndef INC_SC_IF_H_
#define INC_SC_IF_H_


#include <inttypes.h>

#define SC_PRESET_NB 16
#define SC_PRESET_ALL  0xFF

#define SC_MAX_DEPTH		15

#define SC_OUT_CH_NB		4


typedef enum {
	SC_IDX_PIDX = 0,
	SC_IDX_ACTIVE = 1,
	SC_IDX_STEP_DELAY = 2,
	SC_IDX_DEPTH = 3,
	SC_IDX_CURVE = 4,
	SC_IDX_SRC_CH = 5,
	SC_IDX_SRC_NOTE = 6,
	SC_IDX_DST_CH_A = 7,
	SC_IDX_DST_CH_B = 8,
	SC_IDX_DST_CH_C = 9,
	SC_IDX_DST_CH_D = 10,
	SC_IDX_DST_CC_A = 11,
	SC_IDX_DST_CC_B = 12,
	SC_IDX_DST_CC_C = 13,
	SC_IDX_DST_CC_D = 14,
	SC_IDX_DIRTY_SAVE = 15,
	SC_IDX_NB = 16
} sc_idx_t;

#define SC_FIRST_EDITABLE_VALUE_IDX   1 //do not allow editing the PIDX "preset idx" value
#define SC_LAST_EDITABLE_VALUE_IDX    (SC_IDX_NB-1) //the "dirty" value acts in a special way: inc=save, dec=reload

typedef int sc_value_t;

typedef struct __attribute__((packed)){
	sc_value_t pidx;
	sc_value_t active;
	sc_value_t step_delay;
	sc_value_t depth;
	sc_value_t curve;
	sc_value_t src_ch;
	sc_value_t src_note;
	sc_value_t dst_ch[SC_OUT_CH_NB];
	sc_value_t dst_cc[SC_OUT_CH_NB];
	sc_value_t dirty_flag;
}sc_preset_t;


void sc_init(void);

//core interface
sc_preset_t* sc_get_current_preset(void);
int sc_preset_changed(void);

//for midi interface
void sc_input_note_on(uint8_t ch, uint8_t note, uint8_t velocity);
void sc_cc_callback(uint8_t* chbuf, uint8_t *ccbuf, uint8_t value);

//value set/get
sc_idx_t sc_get_current_vidx(void);
uint8_t sc_select_value(int change);
sc_value_t sc_change_value(sc_idx_t vidx, int mod);
sc_value_t sc_change_current_value(int mod);
sc_value_t sc_set_value(uint8_t pidx, sc_idx_t vidx, uint8_t v);
sc_value_t sc_set_current_value(sc_idx_t vidx, uint8_t v);
sc_value_t sc_get_value(uint8_t pidx, sc_idx_t vidx);
sc_value_t sc_get_current_value(void);
const char* sc_get_value_name(sc_idx_t vidx);
const char* sc_get_current_value_name(void);

//preset select
sc_preset_t* sc_change_preset(int change);
sc_preset_t* sc_select_preset(uint8_t pidx);

//preset management
void sc_preset_fill_default(uint8_t idx);
int sc_save_presets(void);
void sc_load_presets(void);
sc_preset_t* sc_get_current_preset(void);
uint8_t sc_get_current_preset_idx(void);
int sc_get_current_dirty_flag(void);

//user control/gui/display
void sc_proc_info_messages(uint8_t on);
const char* sc_get_channel_name(uint8_t channel);
void sc_print_info_str(void);
void sc_disp(void);
void sc_disp_preset(uint8_t pidx);
void sc_dump(void);
void sc_status_current(void);
void sc_status(uint8_t pidx);

//nv-memory callbacks
int sc_cb_presets_load_from_nv(void* buffer, uint16_t len);
int sc_cb_presets_store_to_nv(void* buffer, uint16_t len);

//a wrapper for the process
void sc_process(void);

#endif /* INC_SC_IF_H_ */
