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
#include <png.h>
#include <command.h>
#include <error.h>

//#define USE_BORDER
//#define USE_FILL

#define BPP1	(0<<1)
#define BPP2	(1<<1)
#define BPP4	(2<<1)
#define BPP8	(3<<1)
#define BPP16	(4<<1)

#define NS_TO_CLOCK(ns,c)	((((ns)*((c)/1000) + (1000000 - 1))/1000000))
#define CLOCK_TO_DIV(e,c)	(((c) + (e) - 1)/(e))

#if defined (CONFIG_LCD_3_5_QVGA_20)
	/* Sharp PN LQ035Q7DB02 */
#define PANEL_NAME	"LCD 3.5\" QVGA"
#define PEL_CLOCK_EST	(6800000)     /* 4.5MHz-?-6.8MHz */
#define PEL_CLOCK_DIV	CLOCK_TO_DIV(PEL_CLOCK_EST, HCLK)
#define PEL_CLOCK	(HCLK/PEL_CLOCK_DIV)
#define PEL_WIDTH	(240)
#define PEL_HEIGHT	(320)
#define BITS_PER_PEL_2	BPP16
#define LEFT_MARGIN	(16)
#define RIGHT_MARGIN	(21)
#define TOP_MARGIN	(8)	/* 8 */
#define BOTTOM_MARGIN	(5)
#define HSYNC_WIDTH	(61)
#define VSYNC_WIDTH	NS_TO_CLOCK (60, PEL_CLOCK)
#endif

#if defined (CONFIG_LCD_5_7_QVGA_10)
	/* Sharp PN LQ057Q3DC02, QVGA mode */
#define PANEL_NAME	"LCD 5.7\" QVGA"
#define PEL_CLOCK_EST	(7000000)     /* ?-6.3MHz-7MHz */
#define PEL_CLOCK_DIV	CLOCK_TO_DIV(PEL_CLOCK_EST, HCLK)
#define PEL_CLOCK	(HCLK/PEL_CLOCK_DIV)
#define PEL_WIDTH	(320)
#define PEL_HEIGHT	(240)
#define BITS_PER_PEL_2	BPP16
#define LEFT_MARGIN	(21)
#define RIGHT_MARGIN	(15)
#define TOP_MARGIN	(7)	/* 7 */
#define BOTTOM_MARGIN	(5)
#define HSYNC_WIDTH	(96)	/* 2-96-200 clocks */
#define VSYNC_WIDTH	(1)	/* 2-?-34 lines */
#define INVERT_HSYNC
#define INVERT_VSYNC
#endif

#if defined (CONFIG_LCD_6_4_VGA_10)
	/* Sharp PN LQ64D343 */
#define PANEL_NAME	"LCD 6.4\" VGA"
#define PEL_CLOCK_EST	(28330000)     /* ?-25.18MHz-28.33MHz */
#define PEL_CLOCK_DIV	CLOCK_TO_DIV(PEL_CLOCK_EST, HCLK)
#define PEL_CLOCK	(HCLK/PEL_CLOCK_DIV)
#define PEL_WIDTH	(640)
#define PEL_HEIGHT	(480)
#define BITS_PER_PEL_2	BPP16
#define LEFT_MARGIN	(21)
#define RIGHT_MARGIN	(15)
#define TOP_MARGIN	(34)	/* 34 */
#define BOTTOM_MARGIN	(5)
#define HSYNC_WIDTH	(96)	/* 2-96-200 clocks */
#define VSYNC_WIDTH	(1)	/* 2-?-34 lines */
#define INVERT_HSYNC
#define INVERT_VSYNC
#endif

#if defined (CONFIG_LCD_10_4_VGA_10)
	/* Sharp PN LQ10D368 */
#define PANEL_NAME	"LCD 10.4\" VGA"
#define PEL_CLOCK_EST	(28330000)     /* ?-25.18MHz-28.33MHz */
#define PEL_CLOCK_DIV	CLOCK_TO_DIV(PEL_CLOCK_EST, HCLK)
#define PEL_CLOCK	(HCLK/PEL_CLOCK_DIV)
#define PEL_WIDTH	(640)
#define PEL_HEIGHT	(480)
#define BITS_PER_PEL_2	BPP16
#define LEFT_MARGIN	(21)
#define RIGHT_MARGIN	(15)
#define TOP_MARGIN	(34)	/* 34 */
#define BOTTOM_MARGIN	(5)
#define HSYNC_WIDTH	(96)	/* 2-96-200 clocks */
#define VSYNC_WIDTH	(1)	/* 2-?-34 lines */
#define INVERT_HSYNC
#define INVERT_VSYNC
#endif

