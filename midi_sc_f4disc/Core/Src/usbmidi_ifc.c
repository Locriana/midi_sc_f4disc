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
#include "usbmidi_ifc.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "dbgu.h"
#include "usbh_MIDI.h"
#include "usb_host.h"
#include "usbh_conf.h"

#define MIDI_QUEUE_LEN		100
#define EVENT_PACKET_SIZE	sizeof(T_usbmidi_EVENT_PACKET)		//in bytes
#define TESTING				0
#define WEAK_CB_INFO		0

#if (TESTING != 0)
	#define  TSTPRINT(...) {xprintf("TST: "); xprintf(__VA_ARGS__); printf("\n");}
#else
	#define TSTPRINT(...) do {} while (0)
#endif

#if (WEAK_CB_INFO != 0)
	#define  WEAK_CB_PRINT(...) {xprintf("WEAK CALLBACK: "); xprintf(__VA_ARGS__); printf("\n");}
#else
	#define WEAK_CB_PRINT(...) do {} while (0)
#endif


const portTickType API_TX_TIMEOUT = 100;

static QueueHandle_t midi_out_queue = NULL;
static QueueHandle_t midi_in_queue = NULL;
static SemaphoreHandle_t tx_busy = NULL;
//static void usbmidi_core_task(void* params);	//higher-level / API processing task
static void rx_task(void *params); //a separate task to handle reception callbacks
static void tx_task(void *params); //a separate task to handle reception callbacks
static volatile uint8_t cable = 0;	//this is a bitmask, so don't write anything to its LSBs :P

#define RX_BUFF_SIZE 64 /* USB MIDI buffer : max received data 64 bytes */
uint8_t MIDI_RX_Buffer[RX_BUFF_SIZE]; // MIDI reception buffer

extern ApplicationTypeDef Appli_state;
extern USBH_HandleTypeDef hUsbHostHS;
USBH_HandleTypeDef* phost = &hUsbHostHS;

/*
 * examples:
 * 09 90 24 64 09 90 36 64  09 90 3F 64 0B B1 50 6E
 * 09 90 3F 64 0B B1 50 6E  0B B1 51 3E 09 91 24 7F
 * 0B B1 51 3E 09 91 24 7F
 * 0B B0 40 00
 * 08 80 24 40
 * 0B B0 50 43  //cc on ch1
 * 0B B1 52 4A  //cc on ch2
 *
 * this is a separate task so that user API callbacks can safely block
 * even for a longer time and the data will be queued without disrupting
 * the USB communication handled in the _hl_task
 */
