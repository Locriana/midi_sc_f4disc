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


#include "sc_if.h"
#include "main.h"
#include <string.h>
#include <stdio.h>
#include "dbgu.h"
#include "sc_proc.h"

#define SC_VALUES_NB		10
#define SC_VALUES_NAME_LEN	10

#define SC_VALUE_LIST_ITEM_NAME_LEN	10


#define PRINT_DBG_ON		  0
#define PRINT_STATUS_ON		0

#if (PRINT_DBG_ON != 0)
	#define  PRINT_DBG(...) {xprintf("DBG: "); xprintf(__VA_ARGS__); xprintf(" ");}
#else
	#define PRINT_DBG(...) do {} while (0)
#endif

#if (PRINT_STATUS_ON != 0)
	#define  PRINT_STATUS(...) {xprintf("#SC: "); xprintf(__VA_ARGS__); xprintf("\n");}
#else
	#define PRINT_STATUS(...) do {} while (0)
#endif



#define ITEM_NAME_LEN_MAX		30
const char value_name[SC_IDX_NB][ITEM_NAME_LEN_MAX] = {
		"    Preset Id    ","     Active     ","   Step Delay   ","   Depth   ","   Curve   ","   Source Channel   ","    Source Note    ",
		" Dest. Channel A ", " Dest. Channel B ", " Dest. Channel C ", " Dest. Channel D ",
		"   Dest. CC A   ", "   Dest. CC B   ", "  Dest. CC C  ", "  Dest. CC D  ",
		"   Reload / Save  "
};


const char value_list_binary[2][4] = {"Off","On"};
const char value_list_channels[17][5] = {
	"Off","Ch1","Ch2","Ch3","Ch4","Ch5","Ch6","Ch7","Ch8",
	"Ch9","Ch10","Ch11","Ch12","Ch13","Ch14","Ch15","Ch16"
};

static int preset_changed = 0;

static sc_preset_t presets[SC_PRESET_NB];
static uint8_t current_preset_idx = 0;
static sc_idx_t current_value_idx = SC_FIRST_EDITABLE_VALUE_IDX;
//                                       id, ac,dl,dp,cr,ch,nte,da,db,dc,dd, cca,  ccb,  ccc,               ccd,   dt
static const sc_preset_t preset_v_min = { 0, 0, 1, 0,            0, 1,  0,{ 0, 0, 0, 0}, {0x01, 0x01, 0x01, 0x01}, 0};
static const sc_preset_t preset_v_max = { 0, 1,63, SC_MAX_DEPTH, 3,16,127,{16,16,16,16}, {0x77, 0x77, 0x77, 0x77}, 1};
static const sc_preset_t preset_v_def = { 0, 1, 5, 12,           0, 1, 36,{ 2, 0, 0, 0}, {0x07, 0x07, 0x07, 0x07}, 0};

static int value_valid(uint8_t pidx, sc_idx_t vidx);
//sc_value_t set_value(uint8_t pidx, sc_idx_t vidx, sc_value_t v);
static int check_all_presets(void)__attribute__((unused));
static int check_preset(uint8_t pidx);
static int any_dirty_flag(uint8_t pidx)__attribute__((unused));
static void clear_dirty_flag(uint8_t pidx);
static void set_dirty_flag(uint8_t pidx);



void sc_init(void){
	xprintf("sc_init\n");
	PRINT_STATUS("F=INIT");
	current_preset_idx = 0;
	sc_load_presets();
	clear_dirty_flag(SC_PRESET_ALL);
	for(uint8_t pidx = 0; pidx < SC_PRESET_NB; pidx++){
		if(!check_preset(pidx)){
			sc_preset_fill_default(pidx);
			set_dirty_flag(pidx);
		}
	}
	preset_changed = 1;


/*	if(any_dirty_flag(SC_PRESET_ALL)){
		sc_save_presets();
	}*/
}

const char* sc_get_channel_name(uint8_t channel){
  return &value_list_channels[channel][0];
}

//just pass the information to the core module
void sc_proc_info_messages(uint8_t on){
  sc_proc_core_info_messages(on);
}

void sc_load_presets(void){
	PRINT_STATUS("F=LOAD BSY=1");
	sc_cb_presets_load_from_nv(&presets[0],sizeof(presets));
	PRINT_STATUS("F=LOAD BSY=0");
	preset_changed = 1;
}

static int preset_idx_valid(uint8_t idx){
	if(idx < SC_PRESET_NB) return 1;
	if(idx == SC_PRESET_ALL) return 1;
	return 0;
}


/*
 * args:
 * pidx - preset idx
 * vidx - value idx
 */
