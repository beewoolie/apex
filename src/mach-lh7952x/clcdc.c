/* clcdc.c
     $Id$

   written by Marc Singer
   7 Dec 2004

   Copyright (C) 2004 Marc Singer

   -----------
   DESCRIPTION
   -----------

   Test code for the lcd controller.

   NOTES
   -----

   - The master clock is HCLK.  In the RCPC, it can be divided by a
     power of two, but there doesn't seem to be a good reason to do so
     since the LCD controller can divide by an arbitrary number of
     clocks.
   - The only significant difference between this implementation of
     the controller and the lh7a40x implementation is that the ALI
     includes a LCDVEEEN signal.  The lh7a40x used a signal from the
     CPLD.

*/

#include <config.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <apex.h>
#include "hardware.h"

static unsigned short __attribute__((section(".clcdc.bss"))) 
     buffer[320*240];

/* msleep

   only works after the timer has been initialized.

*/

static void msleep (int ms)
{
  unsigned long time = timer_read ();
	
  do {
  } while (timer_delta (time, timer_read ()) < ms);
}

/* cannot do scaling without __divsi3 */
//#define I(c,i) ((c)*(i)/255)
#define I(c,i) (c)

static void clcdc_init (void)
{
  {
    int i;
    for (i = 0; i < 320*240; ++i) {
      if (i > 3*(320*240)/4)
	buffer[i] = 0xffff;
      else if (i > 2*(320*240)/4)
	buffer[i] = I(0x1f,(i%240)*255/255)<<10;
      else if (i > 1*(320*240)/4)
	buffer[i] = I(0x1f,(i%240)*255/255)<<5;
      else if (i > 0*(320*240)/4)
	buffer[i] = I(0x1f,(i%240)*255/255)<<0;
    }
  }

  __REG (RCPC_PHYS + RCPC_CTRL) |= (1<<9); /* Unlock */
  __REG (RCPC_PHYS + RCPC_PCLKCTRL1) &= ~(1<<0);
  __REG (RCPC_PHYS + RCPC_AHBCLKCTRL) &= ~(1<<4);
  //  __REG (RCPC_PHYS + RCPC_LCDPRE) = (8>>1); 
  __REG (RCPC_PHYS + RCPC_LCDPRE) = (1>>1); 
  __REG (RCPC_PHYS + RCPC_CTRL) &= ~(1<<9); /* Lock */


  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_MUXCTL1),
		(3<<2)|(3<<0),
		(1<<2)|(1<<0));
  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_RESCTL1),
		(3<<2)|(3<<0),
		(0<<2)|(0<<0));

  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_MUXCTL19), 
		(3<<12)|(3<<8)|(3<<4)|(3<<2), 
		/* LCDVEEN, LCDVDDEN, LCDREV, LCDCLS */
		(1<<12)|(1<<8)|(2<<4)|(1<<2));
  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_RESCTL19), 
		(3<<12)|(3<<8)|(3<<4)|(3<<2), 
		(0<<12)|(0<<8)|(0<<4)|(0<<2));

  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_MUXCTL20), 
		(3<<14)|(3<<10)|(3<<6)|(3<<2)|(3<<0), 
		/* LCDPS, LCDDCLK, LCDHRLP, LCDSPS, LCDSPL */
		(1<<14)|(1<<10)|(2<<6)|(2<<2)|(2<<0)); 
  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_RESCTL20), 
		(3<<14)|(3<<10)|(3<<6)|(3<<2)|(3<<0), 
		(0<<14)|(0<<10)|(0<<6)|(0<<2)|(0<<0)); 
  
  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_MUXCTL21), 
		(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0), 
		(1<<10)|(1<<8)|(1<<6)|(1<<4)|(1<<2)|(1<<0));
  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_RESCTL21), 
		(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0), 
		(1<<10)|(1<<8)|(1<<6)|(1<<4)|(1<<2)|(1<<0));

  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_MUXCTL22),
		(3<<14)|(3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0), 
		(1<<14)|(1<<12)|(1<<10)|(1<<8)|(1<<6)|(1<<4)|(1<<2)|(1<<0)); 
  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_RESCTL22), 
		(3<<14)|(3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0), 
		(0<<14)|(0<<12)|(0<<10)|(0<<8)|(0<<6)|(0<<4)|(0<<2)|(0<<0)); 

  __REG(CLCDC_PHYS + CLCDC_TIMING0)   = 0x0e143c38;
  __REG(CLCDC_PHYS + CLCDC_TIMING1)   = 0x075f013f;
  __REG(CLCDC_PHYS + CLCDC_TIMING2)   = 0x00ef3006;
  __REG(CLCDC_PHYS + CLCDC_UPBASE)    = (unsigned long) buffer;
  __REG(CLCDC_PHYS + CLCDC_CTRL)      = 0x00010028;
  __REG(ALI_PHYS   + ALI_SETUP)	      = 0x00000efd;
  __REG(ALI_PHYS   + ALI_CTRL)	      = 0x00000013;
  __REG(ALI_PHYS   + ALI_TIMING1)     = 0x0000087d;
  __REG(ALI_PHYS   + ALI_TIMING2)     = 0x00009ad0;

  __REG(CLCDC_PHYS + CLCDC_CTRL)      |= (1<<0); /* Enable CLCDC */
  msleep (20);			/* Wait 20ms for digital signals  */
  __REG(CLCDC_PHYS + CLCDC_CTRL)      |= (1<<11); /* Apply power */
}

static void clcdc_release (void)
{
  __REG(CLCDC_PHYS + CLCDC_CTRL) &= ~(1<<11); /* Remove power */
  msleep (20);			/* Wait 20ms */
  __REG(CLCDC_PHYS + CLCDC_CTRL) &= ~(1<<0); /* Disable CLCDC controller */

				/*  Shutdown all the clocks */
  __REG (RCPC_PHYS + RCPC_CTRL) |= (1<<9); /* Unlock */
  __REG (RCPC_PHYS + RCPC_PCLKCTRL1) |= 1<<0;
  __REG (RCPC_PHYS + RCPC_AHBCLKCTRL) |= 1<<4;
  __REG (RCPC_PHYS + RCPC_CTRL) &= ~(1<<9); /* Lock */
}

static __service_7 struct service_d lpd79524_clcdc_service = {
  .init = clcdc_init,
  .release = clcdc_release,
};
