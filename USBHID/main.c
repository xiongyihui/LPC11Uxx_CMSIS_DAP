/**
 * @file    main.c
 * @brief   Using Arch as a debug adapter
 * @date    July 3, 2013
 * @version 1.0
 */

#include "lpc11Uxx.h"                        /* LPC11xx definitions */
#include "type.h"

#include "usb.h"
#include "usbcfg.h"
#include "usbhw.h"
#include "usbcore.h"
#include "io_event.h"

#include "dap.h"

/* Main Program */
int main (void) 
{  
  SystemCoreClockUpdate();
    
  DAP_Setup();

  /* P1.8~9 are LED output. */
  LPC_GPIO->DIR[1] |= (0x1 << 8) | (0x1 << 9);
  LPC_GPIO->CLR[1] |= (0x1 << 8);
  LPC_GPIO->SET[1] |= (0x1 << 9);

  USB_Init();                               /* USB Initialization */
  USB_Connect(TRUE);                        /* USB Connect */

  while (1)                                /* Loop forever */
  {
		extern void usbd_hid_process(void);
		usbd_hid_process();
  }
}