static void rx_task(void *params){

	static uint8_t sysex_rx[usbmidi_SYSEX_MAX_LEN];
	static int sysex_rx_idx;
	static T_usbmidi_EVENT_PACKET packet;
	TickType_t TIMEOUT = 100;

	while(1){
		//xprintf(".");
		if( xQueueReceive(midi_in_queue, &packet, TIMEOUT) != pdPASS) continue;
		#if TESTING
			xprintf("rxed some data: ");
			debug_hexbuf(&packet,sizeof(packet));
			xprintf("parsing...\n");
		#endif
		uint8_t cin = packet.cn_cin & 0x0F;	//ignore the Cable Number
		uint8_t ch = (packet.midi[0] & 0x0F) + 1;		//extract channel number (will be useful wherever applicable)

		switch(cin){
			case CIN_NOTE_ON:
				usbmidi_cb_note_on(ch, packet.midi[1], packet.midi[2]);
				break;
			case CIN_NOTE_OFF:
				usbmidi_cb_note_off(ch, packet.midi[1], packet.midi[2]);
				break;
			case CIN_SYSEX_ST_CNT:
				if(sysex_rx_idx < (usbmidi_SYSEX_MAX_LEN-4)){
					sysex_rx[sysex_rx_idx++] = packet.midi[0];
					sysex_rx[sysex_rx_idx++] = packet.midi[1];
					sysex_rx[sysex_rx_idx++] = packet.midi[2];
				}
				else{
					USBH_ErrLog("usbmidi_ifc, process_rx, SYSEX_ST_CNT, sysex_rx_idx out of range");
				}
				break;
			case CIN_SYSEX_END_3B:
				if(sysex_rx_idx < (usbmidi_SYSEX_MAX_LEN-4)){
					sysex_rx[sysex_rx_idx++] = packet.midi[0];
					sysex_rx[sysex_rx_idx++] = packet.midi[1];
					sysex_rx[sysex_rx_idx++] = packet.midi[2];
					usbmidi_cb_sysex(sysex_rx, sysex_rx_idx);
					sysex_rx_idx = 0;
				}
				else{
					USBH_ErrLog("usbmidi_ifc, process_rx, SYSEX_END_3B, sysex_rx_idx out of range");
				}
				break;
			case CIN_SYSEX_END_2B:
				if(sysex_rx_idx < (usbmidi_SYSEX_MAX_LEN-3)){
					sysex_rx[sysex_rx_idx++] = packet.midi[0];
					sysex_rx[sysex_rx_idx++] = packet.midi[1];
					usbmidi_cb_sysex(sysex_rx, sysex_rx_idx);
					sysex_rx_idx = 0;
				}
				else{
					USBH_ErrLog("usbmidi_ifc, process_rx, SYSEX_END_2B, sysex_rx_idx out of range");
				}
				break;
		case CIN_SYSEX_END_COMM_1B:
			if(packet.midi[0] != MIDI_STATUS_SYSEX_END){
				usbmidi_cb_syscomm(packet.midi[0], packet.midi[1], packet.midi[2]);
			}	//if(packet.midi[0] != MIDI_STATUS_SYSEX_END){
			else{
				if(sysex_rx_idx < (usbmidi_SYSEX_MAX_LEN-2)){
					sysex_rx[sysex_rx_idx++] = packet.midi[0];
					usbmidi_cb_sysex(sysex_rx, sysex_rx_idx);
					sysex_rx_idx = 0;
				} //if(sysex_rx_idx < (usbmidi_SYSEX_MAX_LEN-2)){
				else{
					USBH_ErrLog("usbmidi_ifc, process_rx, SYSEX_END_COMM_1B, sysex_rx_idx out of range");
				}	//else/if(sysex_rx_idx < (usbmidi_SYSEX_MAX_LEN-2)){
			}	//else/if(packet.midi[0] != MIDI_STATUS_SYSEX_END){
			break;
		case CIN_CC:
			usbmidi_cb_cc(ch, packet.midi[1], packet.midi[2]);
			break;
		case CIN_PC:
			usbmidi_cb_pc(ch, packet.midi[1]);
			break;
		case CIN_PITCH_BEND:
			usbmidi_cb_pitchbend(ch, packet.midi[1], packet.midi[2]);
			break;
		case CIN_CHAN_PRESSURE:
			usbmidi_cb_aftertouch(packet.midi[0], packet.midi[1], packet.midi[2]);
			break;
		case CIN_SYS_COMM_2B:	//2-byte system common, e.g. time code, SongSelect
			usbmidi_cb_syscomm(packet.midi[0], packet.midi[1], packet.midi[2]);
			break;
		case CIN_SYS_COMM_3B:	//3-byte system common, e.g. song pos
			usbmidi_cb_syscomm(packet.midi[0], packet.midi[1], packet.midi[2]);
			break;
		case CIN_SINGLE_BYTE:
			usbmidi_cb_byte(packet.midi[0]);
			break;
		default:
			USBH_ErrLog("usbmidi_ifc, process_rx, default, unsupported CIN=%X",cin);
			break;
		}//switch
	}//while(1)
}

