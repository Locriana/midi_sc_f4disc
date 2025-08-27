/*
* The MIT License (MIT)
* Copyright (c) 2016-2025 Ada Brzoza-Zajecka (Locriana)
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


#ifndef INC_USBMIDI_IFC_H_
#define INC_USBMIDI_IFC_H_

#include "FreeRTOS.h"
#include "midi_defs.h"


typedef struct __attribute__((packed))
{
	uint8_t cn_cin;
	uint8_t midi[3];
} T_usbmidi_EVENT_PACKET;

typedef enum {
	CIN_SYS_COMM_2B   = 0x02,
	CIN_SYS_COMM_3B   = 0x03,
	CIN_SYSEX_ST_CNT  = 0x04,
	CIN_SYSEX_END_COMM_1B = 0x05,
	CIN_SYSEX_END_2B  = 0x06,
	CIN_SYSEX_END_3B  = 0x07,
	CIN_NOTE_OFF      = 0x08,
	CIN_NOTE_ON       = 0x09,
	CIN_POLY_KEYPRESS = 0x0A,
	CIN_CC            = 0x0B,
	CIN_PC            = 0x0C,
	CIN_CHAN_PRESSURE = 0x0D,
	CIN_PITCH_BEND    = 0x0E,
	CIN_SINGLE_BYTE   = 0x0F
} T_usbmidi_cin;

#define usbmidi_PACKET_LENGTH		4

/*
 * max supported length of the whole sysex message
 * defined for static buffer allocation
 */
#define usbmidi_SYSEX_MAX_LEN		128

int usbmidi_init(void);
void usbmidi_start(void);
void usbmidi_set_cable(uint8_t p_cable);

//callbacks
void usbmidi_cb_note_on(uint8_t ch, uint8_t note, uint8_t velocity);
void usbmidi_cb_note_off(uint8_t ch, uint8_t note, uint8_t velocity);
void usbmidi_cb_syscomm(uint8_t status, uint8_t data1, uint8_t data2);	//raw format, channel is encoded in status
void usbmidi_cb_pc(uint8_t ch, uint8_t program);						//program change
void usbmidi_cb_cc(uint8_t ch, uint8_t ctrl, uint8_t value);			//control change rxed
void usbmidi_cb_sysex(uint8_t* buf, uint16_t len);						//buf contains raw data, including the 0xF0 & 0xF7
void usbmidi_cb_pitchbend(uint8_t ch, uint8_t data1, uint8_t data2);
void usbmidi_cb_aftertouch(uint8_t ch, uint8_t data1, uint8_t data2);
void usbmidi_cb_byte(uint8_t b);

//transmit

int usbmidi_tx_event(T_usbmidi_EVENT_PACKET* packet);
int usbmidi_tx_message(uint8_t status, uint8_t data1, uint8_t data2);
int usbmidi_inject_to_midi_in(T_usbmidi_EVENT_PACKET* packet, uint16_t len);

void usbmidi_tx_note_on(uint8_t ch, uint8_t note, uint8_t velocity);
void usbmidi_tx_note_off(uint8_t ch, uint8_t note, uint8_t velocity);
//void usbmidi_timing_tx(uint8_t status, uint8_t data1, uint8_t data2);	//all those 0xFx messages except (but not SysEx)
void usbmidi_tx_pc(uint8_t ch, uint8_t program);						//program change
void usbmidi_tx_cc(uint8_t ch, uint8_t ctrl, uint8_t value);			//control change rxed
void usbmidi_tx_sysex(uint8_t* buf, uint16_t len);						//buf contains raw data and includes 0xF0 & 0xF7





#endif /* INC_USBMIDI_IFC_H_ */
