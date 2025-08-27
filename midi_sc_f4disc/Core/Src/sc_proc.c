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
#include "sc_curves.h"


#define PRINT_DBG_ON		0

#if (PRINT_DBG_ON != 0)
	#define  PRINT_DBG(...) {xprintf("DBG: "); xprintf(__VA_ARGS__); xprintf(" ");}
#else
	#define PRINT_DBG(...) do {} while (0)
#endif

static uint8_t print_info = 0;

static uint8_t cc_buf[4] = {7,7,7,7};
static uint8_t ch_buf[4] = {2,3,4,5};
static sc_preset_t *preset;
static uint8_t current_curve[SC_CURVE_LEN];

static int state = 0xFF;
static volatile int step_delay_cntr=0;

static void mod_curve(){
	int mod = 0;
	if(preset->depth > 0){
	  mod = (SC_MAX_DEPTH + 1 - preset->depth) * 8;
	}
	else{
	  mod = 127;
	}
	//PRINT_DBG("sc core proc mod=%02X",mod);
	for(int i=0;i<SC_CURVE_LEN;i++){

		int temp = sc_curve_get_item(preset->curve,i);
		temp = temp + mod;
		if(temp > 127) temp = 127;
		current_curve[i] = (uint8_t)temp;
	}
}

static void update_settings(void){
	PRINT_DBG("sc core proc: update_settings\n");
	preset = sc_get_current_preset();
	//set output channels and CCs
	for(int i=0;i<SC_OUT_CH_NB;i++){
		cc_buf[i] = preset->dst_cc[i];
		ch_buf[i] = preset->dst_ch[i];
	}
	//get and prepare curve
	mod_curve();
}

void sc_proc_core_info_messages(uint8_t on){
  if(on){
    print_info = 1;
    xprintf("sc core info messages ON\n");
  }
  else{
    print_info = 0;
    xprintf("sc core info messages OFF\n");
  }
}


void sc_proc_core(void){
	if(sc_preset_changed()){
		update_settings();
	}
	if( step_delay_cntr > 0){
	  step_delay_cntr--;
	}
	else{
	  step_delay_cntr = preset->step_delay;
	  uint8_t value = 127;
	  if(state < SC_CURVE_LEN){
	    if(preset->active){
	      value = current_curve[state];
	    }
      sc_cc_callback(ch_buf, cc_buf, value);
      if(print_info) xprintf("v=%d ",value);
	    if(value == 127){
	      state=SC_CURVE_LEN;
	    }
	    else{
	      state++;
	    }
	  }//if(state < SC_CURVE_LEN){
	  else if(state == SC_CURVE_LEN){ //after all the curve point are sent, set volume to max
      value = 127;
      SC_PROC_LED_OFF;
      step_delay_cntr = 0;
      sc_cc_callback(ch_buf, cc_buf, value);
      if(print_info) xprintf("v=%d ",value);
      if(print_info) xprintf("sc done\n");
      state++;
	  }
	}//else to if( step_delay_cntr > 0)
}

void sc_input_note_on(uint8_t ch, uint8_t note, uint8_t velocity){
	if( (ch==preset->src_ch) && (note==preset->src_note) ){
		state = 0;
		//PRINT_DBG("sc_input_note_on: TRIG! ch=%d, note=%d, v=%d\n",ch,note,velocity);
		if(preset->active){
		  sc_cc_callback(ch_buf,cc_buf,current_curve[state]);
		}
		else{
		  sc_cc_callback(ch_buf,cc_buf,127);
		}
		step_delay_cntr = preset->step_delay;
    SC_PROC_LED_ON;
		if(print_info)xprintf("*sc: ");
	}
	else{
		//PRINT_DBG("sc_input_note_on: ignored: ch=%d, note=%d, v=%d\n",ch,note,velocity);
	}
}

__weak void sc_cc_callback(uint8_t* chbuf, uint8_t *ccbuf, uint8_t value){
  if(print_info) xprintf("*v=%d\n",value);
	/*xprintf("channels:\n");
	debug_hexbuf(chbuf, 4);
	xprintf("CCs:\n");
	debug_hexbuf(ccbuf, 4);*/
}