static void tx_task(void* params){
	static T_usbmidi_EVENT_PACKET packet;
	const TickType_t TIMEOUT = 100;

	while(1){
		//xprintf("*");
		if( xQueueReceive(midi_out_queue, &packet, TIMEOUT) ){
			TSTPRINT("rxed something on midi_out_queue");
			if( xSemaphoreTake(tx_busy,TIMEOUT) == pdTRUE ){	//take the semaphore - will be released @ tx end callback
				TSTPRINT("tx semphr obtained\n");
				USBH_MIDI_Transmit(phost,(uint8_t*)&packet,4);
			}
			else{
				USBH_ErrLog("usbmidi_ifc, process_tx: could not get tx_busy semphr");
			}//else / if( xQueueReceive(midi_out_queue
		}//if( xQueueReceive(midi_out_queue
	}
}

void USBH_MIDI_ReceiveCallback(USBH_HandleTypeDef *phost){
	uint16_t data_len = USBH_MIDI_GetLastReceivedDataSize(phost);
	TSTPRINT("usbmidi_ifc: rxed data len=%02d:\n",data_len);
	if(data_len < 4) {
		USBH_ErrLog("USBH_MIDI_ReceiveCallback: data_len < 4 (%d). Ignoring.\n",(unsigned int)data_len);
		return;
	}
	if(data_len & 0x03){
		USBH_ErrLog("USBH_MIDI_ReceiveCallback: unaligned length of %d. Will truncate.\n",(unsigned int)data_len);
		data_len = data_len & ((uint16_t)(~0x3));
	}
	T_usbmidi_EVENT_PACKET* packet = (T_usbmidi_EVENT_PACKET*)MIDI_RX_Buffer;
	int packets_nb = data_len / 4;

	for( int packet_idx = 0; packet_idx < packets_nb; packet_idx++){
		const TickType_t TIMEOUT = 100;
		//put data into queue - they will be rxed in the rx_process function
		if(xQueueSend(midi_in_queue,&packet[packet_idx],TIMEOUT) != pdPASS){
			USBH_ErrLog("USBH_MIDI_ReceiveCallback: could not send a packet to midi_in_queue.\n");
		}
	}

	USBH_MIDI_Receive(phost, MIDI_RX_Buffer, RX_BUFF_SIZE); // start a new reception
}

void USBH_MIDI_TransmitCallback(USBH_HandleTypeDef *phost){
	TSTPRINT("USB MIDI TX Cplt\n");
	xSemaphoreGive(tx_busy);
}

/*
 * A function for general purpose transmission of a MIDI packet over USB
 * must contain data in the proper format as described in the MIDI device class docs
 * It always transmits 4 bytes (usbmidi_PACKET_LENGTH).
 * Shorter messages should be padded with zeros.
 * The timeout is hard-coded for the ease of use
 * returns 0 if the packet has been successfully added to the TX queue
 * return -1 if the packet couldn't be added to the TX queue due to e.g. a timeout
 */
int usbmidi_tx_event(T_usbmidi_EVENT_PACKET* packet){
#if TESTING
	xprintf("0x%02X, 0x%02X, 0x%02X, 0x%02X, ",packet->cn_cin,packet->midi[0],packet->midi[1],packet->midi[2]);
#endif

	if( xQueueSend(midi_out_queue,packet,API_TX_TIMEOUT) != pdTRUE ){
		USBH_ErrLog("usbmidi_tx_event: could not send to midi_out_queue");
		return -1;
	}
	else{
		TSTPRINT("usbmidi_tx_event: data sent to Queue");
	}
	return 0;
}

/*
 * sends a MIDI message
 * assumes cable #1 (0)
 * not suitable for SysEx
 */
