/* clcdc.c
     $Id$

   written by Marc Singer
   7 Dec 2004

   Copyright (C) 2004 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA.

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

//#define USE_BORDER

#define BPP1	(0<<1)
#define BPP2	(1<<1)
#define BPP4	(2<<1)
#define BPP8	(3<<1)
#define BPP16	(4<<1)

#if defined (CONFIG_LCD_3_5_QVGA_20)
	/* Sharp PN LQ035Q7DB02 */
#define PEL_WIDTH	(240)
#define PEL_HEIGHT	(320)
#define BITS_PER_PEL_2	BPP16
#define LEFT_MARGIN	(21)
#define RIGHT_MARGIN	(15)
#define TOP_MARGIN	(96)
#define BOTTOM_MARGIN	(8)
#define HSYNC_WIDTH	(61)
#define VSYNC_WIDTH	(1)
#endif

#if defined (CONFIG_LCD_5_7_QVGA_10)
	/* Sharp PN LQ057Q3DC02 */
#define PEL_WIDTH	(320)
#define PEL_HEIGHT	(240)
#define BITS_PER_PEL_2	BPP16
#define LEFT_MARGIN	(21)
#define RIGHT_MARGIN	(15)
#define TOP_MARGIN	(7)
#define BOTTOM_MARGIN	(5)
#define HSYNC_WIDTH	(16)
#define VSYNC_WIDTH	(1)
#endif

#define HBP(v)	((((v) - 1) & 0xff)<<24)
#define HFP(v)	((((v) - 1) & 0xff)<<16)
#define HSW(v)	((((v) - 1) & 0xff)<<8)
#define PPL(v)	((((v)/16 - 1) & 0x7f)<<2)

#define VBP(v)	(((v) & 0xff)<<24)
#define VFP(v)	(((v) & 0xff)<<16)
#define VSW(v)	((((v) - 1) & 0x3f)<<8)
#define LPP(v)	(((v) - 1) & 0x3ff)

#define BCD	(1<<26)		/* Bypass pixel clock divider */
#define CPL	((PEL_WIDTH - 1) & 0x3ff)<<16
#define IOE	(1<<14)
#define IPC	(1<<13)
#define IHS	(1<<12)
#define PCD(v)	((v) & 0x1f)
#define WATERMARK (1<<16)
#define PWR	(1<<11)
#define BEPO	(1<<10)
#define BEBO	(1<<9)
#define BGR	(1<<8)
#define DUAL	(1<<7)
#define TFT	(1<<5)
#define BW	(1<<4)
#define LCDEN	(1<<0)


static unsigned short __attribute__((section(".clcdc.bss"))) 
     buffer[PEL_WIDTH*PEL_HEIGHT];

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

#define RGB(r,g,b) ( (((r) & 0xf8) >>  3)\
		    |(((g) & 0xf8) <<  2)\
		    |(((b) & 0xf8) <<  7))

extern int determine_arch_number (void); /* *** HACK */