static int value_valid(uint8_t pidx, sc_idx_t vidx){
	sc_value_t* preset_buf = (sc_value_t*)&presets[pidx];
	sc_value_t* max_buf = (sc_value_t*)&preset_v_max;
	sc_value_t* min_buf = (sc_value_t*)&preset_v_min;
	sc_value_t v_max = max_buf[vidx];
	sc_value_t v_min = min_buf[vidx];
	sc_value_t v = preset_buf[vidx];

	if(vidx == SC_IDX_PIDX){
		if( v == pidx ){
			return 1;
		}
		else{
			PRINT_DBG(" * invalid pidx for preset %d\n",pidx);
			return 0;
		}
	}

	if( (v <= v_max) && (v >= v_min) ){
		return 1;
	}
	else{
		xprintf("* sc, value_valid: invalid value 0x%02X found @ pidx=%d, vidx=%d\n",v,pidx,vidx);
		return 0;
	}

}

sc_preset_t* sc_get_current_preset(void){
	return &presets[current_preset_idx];
}

//preset needs to be re-processed
int sc_preset_changed(void){
	int res = preset_changed;
	preset_changed = 0;
	return res;
}

static void sc_status_preset(uint8_t pidx){
	PRINT_STATUS("F=SPRES PIDX=%d",pidx);
	for(uint8_t vidx = 0; vidx < SC_IDX_NB; vidx++){
		PRINT_STATUS("F=SPRES VIDX=%d VNAME=%s VAL=%d",vidx,value_name[vidx],sc_get_value(pidx, vidx));
	}
}

void sc_status_current(void){
	PRINT_STATUS("F=STATCURR CPIDX=%d CVIDX=%d",current_preset_idx,current_value_idx);
	sc_status(current_preset_idx);
}

void sc_status(uint8_t pidx){
	PRINT_STATUS("F=STAT CPIDX=%d CVIDX=%d",current_preset_idx,current_value_idx);
	if(pidx == SC_PRESET_ALL){
		for(uint8_t pidx = 0; pidx<SC_PRESET_NB; pidx++)
			sc_status_preset(pidx);
	}
	else{
		sc_status_preset(pidx);
	}
}

void sc_disp(void){
	xprintf("Current preset, SC parameters:\n");

}

void sc_disp_preset(uint8_t pidx){
	xprintf("SC parameters of preset %d:\n",pidx);
	xprintf(" * pidx = %d\n",presets[pidx].pidx);
	xprintf(" * actv = %d\n",presets[pidx].active);
	xprintf(" * stpd = %d\n",presets[pidx].step_delay);
	xprintf(" * curv = %d\n",presets[pidx].curve);
	xprintf(" * dpth = %d\n",presets[pidx].depth);
	xprintf(" * srch = %d\n",presets[pidx].src_ch);
	xprintf(" * srnt = %d\n",presets[pidx].src_note);
	for(int i=0;i<4;i++)
		xprintf(" * dch%d = %d\n",i,presets[pidx].dst_ch[i]);
	for(int i=0;i<4;i++)
		xprintf(" * dCC%d = %d\n",i,presets[pidx].dst_cc[i]);
	xprintf(" * drty = %d\n",presets[pidx].dirty_flag);
}

void sc_dump(void){
	xprintf("SC debug information\n");
	xprintf("current_preset_idx = %d\nPreset mem:\n", current_preset_idx);
	debug_dump((void*)presets,sizeof(presets));
}

void sc_print_info_str(void){
	sc_value_t * preset_values = (sc_value_t*)&presets[current_preset_idx];
	sc_value_t value = preset_values[current_value_idx];
	char *dirty = " ";
	if(any_dirty_flag(current_preset_idx)) dirty = "*";
	xprintf("Preset %d%s item%02d: %s setting=%02d\n",current_preset_idx,dirty,current_value_idx, value_name[current_value_idx],value);
}

const char* sc_get_value_name(sc_idx_t vidx){
	return value_name[vidx];
}

sc_idx_t sc_get_current_vidx(void){
	return current_value_idx;
}

const char* sc_get_current_value_name(void){
	return value_name[current_value_idx];
}

static int check_preset(uint8_t pidx){
	PRINT_DBG("check_preset %d...\n",pidx);
	int res = 1;
	for(uint8_t vidx=0;vidx<SC_IDX_NB; vidx++){
		if( !value_valid(pidx, vidx) ){
			res = 0;
			//return res;
		}
	}
	PRINT_DBG("check_presets ends with the code %d\n",res);
	return res;
}