int usbmidi_tx_message(uint8_t status, uint8_t data1, uint8_t data2){
	TSTPRINT("usbmidi_tx_message: status=%02X, data1=%02X, data2=%02X\n",status,data1,data2);
	T_usbmidi_EVENT_PACKET packet;
	packet.cn_cin = cable;
	packet.midi[0] = 0;
	packet.midi[1] = 0;
	packet.midi[2] = 0;
	uint8_t status_no_ch = status & 0xF0;
	TSTPRINT("usbmidi_tx_message: status_no_ch=%02X\n",status_no_ch);

	switch(status_no_ch){
		case MIDI_STATUS_NOTE_ON:
			packet.cn_cin |= CIN_NOTE_ON;
			packet.midi[0] = status;
			packet.midi[1] = data1;
			packet.midi[2] = data2;
			return usbmidi_tx_event(&packet);
		case MIDI_STATUS_NOTE_OFF:
			packet.cn_cin |= CIN_NOTE_OFF;
			packet.midi[0] = status;
			packet.midi[1] = data1;
			packet.midi[2] = data2;
			return usbmidi_tx_event(&packet);
		case MIDI_STATUS_POLY_AFTERTOUCH:
			packet.cn_cin |= CIN_POLY_KEYPRESS;
			packet.midi[0] = status;
			packet.midi[1] = data1;
			packet.midi[2] = data2;
			return usbmidi_tx_event(&packet);
		case MIDI_STATUS_CONTROL_CHANGE:
			packet.cn_cin |= CIN_CC;
			packet.midi[0] = status;
			packet.midi[1] = data1;
			packet.midi[2] = data2;
			return usbmidi_tx_event(&packet);
		case MIDI_STATUS_PITCH_WHEEL:
			packet.cn_cin |= CIN_PITCH_BEND;
			packet.midi[0] = status;
			packet.midi[1] = data1;
			packet.midi[2] = data2;
			return usbmidi_tx_event(&packet);
		case MIDI_STATUS_PROGRAM_CHANGE:
			packet.cn_cin |= CIN_PC;
			packet.midi[0] = status;
			packet.midi[1] = data1;
			packet.midi[2] = 0;
			return usbmidi_tx_event(&packet);
		case MIDI_STATUS_QFRAME_MTC:
			packet.cn_cin |= CIN_SYS_COMM_2B;
			packet.midi[0] = status;
			packet.midi[1] = data1;
			return usbmidi_tx_event(&packet);
		case MIDI_STATUS_SONG_PTR:
			packet.cn_cin |= CIN_SYS_COMM_3B;
			packet.midi[0] = status;
			packet.midi[1] = data1;
			packet.midi[2] = data2;
			return usbmidi_tx_event(&packet);
		case MIDI_STATUS_TIMING_CLOCK:
		case MIDI_STATUS_START:
		case MIDI_STATUS_CONTINUE:
		case MIDI_STATUS_STOP:
		case MIDI_STATUS_ACTIVE_SENSING:
		case MIDI_STATUS_SYSTEM_RESET:
			packet.cn_cin |= CIN_SYSEX_END_COMM_1B;
			packet.midi[0] = status;
			return usbmidi_tx_event(&packet);
		case MIDI_STATUS_SYSEX_START: return -1;
		case MIDI_STATUS_SYSEX_END: return -1;
		default:
			return -1;
	}//switch

	return -1;
}

int usbmidi_inject_to_midi_in(T_usbmidi_EVENT_PACKET* packet, uint16_t len){
	for(uint16_t packet_idx = 0; packet_idx < len; packet_idx++){
#if TESTING
		T_usbmidi_EVENT_PACKET* pPacket = &packet[packet_idx];
		xprintf("usbmidi_inject_to_midi_in, proc pkt idx=%d: ",packet_idx);
			debug_hexbuf(pPacket, 4);
#endif
			if(xQueueSend(midi_in_queue,&packet[packet_idx],100) != pdPASS){
			USBH_ErrLog("usbmidi_inject_to_midi_in: could not send a packet to midi_in_queue.\n");
		}
	}
	return 0;
}

//transmit
void usbmidi_tx_note_on(uint8_t ch, uint8_t note, uint8_t velocity){
	TSTPRINT("usbmidi_tx_note_on, ch=%X, note=%02X, vel=%02X",ch,note,velocity);
	if(ch > 16) {xprintf("usbmidi_note_on_tx, ch>16\n"); return;}
	if(ch == 0) {xprintf("usbmidi_note_on_tx, ch==0\n"); return;}
	ch--;
	if(note > 0x7F) {xprintf("usbmidi_note_on_tx, note>0x7F\n"); return;}
	if(velocity > 0x7F)  {xprintf("usbmidi_note_on_tx, velo>0x7F\n"); return;}
	usbmidi_tx_message(MIDI_STATUS_NOTE_ON | ch, note, velocity);
}