static void clcdc_init (void)
{
  {
    int i;
    for (i = 0; i < PEL_WIDTH*PEL_HEIGHT; ++i) {
      if (i > 3*(PEL_WIDTH*PEL_HEIGHT)/4)
	buffer[i] = 0xffff;
      else if (i > 2*(PEL_WIDTH*PEL_HEIGHT)/4)
	buffer[i] = I(0x1f,(i%PEL_WIDTH)*255/255)<<10;
      else if (i > 1*(PEL_WIDTH*PEL_HEIGHT)/4)
	buffer[i] = I(0x1f,(i%PEL_WIDTH)*255/255)<<5;
      else if (i > 0*(PEL_WIDTH*PEL_HEIGHT)/4)
	buffer[i] = I(0x1f,(i%PEL_WIDTH)*255/255)<<0;
    }
  }

#if defined (USE_BORDER)

  {
    int row;
    printf ("rgb value %x\n", RGB (0x80, 0x80, 0x80));
    for (row = 0; row < PEL_HEIGHT; ++row) {
      buffer[row*PEL_WIDTH] = RGB (0x80, 0x80, 0x80);
      buffer[row*PEL_WIDTH + 1] = 0;
      buffer[row*PEL_WIDTH + PEL_WIDTH - 1] = RGB (0x80, 0x80, 0x80);
      buffer[row*PEL_WIDTH + PEL_WIDTH - 2] = 0;
      //      buffer[row*PEL_WIDTH] = 0xffff;
    }
  }

#endif

  GPIO_PINMUX |= (1<<1) | (1<<0); /* LCDVD[15:4] */

  CLCDC_TIMING0 = HBP (LEFT_MARGIN) | HFP (RIGHT_MARGIN) | HSW (HSYNC_WIDTH)
    | PPL (PEL_WIDTH);
  CLCDC_TIMING1 = VBP (TOP_MARGIN) | VFP (BOTTOM_MARGIN) | VSW (VSYNC_WIDTH) 
    | LPP (PEL_HEIGHT);

#if defined (CONFIG_LCD_3_5_QVGA_20)

//  __REG(CLCDC_PHYS + CLCDC_TIMING0)   = 0x0e143c38;
//  __REG(CLCDC_PHYS + CLCDC_TIMING1)   = 0x075f013f;
//  __REG(CLCDC_PHYS + CLCDC_TIMING2)   = 0x00ef300e;
//  __REG(CLCDC_PHYS + CLCDC_CTRL)      = 0x00010028;

  CLCDC_TIMING2   = CPL | IPC | IHS | PCD (0xe);

  CLCDC_UPBASE    = (unsigned long) buffer;
  CLCDC_CTRL      = WATERMARK | TFT | BITS_PER_PEL_2;

  HRTFTC_SETUP   = 0x00002efd;
  HRTFTC_CON     = 0x00000003;
  HRTFTC_TIMING1 = 0x0000087d;
  HRTFTC_TIMING2 = 0x00009ad0;

#endif

#if defined (CONFIG_LCD_5_7_QVGA_10)

  CLCDC_TIMING2   = CPL | IPC | IHS | PCD (0xe);

  CLCDC_UPBASE    = (unsigned long) buffer;
  CLCDC_CTRL      = WATERMARK | TFT | BITS_PER_PEL_2;

#endif

  CLCDC_CTRL      |= (1<<0); /* Enable CLCDC */
  msleep (20);			/* Wait 20ms for digital signals  */
  CLCDC_CTRL      |= (1<<11); /* Apply power */

  if (determine_arch_number () == 389)		/* lh7400 */
    CPLD_CONTROL |= CPLD_CONTROL_LCD_VEEEN;
  if (determine_arch_number () == 390) {	/* lh7a404 */
    GPIO_PCDD |= (1<<3);
    GPIO_PCD  |= (1<<3);
  }

//  printf ("CLDC\n");
//  dump ((void*) HRTFTC_PHYS, 16, HRTFTC_PHYS);
//  dump ((void*) CLCDC_PHYS, 0x40, CLCDC_PHYS);
}

static void clcdc_release (void)
{
  if (determine_arch_number () == 389)		/* lh7a400 */
    CPLD_CONTROL &= ~CPLD_CONTROL_LCD_VEEEN;
  if (determine_arch_number () == 390)		/* lh7a404  */
    GPIO_PCD  &= ~(1<<3);

  CLCDC_CTRL &= ~(1<<11); /* Remove power */
  msleep (20);			/* Wait 20ms */
  CLCDC_CTRL &= ~(1<<0); /* Disable CLCDC controller */

  HRTFTC_SETUP &= ~(1<<13); /* Disable HRTFT controller */

//  printf ("clcdc %x %x\n", 
//	  __REG(CLCDC_PHYS + CLCDC_CTRL), __REG(HRTFTC_PHYS + HRTFTC_SETUP));
}

static __service_7 struct service_d lpd7a40x_clcdc_service = {
  .init    = clcdc_init,
  .release = clcdc_release,
};
