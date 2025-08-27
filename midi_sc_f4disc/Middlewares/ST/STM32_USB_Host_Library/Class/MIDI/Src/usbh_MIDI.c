/*
 * This file is based on the the usbh_cdc.c from the ST
 * CubeMX software pack.
 */

//the original header:
/**
  ******************************************************************************
  * @file    usbh_cdc.c
  * @author  MCD Application Team
  * @brief   This file is the CDC Layer Handlers for USB Host CDC class.
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  *  @verbatim
  *
  *          ===================================================================
  *                                CDC Class Driver Description
  *          ===================================================================
  *           This driver manages the "Universal Serial Bus Class Definitions for Communications Devices
  *           Revision 1.2 November 16, 2007" and the sub-protocol specification of "Universal Serial Bus
  *           Communications Class Subclass Specification for PSTN Devices Revision 1.2 February 9, 2007"
  *           This driver implements the following aspects of the specification:
  *             - Device descriptor management
  *             - Configuration descriptor management
  *             - Enumeration as CDC device with 2 data endpoints (IN and OUT) and 1 command endpoint (IN)
  *             - Requests management (as described in section 6.2 in specification)
  *             - Abstract Control Model compliant
  *             - Union Functional collection (using 1 IN endpoint for control)
  *             - Data interface class
  *
  *  @endverbatim
  *
  ******************************************************************************
  */

  
  
#include "usbh_MIDI.h"