void usbmidi_tx_note_off(uint8_t ch, uint8_t note, uint8_t velocity){
	TSTPRINT("usbmidi_tx_note_off, ch=%X, note=%02X, vel=%02X",ch,note,velocity);
	if(ch > 16) {xprintf("usbmidi_note_on_tx, ch>16\n"); return;}
	if(ch == 0) {xprintf("usbmidi_note_on_tx, ch==0\n"); return;}
	ch--;
	if(note > 0x7F) {xprintf("usbmidi_note_on_tx, note>0x7F\n"); return;}
	if(velocity > 0x7F)  {xprintf("usbmidi_note_on_tx, velo>0x7F\n"); return;}
	usbmidi_tx_message(MIDI_STATUS_NOTE_OFF | ch, note, velocity);
}

void usbmidi_tx_pc(uint8_t ch, uint8_t program){						//program change
	TSTPRINT("usbmidi_pc_tx, ch=%X, program=%02X\n",ch,program);
	if(ch > 16) return;
	if(ch == 0) return;
	ch--;
	if(program > 0x7F) return;
	usbmidi_tx_message(MIDI_STATUS_PROGRAM_CHANGE | ch, program, 0);
}

void usbmidi_tx_cc(uint8_t ch, uint8_t ctrl, uint8_t value){			//control change rxed
	TSTPRINT("usbmidi_note_off_tx, ch=%X, ctrl=%02X, value=%02X\n",ch,ctrl,value);
	if(ch > 16) return;
	if(ch == 0) return;
	ch--;
	if(ctrl > 0x7F) return;
	if(value > 0x7F) return;
	usbmidi_tx_message(MIDI_STATUS_CONTROL_CHANGE | ch, ctrl, value);
}


void usbmidi_tx_sysex(uint8_t* buf, uint16_t len){
	int i = 0;
	while(i<len){
		T_usbmidi_EVENT_PACKET packet;
		int remaining  = len - i;
		if(i==0 && remaining <= 3){
			//short sysex: whole message fits into one packet
			packet.cn_cin = cable | ( remaining == 1 ? CIN_SYSEX_END_COMM_1B : remaining == 2 ? CIN_SYSEX_END_2B : CIN_SYSEX_END_3B);
			packet.midi[0] = buf[i++];
			packet.midi[1] = (remaining > 1) ? buf[i++] : 0;
			packet.midi[2] = (remaining > 2) ? buf[i++] : 0;
		}
		else if (i==0){
			//start of sysex
			packet.cn_cin = cable | CIN_SYSEX_ST_CNT;
			packet.midi[0] = buf[i++];
			packet.midi[1] = buf[i++];
			packet.midi[2] = buf[i++];
		}
		else if (remaining <= 3){
			//end of long sysex
			packet.cn_cin = cable | (remaining == 1 ? CIN_SYSEX_END_COMM_1B : remaining == 2 ? CIN_SYSEX_END_2B : CIN_SYSEX_END_3B);
			packet.midi[0] = buf[i++];
			packet.midi[1] = (remaining > 1) ? buf[i++] : 0;
			packet.midi[2] = (remaining > 2) ? buf[i++] : 0;
		}
		else{
			packet.cn_cin = cable | CIN_SYSEX_ST_CNT;
			packet.midi[0] = buf[i++];
			packet.midi[1] = buf[i++];
			packet.midi[2] = buf[i++];
		}
		usbmidi_tx_event(&packet);
	}//while(i<len)
}



