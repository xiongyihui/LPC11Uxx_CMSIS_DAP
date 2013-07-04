/*----------------------------------------------------------------------------
 *      U S B  -  K e r n e l
 *----------------------------------------------------------------------------
 * Name:    usbuser.c
 * Purpose: USB Custom User Module
 * Version: V1.20
 *----------------------------------------------------------------------------
 *      This software is supplied "AS IS" without any warranties, express,
 *      implied or statutory, including but not limited to the implied
 *      warranties of fitness for purpose, satisfactory quality and
 *      noninfringement. Keil extends you a royalty-free right to reproduce
 *      and distribute executable files created using this software for use
 *      on NXP Semiconductors LPC family microcontroller devices only. Nothing 
 *      else gives you the right to use this software.
 *
 * Copyright (c) 2009 Keil - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/

#include "type.h"

#include "usb.h"
#include "usbcfg.h"
#include "usbhw.h"
#include "usbcore.h"
#include "usbuser.h"
#include "hiduser.h"

#include "DAP_config.h"
#include "DAP.h"

#include <string.h>

static volatile uint8_t  USB_RequestFlag = 0;       // Request  Buffer Usage Flag
static volatile uint32_t USB_RequestIn = 0;         // Request  Buffer In  Index
static volatile uint32_t USB_RequestOut = 0;        // Request  Buffer Out Index

static volatile uint8_t  USB_ResponseIdle = 1;      // Response Buffer Idle  Flag
static volatile uint8_t  USB_ResponseFlag = 0;      // Response Buffer Usage Flag
static volatile uint32_t USB_ResponseIn = 0;        // Response Buffer In  Index
static volatile uint32_t USB_ResponseOut = 0;       // Response Buffer Out Index

static          uint8_t  USB_Request [DAP_PACKET_COUNT][DAP_PACKET_SIZE];  // Request  Buffer
static          uint8_t  USB_Response[DAP_PACKET_COUNT][DAP_PACKET_SIZE];  // Response Buffer


// Process USB HID Data
void usbd_hid_process (void) {
  uint32_t n;

  // Process pending requests
  if ((USB_RequestOut != USB_RequestIn) || USB_RequestFlag) {

    // Process DAP Command and prepare response
    DAP_ProcessCommand(USB_Request[USB_RequestOut], USB_Response[USB_ResponseIn]);

    // Update request index and flag
    n = USB_RequestOut + 1;
    if (n == DAP_PACKET_COUNT) {
      n = 0;
    }
    USB_RequestOut = n;
    if (USB_RequestOut == USB_RequestIn) {
      USB_RequestFlag = 0;
    }

    if (USB_ResponseIdle) {
      // Request that data is send back to host
      USB_ResponseIdle = 0;
      USB_WriteEP(HID_EP_IN, USB_Response[USB_ResponseIn], DAP_PACKET_SIZE);
    } else {
      // Update response index and flag
      n = USB_ResponseIn + 1;
      if (n == DAP_PACKET_COUNT) {
        n = 0;
      }
      USB_ResponseIn = n;
      if (USB_ResponseIn == USB_ResponseOut) {
        USB_ResponseFlag = 1;
      }
    }
  }
}



/*
 *  USB Power Event Callback
 *   Called automatically on USB Power Event
 *    Parameter:       power: On(TRUE)/Off(FALSE)
 */

#if USB_POWER_EVENT
void USB_Power_Event (uint32_t  power) {
}
#endif


/*
 *  USB Reset Event Callback
 *   Called automatically on USB Reset Event
 */

#if USB_RESET_EVENT
void USB_Reset_Event (void) {
  USB_ResetCore();
}
#endif


/*
 *  USB Suspend Event Callback
 *   Called automatically on USB Suspend Event
 */

#if USB_SUSPEND_EVENT
void USB_Suspend_Event (void) {
}
#endif


/*
 *  USB Resume Event Callback
 *   Called automatically on USB Resume Event
 */

#if USB_RESUME_EVENT
void USB_Resume_Event (void) {
}
#endif


