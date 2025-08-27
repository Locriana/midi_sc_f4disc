#ifndef __USBH_MIDI_H
#define __USBH_MIDI_H

#include "main.h"
#include "usbh_core.h"

#define USB_MIDI_DATA_IN_SIZE     64
#define USB_MIDI_DATA_OUT_SIZE    64

#define USB_AUDIO_CLASS                 0x01
#define USB_MIDISTREAMING_SubCLASS      0x03
#define USB_MIDI_DESC_SIZE                 9
#define USBH_MIDI_CLASS    &MIDI_Class

extern USBH_ClassTypeDef  MIDI_Class;

typedef enum
{
  MIDI_IDLE= 0,
  MIDI_SEND_DATA,
  MIDI_SEND_DATA_WAIT,
  MIDI_RECEIVE_DATA,
  MIDI_RECEIVE_DATA_WAIT,
}
MIDI_DataStateTypeDef;

typedef enum
{
  MIDI_IDLE_STATE= 0,
  MIDI_TRANSFER_DATA,
  MIDI_ERROR_STATE,
}
MIDI_StateTypeDef;

typedef struct _MIDI_Process
{
  MIDI_StateTypeDef     state;
  uint8_t     InPipe;
  uint8_t     OutPipe;
  uint8_t     OutEp;
  uint8_t     InEp;
  uint16_t    OutEpSize;
  uint16_t    InEpSize;

  uint8_t     *pTxData;
  uint8_t     *pRxData;
  uint16_t    TxDataLength;
  uint16_t    RxDataLength;
  MIDI_DataStateTypeDef   data_tx_state;
  MIDI_DataStateTypeDef   data_rx_state;
  uint8_t           Rx_Poll;
}
MIDI_HandleTypeDef;

/*---------------------------Exported_FunctionsPrototype-------------------------------------*/

USBH_StatusTypeDef  USBH_MIDI_Transmit(USBH_HandleTypeDef *phost,
                                      uint8_t *pbuff,
                                      uint16_t length);

USBH_StatusTypeDef  USBH_MIDI_Receive(USBH_HandleTypeDef *phost,
                                     uint8_t *pbuff,
                                     uint16_t length);


uint16_t            USBH_MIDI_GetLastReceivedDataSize(USBH_HandleTypeDef *phost);

USBH_StatusTypeDef  USBH_MIDI_Stop(USBH_HandleTypeDef *phost);

void USBH_MIDI_TransmitCallback(USBH_HandleTypeDef *phost);

void USBH_MIDI_ReceiveCallback(USBH_HandleTypeDef *phost);

/*-------------------------------------------------------------------------------------------*/
#endif /* __USBH_MIDI_H */