#if defined (CONFIG_LCD_12_1_SVGA_10)
	/* Sharp PN LQ121S1DG41, was LQ121S1DG31 */
#define PANEL_NAME	"LCD 12.1\" SVGA"
/* *** FIXME: this target frequency range isn't achievable with HCLK
   *** at 99993900 Hz. */
#define PEL_CLOCK_EST	(42000000)     /* 35MHz-40MHz-42MHz */
#define PEL_CLOCK_DIV	CLOCK_TO_DIV(PEL_CLOCK_EST, HCLK)
#define PEL_CLOCK	(HCLK/PEL_CLOCK_DIV)
#define PEL_WIDTH	(800)
#define PEL_HEIGHT	(600)
#define BITS_PER_PEL_2	BPP16
#define LEFT_MARGIN	(86)
#define RIGHT_MARGIN	(15)
#define TOP_MARGIN	(23)	/* 23 */
#define BOTTOM_MARGIN	(5)
#define HSYNC_WIDTH	(128)	/* 2-128-200 clocks */
#define VSYNC_WIDTH	(1)	/* 2-4-6 lines */
#define INVERT_HSYNC
#define INVERT_VSYNC
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
#define IVS	(1<<11)
#define PCD(v)	(((v) - 2) & 0x1f)
#define WATERMARK (1<<16)
#define PWR	(1<<11)
#define BEPO	(1<<10)
#define BEBO	(1<<9)
#define BGR	(1<<8)
#define DUAL	(1<<7)
#define TFT	(1<<5)
#define BW	(1<<4)
#define LCDEN	(1<<0)


static unsigned short __xbss(clcdc) buffer[PEL_WIDTH*PEL_HEIGHT];

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
#define I(c,i) ((c)*(i)/255)
//#define I(c,i) (c)

#define RGB(r,g,b) ( (((r) & 0xf8) >>  3)\
		    |(((g) & 0xf8) <<  2)\
		    |(((b) & 0xf8) <<  7))


#define RGBI(r,g,b,i) ( (((r) & 0xf8) >>  3)\
		       |(((g) & 0xf8) <<  2)\
		       |(((b) & 0xf8) <<  7)\
		       |(((i) & 1) << 15))

extern int determine_arch_number (void); /* *** HACK */

static void clcdc_init (void)
{
  printf ("clcdc: initializing %s\n", PANEL_NAME);
  printf ("  clk %d/%d->%d (%d)  sync (%d %d)  porch (%d %d %d %d)\n",
	  HCLK, PEL_CLOCK_DIV, PEL_CLOCK, PEL_CLOCK_EST,
	  HSYNC_WIDTH, VSYNC_WIDTH,
	  LEFT_MARGIN, RIGHT_MARGIN, TOP_MARGIN, BOTTOM_MARGIN);

#if defined (USE_FILL)

  {
    int i;
    for (i = 0; i < PEL_WIDTH*PEL_HEIGHT; ++i) {
      if (i >= 3*(PEL_WIDTH*PEL_HEIGHT)/4)
	buffer[i] = 0xefff;
      else if (i >= 2*(PEL_WIDTH*PEL_HEIGHT)/4)
	buffer[i] = RGB(0,0,(i%PEL_WIDTH)*255/PEL_WIDTH);
      else if (i >= 1*(PEL_WIDTH*PEL_HEIGHT)/4)
	buffer[i] = RGB(0,(i%PEL_WIDTH)*255/PEL_WIDTH,0);
      else if (i >= 0*(PEL_WIDTH*PEL_HEIGHT)/4)
	buffer[i] = RGB((i%PEL_WIDTH)*255/PEL_WIDTH,0,0);
    }
  }

#if defined (USE_BORDER)

  {
    int row;
    //    printf ("rgb value %x\n", RGB (0x80, 0x80, 0x80));
    for (row = 0; row < PEL_HEIGHT; ++row) {
      buffer[row*PEL_WIDTH] = RGB (0x80, 0x80, 0x80);
      buffer[row*PEL_WIDTH + 1] = 0;
      buffer[row*PEL_WIDTH + PEL_WIDTH - 1] = RGB (0x80, 0x80, 0x80);
      buffer[row*PEL_WIDTH + PEL_WIDTH - 2] = 0;
      //      buffer[row*PEL_WIDTH] = 0xffff;
    }
  }

#endif
#endif

  /* Note that PE4 must be driven high on the LPD7A404 to prevent the
     CPLD JTAG chain from crashing the board.  */

  GPIO_PINMUX |= (1<<1) | (1<<0); /* LCDVD[15:4] */

  CLCDC_TIMING0 = HBP (LEFT_MARGIN) | HFP (RIGHT_MARGIN) | HSW (HSYNC_WIDTH)
    | PPL (PEL_WIDTH);
  CLCDC_TIMING1 = VBP (TOP_MARGIN) | VFP (BOTTOM_MARGIN) | VSW (VSYNC_WIDTH) 
    | LPP (PEL_HEIGHT);
  CLCDC_TIMING2   = CPL | IPC
#if defined (INVERT_HSYNC)
    | IHS
#endif
#if defined (INVERT_VSYNC)
    | IVS
#endif
    | PCD (PEL_CLOCK_DIV);

  CLCDC_UPBASE    = (unsigned long) buffer;
  CLCDC_CTRL      = WATERMARK | TFT | BITS_PER_PEL_2;

#if defined (CONFIG_LCD_3_5_QVGA_20)
# if defined (CONFIG_ARCH_LH7A400)
  HRTFTC_SETUP   = 0x00002efd;
  HRTFTC_CON     = 0x00000003;
  HRTFTC_TIMING1 = 0x0000087d;
  HRTFTC_TIMING2 = 0x00009ad0;
# endif

# if defined (CONFIG_ARCH_LH7A404)
  ALI_SETUP   = 0x00002efd;
  ALI_CONTROL = 0x00000003;
  ALI_TIMING1 = 0x0000087d;
  ALI_TIMING2 = 0x00009ad0;
# endif

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

#if defined (CONFIG_ARCH_LH7A400)
  HRTFTC_SETUP &= ~(1<<13); /* Disable HRTFT controller */
#endif

#if defined (CONFIG_ARCH_LH7A404)
  ALI_SETUP &= ~(1<<13); /* Disable ALI */
#endif
}