/*
 *  USB Remote Wakeup Event Callback
 *   Called automatically on USB Remote Wakeup Event
 */

#if USB_WAKEUP_EVENT
void USB_WakeUp_Event (void) {
}
#endif


/*
 *  USB Start of Frame Event Callback
 *   Called automatically on USB Start of Frame Event
 */

#if USB_SOF_EVENT
void USB_SOF_Event (void) {
}
#endif


/*
 *  USB Error Event Callback
 *   Called automatically on USB Error Event
 *    Parameter:       error: Error Code
 */

#if USB_ERROR_EVENT
void USB_Error_Event (uint32_t error) {
}
#endif


/*
 *  USB Set Configuration Event Callback
 *   Called automatically on USB Set Configuration Request
 */

#if USB_CONFIGURE_EVENT
void USB_Configure_Event (void) {

  if (USB_Configuration) {                  /* Check if USB is configured */
  }
}
#endif


/*
 *  USB Set Interface Event Callback
 *   Called automatically on USB Set Interface Request
 */

#if USB_INTERFACE_EVENT
void USB_Interface_Event (void) {
}
#endif


/*
 *  USB Set/Clear Feature Event Callback
 *   Called automatically on USB Set/Clear Feature Request
 */

#if USB_FEATURE_EVENT
void USB_Feature_Event (void) {
}
#endif


#define P_EP(n) ((USB_EP_EVENT & (1 << (n))) ? USB_EndPoint##n : NULL)

/* USB Endpoint Events Callback Pointers */
void (* const USB_P_EP[USB_LOGIC_EP_NUM]) (uint32_t event) = {
  P_EP(0),
  P_EP(1),
  P_EP(2),
  P_EP(3),
  P_EP(4)
};


/*
 *  USB Endpoint 1 Event Callback
 *   Called automatically on USB Endpoint 1 Event
 *    Parameter:       event
 */

void USB_EndPoint1 (uint32_t event) {
	int len;
	uint8_t *buf;
	
  switch (event) {
    case USB_EVT_IN:
					if ((USB_ResponseOut != USB_ResponseIn) || USB_ResponseFlag) {
						USB_WriteEP(HID_EP_IN, USB_Response[USB_ResponseIn], DAP_PACKET_SIZE);
            USB_ResponseOut++;
            if (USB_ResponseOut == DAP_PACKET_COUNT) {
              USB_ResponseOut = 0;
            }
            if (USB_ResponseOut == USB_ResponseIn) {
              USB_ResponseFlag = 0;
            }
            return;
          } else {
            USB_ResponseIdle = 1;
          }
          break;
    case USB_EVT_OUT:
			buf = USB_Request[USB_RequestIn];
			len = USB_ReadEP(HID_EP_OUT, buf);
      if (len == 0) break;
      if (buf[0] == ID_DAP_TransferAbort) {
        DAP_TransferAbort = 1;
        break;
      }
      if (USB_RequestFlag && (USB_RequestIn == USB_RequestOut)) {
        break;  // Discard packet when buffer is full
      }
			
			
      // Store data into request packet buffer
//      memcpy(USB_Request[USB_RequestIn], buf, len);
      USB_RequestIn++;
      if (USB_RequestIn == DAP_PACKET_COUNT) {
        USB_RequestIn = 0;
      }
      if (USB_RequestIn == USB_RequestOut) {
        USB_RequestFlag = 1;
      }
      break;
  }
}


/*
 *  USB Endpoint 2 Event Callback
 *   Called automatically on USB Endpoint 2 Event
 *    Parameter:       event
 */

void USB_EndPoint2 (uint32_t event) {

}

/*
 *  USB Endpoint 3 Event Callback
 *   Called automatically on USB Endpoint 3 Event
 *    Parameter:       event
 */

void USB_EndPoint3 (uint32_t event) {

}


/*
 *  USB Endpoint 4 Event Callback
 *   Called automatically on USB Endpoint 4 Event
 *    Parameter:       event
 */

void USB_EndPoint4 (uint32_t event) {

}