//returns 1 if every parameter of every preset is okay
//otherwise floods the terminal with annoying messages
static int check_all_presets(void){
	PRINT_DBG("check_presets...\n");
	int res = 1;
	for(uint8_t pidx = 0; pidx < SC_PRESET_NB; pidx++){
		for(uint8_t vidx=0;vidx<SC_IDX_NB; vidx++){
			if( check_preset(pidx) == 0){
				res = 0;
			}
		}
	}
	PRINT_DBG("check_presets ends with the code %d\n",res);
	return res;
}

sc_preset_t* sc_select_preset(uint8_t pidx){
  if(pidx < SC_PRESET_NB){
    current_preset_idx = pidx;
  }
  sc_load_presets();
  clear_dirty_flag(SC_PRESET_ALL);
  PRINT_STATUS("F=SELPRES CPIDX=%d CVIDX=%d",current_preset_idx,current_value_idx);
  PRINT_DBG("sc_select_preset ends, curr pidx=%d, ret addr = %08X\n",current_preset_idx,(unsigned int)&presets[current_preset_idx]);
  preset_changed = 1;
  return &presets[current_preset_idx];
}

/*
 * changes a selected preset ++ or -- or just reloads (if change=0)
 * performs a roll-over on boundary values
 * returns a pointer to the selected preset
 */
sc_preset_t* sc_change_preset(int change){
	PRINT_DBG("sc_change_preset, ch=%d curr pidx=%d\n",change,current_preset_idx);
	if(change > 0){
		if(current_preset_idx < (SC_PRESET_NB-1)) current_preset_idx++; else current_preset_idx = 0;
	}
	else if(change < 0){
		if(current_preset_idx > 0) current_preset_idx--; else current_preset_idx = SC_PRESET_NB - 1;
	}
	return sc_select_preset(current_preset_idx);
}

uint8_t sc_select_value(int change){
	PRINT_DBG("sc_select_value, ch=%d curr pidx=%d\n",change,current_value_idx);
	if(change > 0){
		if(current_value_idx < SC_LAST_EDITABLE_VALUE_IDX) current_value_idx++; else current_value_idx = SC_FIRST_EDITABLE_VALUE_IDX;
	}
	else if(change < 0){
		if(current_value_idx > SC_FIRST_EDITABLE_VALUE_IDX) current_value_idx--; else current_value_idx = SC_LAST_EDITABLE_VALUE_IDX;
	}
	PRINT_STATUS("F=SELVAL CPIDX=%d CVIDX=%d",current_preset_idx,current_value_idx);
	PRINT_DBG("sc_select_value ends, curr pidx=%d, ret addr = %08X\n",current_value_idx,(unsigned int)&presets[current_value_idx]);
	return current_value_idx;
}


int sc_save_presets(void){
	PRINT_STATUS("F=SAVE BSY=1");
	int res = sc_cb_presets_store_to_nv(&presets[0], sizeof(presets));
	for(int i=0; i< SC_PRESET_NB; i++){
		clear_dirty_flag(i);
	}
	xprintf("sc_save_presets done\n");
	PRINT_STATUS("F=SAVE BSY=0");
	return res;
}

sc_value_t sc_get_value(uint8_t pidx, sc_idx_t vidx){
	sc_value_t* preset_buf = (sc_value_t*)&presets[pidx];
	return preset_buf[vidx];
}

sc_value_t sc_get_current_value(void){
	return sc_get_value(current_preset_idx,current_value_idx);
}


uint8_t sc_get_current_preset_idx(void){
	return current_preset_idx;
}


sc_value_t sc_change_current_value(int mod){
	PRINT_STATUS("F=CHGCVAL VIDX=%d",current_value_idx);
	return sc_change_value(current_value_idx,mod);
}

/*
 * safely changes a selected value in the active preset
 * no rollover - if reaching the min/max, just stays there
 * for the "dirty" value it calls save (mod=+1) on reload (mod=-1)
 * returns the new value
 */
sc_value_t sc_change_value(sc_idx_t vidx, int mod){
	PRINT_DBG("sc_change_value: vidx=%d, mod=%d\n",(int)vidx,mod);

	if(vidx == SC_IDX_DIRTY_SAVE){
		if(mod > 0){
			sc_save_presets();
		}
		else if(mod < 0){
			sc_load_presets();
		}
		return 0;
	}
	else{
		sc_value_t* preset_buf = (sc_value_t*)&presets[current_preset_idx];
		sc_value_t v = preset_buf[vidx];
		sc_value_t* max_buf = (sc_value_t*)&preset_v_max;
		sc_value_t* min_buf = (sc_value_t*)&preset_v_min;
		sc_value_t v_max = max_buf[vidx];
		sc_value_t v_min = min_buf[vidx];

		v = v + mod;

		if( (v <= v_max) && (v >= v_min) ){
			preset_buf[vidx] = v;
		}
		else if (v > v_max){
			preset_buf[vidx] = v_max;
		}
		else{
			preset_buf[vidx] = v_min;
		}
		set_dirty_flag(current_preset_idx);
		PRINT_STATUS("F=CHGVAL PIDX=%d VIDX=%d VNAME=%s VAL=%d",current_preset_idx,vidx,value_name[vidx],preset_buf[vidx]);
		preset_changed = 1;
		return preset_buf[vidx];
	}
}