static __service_7 struct service_d lpd7a40x_clcdc_service = {
  .init    = clcdc_init,
  .release = clcdc_release,
};



#if defined (CONFIG_CMD_SPLASH) || 1

int cmd_splash (int argc, const char** argv)
{
  int result;
  struct descriptor_d d;
  void* pv;
  struct png_header hdr;
  unsigned short* ps = buffer;
  int i;
  int j;

  if (argc != 2)
    return ERROR_PARAM;

  if (   (result = parse_descriptor (argv[1], &d))
      || (result = open_descriptor (&d))) {
    printf ("Unable to open source %s\n", argv[1]);
    goto fail_early;
  }

  //  printf ("open_pngr %s\n", argv[1]);
  pv = open_pngr (&d);
  if (pv == NULL)
    goto fail_early;

//  printf ("read_ihdr\n");
  if (read_pngr_ihdr (pv, &hdr))
    goto fail_close;

  if (hdr.bit_depth != 8)
    goto fail_close;


  memset (buffer, 0, sizeof (buffer));

  {
    for (i = hdr.height; i--; ps += PEL_WIDTH - hdr.width) {
      const unsigned char* pb = read_pngr_row (pv);
      if (pb == NULL) {
	printf ("%s: read failed at %d\n", __FUNCTION__, i);
	goto fail_close;
      }
      switch (hdr.color_type) {
      case 2:			/* RGB */
	for (j = 0; j < hdr.width; ++j, ++ps)
	  *ps = 0
	    +   ((pb[j*3    ] & 0xf8) >> 3)
	    +   ((pb[j*3 + 1] & 0xf8) << 2)
	    +   ((pb[j*3 + 2] & 0xf8) << 7)
	    ;
	break;
      case 6:			/* RGBA */
	for (j = 0; j < hdr.width; ++j, ++ps)
	  *ps = ((pb[j*4    ] & 0xf8) >> 3)
	    +   ((pb[j*4 + 1] & 0xf8) << 2)
	    +   ((pb[j*4 + 2] & 0xf8) << 7);
	
	break;
      }
    }
  }

 fail_close:
  close_png (pv);

 fail_early:
  close_descriptor (&d);

  return result;
}


static __command struct command_d c_splash = {
  .command = "splash",
  .description = "display image on the LCD",
  .func = cmd_splash,
  COMMAND_HELP(
"splash"
" SRC\n"
"  Display the image from SRC on the LCD screen.\n"
"  The image is read from region SRC.\n\n"
"  e.g.  splash fat://1/logo.png\n"
  )
};

#endif