int usbmidi_init(void){
	midi_out_queue = xQueueCreate(MIDI_QUEUE_LEN, sizeof(T_usbmidi_EVENT_PACKET));
	midi_in_queue  = xQueueCreate(MIDI_QUEUE_LEN, sizeof(T_usbmidi_EVENT_PACKET));
	tx_busy = xSemaphoreCreateBinary();
	BaseType_t res;
	//res = xTaskCreate(usbmidi_core_task, "mcore", configMINIMAL_STACK_SIZE + 512, NULL, osPriorityAboveNormal, NULL);
	//if(res != pdPASS) {USBH_ErrLog("usbmidi_core_task not created\n"); return -1;}
	res = xTaskCreate(rx_task, "rx", configMINIMAL_STACK_SIZE + 128, NULL, osPriorityNormal, NULL);
	if(res != pdPASS) {USBH_ErrLog("rx_task not created\n"); return -1;}
	res = xTaskCreate(tx_task, "tx", configMINIMAL_STACK_SIZE + 128, NULL, osPriorityNormal, NULL);
	if(res != pdPASS) {USBH_ErrLog("tx_task not created\n"); return -1;}

	if(midi_out_queue == NULL) {USBH_ErrLog("midi_out_queue not created\n"); return -1;}
	if(midi_in_queue == NULL) {USBH_ErrLog("midi_in_queue not created\n"); return -1;}
	if(tx_busy == NULL) {USBH_ErrLog("tx_busy semaphore not created\n"); return -1;}

	xprintf("usbmidi_init OK\n");
	return 0;
}


void usbmidi_set_cable(uint8_t p_cable){
	cable = p_cable << 4;
}


void usbmidi_start(void){
	xprintf("usbmidi_start...\n");
	vTaskDelay(1000);
	USBH_MIDI_Receive(phost, MIDI_RX_Buffer, RX_BUFF_SIZE); //initiate the rx of the first packet
	xSemaphoreGive(tx_busy);
	xprintf("usbmidi_start exit\n");
}

__weak void usbmidi_cb_byte(uint8_t b){
	WEAK_CB_PRINT("WEAK Callback: usbmidi_cb_byte, b=%X\n",b);
}

__weak void usbmidi_cb_note_on(uint8_t ch, uint8_t note, uint8_t velocity){
	WEAK_CB_PRINT("WEAK Callback: usbmidi_note_on_cb, ch=%X, note=%02X, velo=%02X\n",ch,note,velocity);
}

__weak void usbmidi_cb_note_off(uint8_t ch, uint8_t note, uint8_t velocity){
	WEAK_CB_PRINT("WEAK Callback: usbmidi_note_off_cb, ch=%X, note=%02X, velo=%02X\n",ch,note,velocity);
}

__weak void usbmidi_cb_pitchbend(uint8_t ch, uint8_t data1, uint8_t data2){
	WEAK_CB_PRINT("WEAK Callback: usbmidi_pitchbend_cb, ch=%X, data1=%02X, data2=%02X\n",ch,data1,data2);
}

__weak void usbmidi_cb_aftertouch(uint8_t ch, uint8_t data1, uint8_t data2){
	WEAK_CB_PRINT("WEAK Callback: usbmidi_cb_aftertouch, ch=%X, data1=%02X, data2=%02X\n",ch,data1,data2);
}

__weak void usbmidi_cb_syscomm(uint8_t status, uint8_t data1, uint8_t data2){	//raw format, channel is encoded in status
	WEAK_CB_PRINT("WEAK Callback: usbmidi_syscomm_cb, status=%X, data1=%02X, data2=%02X\n",status,data1,data2);
}

__weak void usbmidi_cb_pc(uint8_t ch, uint8_t program){						//program change
	WEAK_CB_PRINT("WEAK Callback: usbmidi_pc_cb, ch=%X, program=%02X\n",ch,program);
}

__weak void usbmidi_cb_cc(uint8_t ch, uint8_t ctrl, uint8_t value){			//control change rxed
	WEAK_CB_PRINT("WEAK Callback: usbmidi_cc_cb");
}

__weak void usbmidi_cb_sysex(uint8_t* buf, uint16_t len){						//buf contains raw data, including the 0xF0 & 0xF7
	WEAK_CB_PRINT("WEAK Callback: usbmidi_sysex_cb, len=%02X\n",len);
}