sc_value_t sc_set_current_value(sc_idx_t vidx, uint8_t v){
//  PRINT_DBG("sc_set_current_value: vidx=%d, mod=%d\n",(int)vidx,mod);
  return sc_set_value(current_preset_idx,vidx,v);
}

sc_value_t sc_set_value(uint8_t pidx, sc_idx_t vidx, uint8_t v){
  PRINT_DBG("sc_set_value: vidx=%d, v=%d\n",(int)vidx,v);

  sc_value_t* preset_buf = (sc_value_t*)&presets[pidx];
  sc_value_t* max_buf = (sc_value_t*)&preset_v_max;
  sc_value_t* min_buf = (sc_value_t*)&preset_v_min;
  sc_value_t v_max = max_buf[vidx];
  sc_value_t v_min = min_buf[vidx];

  if( (v <= v_max) && (v >= v_min) ){
    preset_buf[vidx] = v;
  }
  else if (v > v_max){
    preset_buf[vidx] = v_max;
  }
  else{
    preset_buf[vidx] = v_min;
  }
  set_dirty_flag(current_preset_idx);
  PRINT_STATUS("F=SETVAL PIDX=%d VIDX=%d VNAME=%s VAL=%d",current_preset_idx,vidx,value_name[vidx],preset_buf[vidx]);
  preset_changed = 1;
  return preset_buf[vidx];
}




void sc_preset_fill_default(uint8_t pidx){
	if(!preset_idx_valid(pidx)){xprintf("sc_preset_load_default: pidx out of range\n"); return;}
	PRINT_STATUS("F=FILLDEF");

	if(pidx==SC_PRESET_ALL){
		for(uint8_t i=0;i<SC_PRESET_NB;i++){
			memcpy(&presets[i],&preset_v_def,sizeof(sc_preset_t));
			presets[i].pidx = i;
			set_dirty_flag(i);
		}
	}
	else{
		memcpy(&presets[pidx],&preset_v_def,sizeof(sc_preset_t));
		presets[pidx].pidx = pidx;
		set_dirty_flag(pidx);
	}
	preset_changed = 1;
}

static void set_dirty_flag(uint8_t pidx){
	presets[pidx].dirty_flag = 1;
}

static void clear_dirty_flag(uint8_t pidx){
	PRINT_DBG("clearing dirty flag pidx=0x%02X\n",pidx);
	if(pidx==SC_PRESET_ALL){
		for(uint8_t i=0;i<SC_PRESET_NB;i++){
			presets[i].dirty_flag = 0;
		}
	}
	else{
		presets[pidx].dirty_flag = 0;
	}
}

int sc_get_current_dirty_flag(void){
	return presets[current_preset_idx].dirty_flag;
}

static int any_dirty_flag(uint8_t pidx){
	if(pidx==SC_PRESET_ALL){
		for(uint8_t i=0;i<SC_PRESET_NB;i++){
			if(presets[i].dirty_flag) return 1;
		}
	}
	else{
		if(presets[pidx].dirty_flag) return 1;
	}
	return 0;
}



__weak int sc_cb_presets_load_from_nv(void* buffer, uint16_t len){
	xprintf("WEAK CALLBACK: sc_cb_presets_load_from_nv: buf @ 0x%02X, len=%d\n",(unsigned int)buffer,(int)len);
	return 0;
}

__weak int sc_cb_presets_store_to_nv(void* buffer, uint16_t len){
	xprintf("WEAK CALLBACK: sc_cb_presets_store_to_nv: buf @ 0x%02X, len=%d\n",(unsigned int)buffer,(int)len);
	return 0;
}


//void sc_mod_param(uin8_t idx, )

/*
sc_value_t* sc_select_next(void){

	return NULL;
}

sc_value_t* sc_select_prev(void){

	return NULL;
}

int sc_mod(int mod){

	return 0;
}*/

void sc_process(void){
	sc_proc_core();
}

/*__weak void sc_cb_save(){

}

__weak void sc_cb_recall(sc_value_t* opt, uint16_t len_b){

}*/