static USBH_StatusTypeDef USBH_MIDI_InterfaceInit  (USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_InterfaceDeInit  (USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_Process(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_SOFProcess(USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef USBH_MIDI_ClassRequest (USBH_HandleTypeDef *phost);
static void MIDI_ProcessTransmission(USBH_HandleTypeDef *phost);
static void MIDI_ProcessReception(USBH_HandleTypeDef *phost);

USBH_ClassTypeDef  MIDI_Class =
{
    "MIDI",
    USB_AUDIO_CLASS,
    USBH_MIDI_InterfaceInit,
    USBH_MIDI_InterfaceDeInit,
    USBH_MIDI_ClassRequest,
    USBH_MIDI_Process,
    USBH_MIDI_SOFProcess,
    NULL
};


/**
  * @brief  USBH_FindInterface_MC_MIDI (rewritten & added here by Ada Locriana)
  *     The original version didn't perform a proper scan through the interface descriptors
  *     (ST, how could you!)
  *     functionality:
  *         Find the interface index for a specific class modified for MC-101 & MC-707
  * @param  phost: Host Handle
  * @param  Class: Class code
  * @param  SubClass: SubClass code
  * @param  Protocol: Protocol code
  * @retval interface index in the configuration structure
  * @note : (1)interface index 0xFF means interface index not found
  */
uint8_t USBH_FindInterface_MC_MIDI(USBH_HandleTypeDef *phost)
{
  USBH_InterfaceDescTypeDef *pif;
  USBH_CfgDescTypeDef *pcfg;
  uint8_t if_ix = 0U;

  //we're looking for: USB_AUDIO_CLASS (0x01) & USB_MIDISTREAMING_SubCLASS (0x03)

  pif = (USBH_InterfaceDescTypeDef *)NULL;
  pcfg = &phost->device.CfgDesc;

  while (if_ix < USBH_MAX_NUM_INTERFACES)
  {
  pif = &pcfg->Itf_Desc[if_ix];
  USBH_DbgLog("scanning interface idx=%d",if_ix);

#if 0
  debug_dump(pif,sizeof(pcfg->Itf_Desc[if_ix]));
#endif

    if ((pif->bInterfaceClass == USB_AUDIO_CLASS) &&
        (pif->bInterfaceSubClass == USB_MIDISTREAMING_SubCLASS))
    {
      return  if_ix;
    }

    if_ix++;
  }
  return 0xFFU;
}


/**
  * @brief  USBH_SelectInterface_Fix
  *     (rewritten & added here by Ada Locriana)
  *         Selects current interface - supports the real number of interfaces
  *         and will not produce a nonsense error for a device which has
  *         lots of interface descriptors but reports less (like the MCs)
  * @param  phost: Host Handle
  * @param  interface: Interface number
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_SelectInterface_Fix(USBH_HandleTypeDef *phost, uint8_t interface)
{
  USBH_StatusTypeDef status = USBH_OK;

  /*
   * just ignore this check
   * yes, the MCs return only bNumInterfaces = 4 ;)
   */
#if 1
  phost->device.current_interface = interface;
  USBH_UsrLog("Switching to Interface (#%d) !!! overriding the bNumInterfaces check !!!", interface);
  USBH_UsrLog("Class    : %xh", phost->device.CfgDesc.Itf_Desc[interface].bInterfaceClass);
  USBH_UsrLog("SubClass : %xh", phost->device.CfgDesc.Itf_Desc[interface].bInterfaceSubClass);
  USBH_UsrLog("Protocol : %xh", phost->device.CfgDesc.Itf_Desc[interface].bInterfaceProtocol);
#else
  if (interface < phost->device.CfgDesc.bNumInterfaces)
  {
    phost->device.current_interface = interface;
    USBH_UsrLog("Switching to Interface (#%d)", interface);
    USBH_UsrLog("Class    : %xh", phost->device.CfgDesc.Itf_Desc[interface].bInterfaceClass);
    USBH_UsrLog("SubClass : %xh", phost->device.CfgDesc.Itf_Desc[interface].bInterfaceSubClass);
    USBH_UsrLog("Protocol : %xh", phost->device.CfgDesc.Itf_Desc[interface].bInterfaceProtocol);
  }
  else
  {
    USBH_ErrLog("Cannot Select This Interface.");
    status = USBH_FAIL;
  }
#endif


  return status;
}



/**
 * @brief  USBH_MIDI_InterfaceInit
 *         The function which initializes the MIDI class.
 * @param  phost: Host handle
 * @retval USBH Status
 * Modified by Ada Locriana
 * Btw. a good resrouce:
 * https://community.st.com/t5/stm32-mcus-products/usb-library-with-device-having-multiple-interfaces/td-p/448212
 */
static USBH_StatusTypeDef USBH_MIDI_InterfaceInit (USBH_HandleTypeDef *phost)
{

  USBH_StatusTypeDef status = USBH_FAIL ;
  uint8_t interface = 0;
  MIDI_HandleTypeDef *MIDI_Handle;

  interface = USBH_FindInterface_MC_MIDI(phost);

  USBH_DbgLog ("Interface found, if=%d ...",interface);

//testing only ;)
#if 0
  interface = 5;
  USBH_DbgLog ("...but hardcoded to %d",interface);
#endif

  if(interface == 0xFF) /* No Valid Interface */
  {
    USBH_DbgLog ("Cannot Find the interface for MIDI Interface Class.");
    status = USBH_FAIL;
  }
  else
  {
  USBH_SelectInterface_Fix (phost, interface);
    phost->pActiveClass->pData = (MIDI_HandleTypeDef *)USBH_malloc (sizeof(MIDI_HandleTypeDef));
    MIDI_Handle =  (MIDI_HandleTypeDef *)phost->pActiveClass->pData;
    
    if (MIDI_Handle == NULL)
    {
      USBH_DbgLog("Cannot allocate memory for MIDI Handle");
      return USBH_FAIL;
    }

    USBH_memset(MIDI_Handle, 0, sizeof(MIDI_HandleTypeDef));

    USBH_DbgLog("phost->device.current_interface = %d",phost->device.current_interface);
    USBH_DbgLog("Ep_Desc[0].bEndpointAddress = 0x%02X",phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[0].bEndpointAddress);
    USBH_DbgLog("Ep_Desc[1].bEndpointAddress = 0x%02X",phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[1].bEndpointAddress);
    USBH_DbgLog("Ep_Desc[0].wMaxPacketSize = %d",phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[0].wMaxPacketSize);
    USBH_DbgLog("Ep_Desc[1].wMaxPacketSize = %d",phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[1].wMaxPacketSize);

    if(phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[0].bEndpointAddress & 0x80)
    {
      USBH_DbgLog("Setting EP %d as IN", 0x0F & phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[0].bEndpointAddress);
      MIDI_Handle->InEp = (phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[0].bEndpointAddress);
      MIDI_Handle->InEpSize  = phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[0].wMaxPacketSize;
    }
    else
    {
      USBH_DbgLog("Setting EP %d as OUT", 0x0F & phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[0].bEndpointAddress);
      MIDI_Handle->OutEp = (phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[0].bEndpointAddress);
      MIDI_Handle->OutEpSize  = phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[0].wMaxPacketSize;
    }

    if(phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[1].bEndpointAddress & 0x80)
    {
      USBH_DbgLog("Setting EP %d as IN", 0x0F & phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[1].bEndpointAddress);
      MIDI_Handle->InEp = (phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[1].bEndpointAddress);
      MIDI_Handle->InEpSize  = phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[1].wMaxPacketSize;
    }
    else
    {
      USBH_DbgLog("Setting EP %d as OUT", 0x0F & phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[1].bEndpointAddress);
      MIDI_Handle->OutEp = (phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[1].bEndpointAddress);
      MIDI_Handle->OutEpSize  = phost->device.CfgDesc.Itf_Desc[phost->device.current_interface].Ep_Desc[1].wMaxPacketSize;
    }


    MIDI_Handle->OutPipe = USBH_AllocPipe(phost, MIDI_Handle->OutEp);
    MIDI_Handle->InPipe = USBH_AllocPipe(phost, MIDI_Handle->InEp);

    USBH_DbgLog("MIDI_Handle->OutEp = 0x%02X",MIDI_Handle->OutEp);
    USBH_DbgLog("MIDI_Handle->InEp = 0x%02X",MIDI_Handle->InEp);
    USBH_DbgLog("MIDI_Handle->OutPipe = 0x%02X",MIDI_Handle->OutPipe);
    USBH_DbgLog("MIDI_Handle->InPipe = 0x%02X",MIDI_Handle->InPipe);

    /* Open the new channels */
    USBH_OpenPipe  (phost,
        MIDI_Handle->OutPipe,
        MIDI_Handle->OutEp,
        phost->device.address,
        phost->device.speed,
        USB_EP_TYPE_BULK,
        MIDI_Handle->OutEpSize);

    USBH_OpenPipe  (phost,
        MIDI_Handle->InPipe,
        MIDI_Handle->InEp,
        phost->device.address,
        phost->device.speed,
        USB_EP_TYPE_BULK,
        MIDI_Handle->InEpSize);

    //USB_MIDI_ChangeConnectionState(1);
    MIDI_Handle->state = MIDI_IDLE_STATE;


    USBH_LL_SetToggle  (phost, MIDI_Handle->InPipe,0);
    USBH_LL_SetToggle  (phost, MIDI_Handle->OutPipe,0);
    status = USBH_OK;
  }
  return status;
}


/*------------------------------------------------------------------------------------------------------------------------------*/

/**
 * @brief  USBH_MIDI_InterfaceDeInit
 *         The function DeInit the Pipes used for the MIDI class.
 * @param  phost: Host handle
 * @retval USBH Status
 */
USBH_StatusTypeDef USBH_MIDI_InterfaceDeInit (USBH_HandleTypeDef *phost)
{
  MIDI_HandleTypeDef *MIDI_Handle =  phost->pActiveClass->pData;

  if ( MIDI_Handle->OutPipe)
  {
    USBH_ClosePipe(phost, MIDI_Handle->OutPipe);
    USBH_FreePipe  (phost, MIDI_Handle->OutPipe);
    MIDI_Handle->OutPipe = 0;     /* Reset the Channel as Free */
  }

  if ( MIDI_Handle->InPipe)
  {
    USBH_ClosePipe(phost, MIDI_Handle->InPipe);
    USBH_FreePipe  (phost, MIDI_Handle->InPipe);
    MIDI_Handle->InPipe = 0;     /* Reset the Channel as Free */
  }

  if((phost->pActiveClass->pData) != NULL)
  {
    USBH_free (phost->pActiveClass->pData);
    phost->pActiveClass->pData = 0U;
  }

  return USBH_OK;
}

/*------------------------------------------------------------------------------------------------------------------------------*/

/**
 * @brief  USBH_MIDI_ClassRequest
 *         The function is responsible for handling Standard requests
 *         for MIDI class.
 * @param  phost: Host handle
 * @retval USBH Status
 */
static USBH_StatusTypeDef USBH_MIDI_ClassRequest (USBH_HandleTypeDef *phost)
{   
  phost->pUser(phost, HOST_USER_CLASS_ACTIVE);

  return USBH_OK;
}

/*------------------------------------------------------------------------------------------------------------------------------*/

/**
  * @brief  USBH_MIDI_Stop
  *         Stop current MIDI Transmission
  * @param  phost: Host handle
  * @retval USBH Status
  */
USBH_StatusTypeDef  USBH_MIDI_Stop(USBH_HandleTypeDef *phost)
{
  MIDI_HandleTypeDef *MIDI_Handle =  phost->pActiveClass->pData;

  if(phost->gState == HOST_CLASS)
  {
    MIDI_Handle->state = MIDI_IDLE_STATE;

    USBH_ClosePipe(phost, MIDI_Handle->InPipe);
    USBH_ClosePipe(phost, MIDI_Handle->OutPipe);
  }
  return USBH_OK;
}

/*------------------------------------------------------------------------------------------------------------------------------*/

/**
 * @brief  USBH_MIDI_Process
 *         The function is for managing state machine for MIDI data transfers
 *         (background process)
 * @param  phost: Host handle
 * @retval USBH Status
 */
static USBH_StatusTypeDef USBH_MIDI_Process (USBH_HandleTypeDef *phost)
{
  USBH_StatusTypeDef status = USBH_BUSY;
  USBH_StatusTypeDef req_status = USBH_OK;
  MIDI_HandleTypeDef *MIDI_Handle =  phost->pActiveClass->pData;

  vTaskDelay(1);

  switch(MIDI_Handle->state)
  {

  case MIDI_IDLE_STATE:
    status = USBH_OK;
    break;

  case MIDI_TRANSFER_DATA:

    MIDI_ProcessTransmission(phost);
    MIDI_ProcessReception(phost);
    break;

  case MIDI_ERROR_STATE:
    req_status = USBH_ClrFeature(phost, 0x00);

    if(req_status == USBH_OK )
    {
      /*Change the state to waiting*/
      MIDI_Handle->state = MIDI_IDLE_STATE ;
    }
    break;

  default:
    break;

  }
  return status;
}
/*------------------------------------------------------------------------------------------------------------------------------*/

/**
  * @brief  USBH_MIDI_SOFProcess 
  *         The function is for managing SOF callback 
  * @param  phost: Host handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_MIDI_SOFProcess (USBH_HandleTypeDef *phost)
{
  return USBH_OK;  
}
  
/*------------------------------------------------------------------------------------------------------------------------------*/

/**
 * @brief  This function return last recieved data size
 * @param  None
 * @retval None
 */
uint16_t USBH_MIDI_GetLastReceivedDataSize(USBH_HandleTypeDef *phost)
{
  MIDI_HandleTypeDef *MIDI_Handle =  phost->pActiveClass->pData;

  if(phost->gState == HOST_CLASS)
  {
    return USBH_LL_GetLastXferSize(phost, MIDI_Handle->InPipe);
  }
  else
  {
    return 0;
  }
}

/*------------------------------------------------------------------------------------------------------------------------------*/

/**
 * @brief  This function prepares the state before issuing the class specific commands
 * @param  None
 * @retval None
 */
USBH_StatusTypeDef  USBH_MIDI_Transmit(USBH_HandleTypeDef *phost, uint8_t *pbuff, uint16_t length)
{
  //USBH_DbgLog("USBH_MIDI_Transmit: start");
  USBH_StatusTypeDef Status = USBH_BUSY;
  MIDI_HandleTypeDef *MIDI_Handle =  phost->pActiveClass->pData;

  if((MIDI_Handle->state == MIDI_IDLE_STATE) || (MIDI_Handle->state == MIDI_TRANSFER_DATA))
  {
    //USBH_DbgLog("USBH_MIDI_Transmit: state IDLE or TRANSFER");
    MIDI_Handle->pTxData = pbuff;
    MIDI_Handle->TxDataLength = length;
    MIDI_Handle->state = MIDI_TRANSFER_DATA;
    MIDI_Handle->data_tx_state = MIDI_SEND_DATA;
    Status = USBH_OK;
#if (USBH_USE_OS == 1U)
    USBH_OS_PutMessage(phost, USBH_CLASS_EVENT, 0U, 0U);
#endif /* (USBH_USE_OS == 1U) */
  }
  //USBH_DbgLog("USBH_MIDI_Transmit returns status %d",Status);
  return Status;
}

/*------------------------------------------------------------------------------------------------------------------------------*/

/**
 * @brief  This function prepares the state before issuing the class specific commands
 * @param  None
 * @retval None
 */
USBH_StatusTypeDef  USBH_MIDI_Receive(USBH_HandleTypeDef *phost, uint8_t *pbuff, uint16_t length)
{
  USBH_StatusTypeDef Status = USBH_BUSY;
  MIDI_HandleTypeDef *MIDI_Handle =  phost->pActiveClass->pData;

  if((MIDI_Handle->state == MIDI_IDLE_STATE) || (MIDI_Handle->state == MIDI_TRANSFER_DATA))
  {
    MIDI_Handle->pRxData = pbuff;
    MIDI_Handle->RxDataLength = length;
    MIDI_Handle->state = MIDI_TRANSFER_DATA;
    MIDI_Handle->data_rx_state = MIDI_RECEIVE_DATA;
    Status = USBH_OK;
  }
#if (USBH_USE_OS == 1U)
    USBH_OS_PutMessage(phost, USBH_CLASS_EVENT, 0U, 0U);
#endif /* (USBH_USE_OS == 1U) */
  //USBH_DbgLog("USBH_MIDI_Receive returns status %d",Status);
  return Status;
}

/*------------------------------------------------------------------------------------------------------------------------------*/

/**
 * @brief  The function is responsible for sending data to the device
 *  @param  pdev: Selected device
 * @retval None
 */
static void MIDI_ProcessTransmission(USBH_HandleTypeDef *phost)
{
  MIDI_HandleTypeDef *MIDI_Handle =  phost->pActiveClass->pData;
  USBH_URBStateTypeDef URB_Status = USBH_URB_IDLE;

  switch(MIDI_Handle->data_tx_state)
  {

  case MIDI_SEND_DATA:
    //USBH_DbgLog("MIDI_ProcessTransmission, MIDI_SEND_DATA");

    if(MIDI_Handle->TxDataLength > MIDI_Handle->OutEpSize)
    {
      USBH_BulkSendData (phost,
          MIDI_Handle->pTxData,
          MIDI_Handle->OutEpSize,
          MIDI_Handle->OutPipe,
          1U);
    }
    else
    {
      USBH_BulkSendData (phost,
          MIDI_Handle->pTxData,
          (uint16_t)MIDI_Handle->TxDataLength,
          MIDI_Handle->OutPipe,
          1U);
    }

    MIDI_Handle->data_tx_state = MIDI_SEND_DATA_WAIT;
    break;

  case MIDI_SEND_DATA_WAIT:
  
    URB_Status = USBH_LL_GetURBState(phost, MIDI_Handle->OutPipe);

    /*Check the status done for transmission*/
    if(URB_Status == USBH_URB_DONE )
    {
      if(MIDI_Handle->TxDataLength > MIDI_Handle->OutEpSize)
      {
        MIDI_Handle->TxDataLength -= MIDI_Handle->OutEpSize ;
        MIDI_Handle->pTxData += MIDI_Handle->OutEpSize;
      }
      else
      {
        MIDI_Handle->TxDataLength = 0;
      }

      if( MIDI_Handle->TxDataLength > 0)
      {
        MIDI_Handle->data_tx_state = MIDI_SEND_DATA;
      }
      else
      {
        MIDI_Handle->data_tx_state = MIDI_IDLE;
        //USBH_DbgLog("calling the USBH_MIDI_TransmitCallback");
        USBH_MIDI_TransmitCallback(phost);
      }
#if (USBH_USE_OS == 1U)
        USBH_OS_PutMessage(phost, USBH_CLASS_EVENT, 0U, 0U);
#endif /* (USBH_USE_OS == 1U) */
    }
    else
    {
      if( URB_Status == USBH_URB_NOTREADY ){
      MIDI_Handle->data_tx_state = MIDI_SEND_DATA;
#if (USBH_USE_OS == 1U)
      USBH_OS_PutMessage(phost, USBH_CLASS_EVENT, 0U, 0U);
#endif /* (USBH_USE_OS == 1U) */
      }
    }
    break;
  default:
    break;
  }
}

/**
 * @brief  This function responsible for reception of data from the device
 *  @param  pdev: Selected device
 * @retval None
 */

static void MIDI_ProcessReception(USBH_HandleTypeDef *phost)
{
  MIDI_HandleTypeDef *MIDI_Handle =  phost->pActiveClass->pData;
  USBH_URBStateTypeDef URB_Status = USBH_URB_IDLE;
  uint16_t length;

  switch(MIDI_Handle->data_rx_state)
  {

  case MIDI_RECEIVE_DATA:
    USBH_BulkReceiveData (phost,
        MIDI_Handle->pRxData,
        MIDI_Handle->InEpSize,
        MIDI_Handle->InPipe);

#if defined (USBH_IN_NAK_PROCESS) && (USBH_IN_NAK_PROCESS == 1U)
      phost->NakTimer = phost->Timer;
#endif  /* defined (USBH_IN_NAK_PROCESS) && (USBH_IN_NAK_PROCESS == 1U) */

      MIDI_Handle->data_rx_state = MIDI_RECEIVE_DATA_WAIT;
      break;

  case MIDI_RECEIVE_DATA_WAIT:

    URB_Status = USBH_LL_GetURBState(phost, MIDI_Handle->InPipe);



    /*Check the status done for reception*/
    if(URB_Status == USBH_URB_DONE )
    {


      length = USBH_LL_GetLastXferSize(phost, MIDI_Handle->InPipe);

      if(((MIDI_Handle->RxDataLength - length) > 0U) && (length > MIDI_Handle->InEpSize))
      {
        MIDI_Handle->RxDataLength -= length ;
        MIDI_Handle->pRxData += length;
        MIDI_Handle->data_rx_state = MIDI_RECEIVE_DATA;
      }
      else
      {
        MIDI_Handle->data_rx_state = MIDI_IDLE;
        //USBH_DbgLog("calling the USBH_MIDI_ReceiveCallback");
        USBH_MIDI_ReceiveCallback(phost);
      }
#if (USBH_USE_OS == 1U)
        USBH_OS_PutMessage(phost, USBH_CLASS_EVENT, 0U, 0U);
#endif /* (USBH_USE_OS == 1U) */
    }
#if defined (USBH_IN_NAK_PROCESS) && (USBH_IN_NAK_PROCESS == 1U)
      else if (URB_Status == USBH_URB_NAK_WAIT)
      {
        MIDI_Handle->data_rx_state = MIDI_RECEIVE_DATA_WAIT;

        if ((phost->Timer - phost->NakTimer) > phost->NakTimeout)
        {
          phost->NakTimer = phost->Timer;
          USBH_ActivatePipe(phost, MIDI_Handle->DataItf.InPipe);
        }

#if (USBH_USE_OS == 1U)
        USBH_OS_PutMessage(phost, USBH_CLASS_EVENT, 0U, 0U);
#endif /* (USBH_USE_OS == 1U) */
      }
#endif /* defined (USBH_IN_NAK_PROCESS) && (USBH_IN_NAK_PROCESS == 1U) */
    else{
       /* ... */
    }
    break;

  default:
    break;
  }
}


/**
 * @brief  The function informs user that data have been transmitted.
 *  @param  pdev: Selected device
 * @retval None
 */
__weak void USBH_MIDI_TransmitCallback(USBH_HandleTypeDef *phost)
{
  USBH_DbgLog("(weak) USBH_MIDI_TransmitCallback");

}


/**
 * @brief  The function informs user that data have been received.
 * @retval None
 */
__weak void USBH_MIDI_ReceiveCallback(USBH_HandleTypeDef *phost)
{
  USBH_DbgLog("(weak) USBH_MIDI_ReceiveCallback");

}
