/* clcdc.c
     $Id$

   written by Marc Singer
   7 Dec 2004

   Copyright (C) 2004 Marc Singer

   -----------
   DESCRIPTION
   -----------

   Test code for the lcd controller.

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

extern int determine_arch_number (void); /* *** HACK */

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

  __REG(GPIO_PHYS + GPIO_PINMUX) |= (1<<1) | (1<<0); /* LCDVD[15:4] */
  __REG(GPIO_PHYS + GPIO_PADD)	 |= (1<<2);	     /* PA2 output */
  __REG(GPIO_PHYS + GPIO_PAD)	 |= (1<<2);	     /* PA2 high */

  __REG(CLCDC_PHYS + CLCDC_TIMING0)   = 0x0e143c38;
  __REG(CLCDC_PHYS + CLCDC_TIMING1)   = 0x075f013f;
  __REG(CLCDC_PHYS + CLCDC_TIMING2)   = 0x00ef300e;
  __REG(CLCDC_PHYS + CLCDC_UPBASE)    = (unsigned long) buffer;
  __REG(CLCDC_PHYS + CLCDC_CTRL)      = 0x00010028;

  __REG(HRTFTC_PHYS + HRTFTC_SETUP)   = 0x00002efd;
  __REG(HRTFTC_PHYS + HRTFTC_CON)     = 0x00000003;
  __REG(HRTFTC_PHYS + HRTFTC_TIMING1) = 0x0000087d;
  __REG(HRTFTC_PHYS + HRTFTC_TIMING2) = 0x00009ad0;

  __REG(CLCDC_PHYS + CLCDC_CTRL)      |= (1<<0); /* Enable CLCDC */
  msleep (20);			/* Wait 20ms for digital signals  */
  __REG(CLCDC_PHYS + CLCDC_CTRL)      |= (1<<11); /* Apply power */

  if (determine_arch_number () == 389)
    CPLD_CONTROL |= CPLD_CONTROL_LCD_VEEEN;
}

static void clcdc_release (void)
{
  if (determine_arch_number () == 389)
    CPLD_CONTROL &= ~CPLD_CONTROL_LCD_VEEEN;

  __REG(CLCDC_PHYS + CLCDC_CTRL) &= ~(1<<11); /* Remove power */
  msleep (20);			/* Wait 20ms */
  __REG(CLCDC_PHYS + CLCDC_CTRL) &= ~(1<<0); /* Disable CLCDC controller */

  __REG(HRTFTC_PHYS + HRTFTC_SETUP) &= ~(1<<13); /* Disable HRTFT controller */

//  printf ("clcdc %x %x\r\n", 
//	  __REG(CLCDC_PHYS + CLCDC_CTRL), __REG(HRTFTC_PHYS + HRTFTC_SETUP));
}

static __service_7 struct service_d lpd7a40x_clcdc_service = {
  .init    = clcdc_init,
  .release = clcdc_release,
};
