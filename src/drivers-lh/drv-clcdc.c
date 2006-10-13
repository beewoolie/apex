/* drv-clcdc.c

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
   along with this program; if not, write to the Free Software4096
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA.

   -----------
   DESCRIPTION
   -----------

   Sharp LH CLCD controller initialization, splash, and test support.

   This code needs a serious overhaul to remove the ifdef's from
   within the bodies of the functions.  Macros for the custom
   functions please.

   Splash Command
   --------------

   This code is, more or less, complete.  It doesn't read from tftp
   and it probably doesn't read from a filesystem either.  That has to
   be fixed.  It also doesn't double-buffer which would make the
   display snappier.  However, the image decode is robust.


   Target Macros
   -------------

   Some of the functions of this driver are target specific.  Instead
   of including all of the ifdefs within the body of the driver, each
   target supplies these functions as macros in the header
   mach/drv-clcdc.h.  A target may choose to not define a macro in
   which case it will be a NOP.  The macros are as follows.

   DRV_CLCDC_SETUP			- Setup clocks and muxing.
   DRV_CLCDC_POWER_ENABLE		- Enable power to LCD panel
   DRV_CLCDC_BACKLIGHT_ENABLE		- Enable power backlight power
   DRV_CLCDC_BACKLIGHT_DISABLE		- Disable backlight power
   DRV_CLCDC_DISABLE			- Extra LCD controller disabling
   DRV_CLCDC_POWER_DISABLE		- Disable power to LCD panel
   DRV_CLCDC_RELEASE			- Release clocks


   Nomenclature -- Porches
   -----------------------

   Front porches and back porches refer to the sync pulse as opposed
   to the displayed frame.  The front porch is in front of the sync
   pulse, the back porch follows it.

*/

#include <config.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <apex.h>
#include <mach/hardware.h>
#include <png.h>
#include <command.h>
#include <error.h>
#include <asm/mmu.h>
#include <mach/drv-clcdc.h>
#include <debug_ll.h>

#if ! defined (DRV_CLCDC_SETUP)
# define DRV_CLCDC_SETUP
#endif
#if ! defined (DRV_CLCDC_POWER_ENABLE)
# define DRV_CLCDC_POWER_ENABLE
#endif
#if ! defined (DRV_CLCDC_BACKLIGHT_ENABLE)
# define DRV_CLCDC_BACKLIGHT_ENABLE
#endif
#if ! defined (DRV_CLCDC_BACKLIGHT_DISABLE)
# define DRV_CLCDC_BACKLIGHT_DISABLE
#endif
#if ! defined (DRV_CLCDC_DISABLE)
# define DRV_CLCDC_DISABLE
#endif
#if ! defined (DRV_CLCDC_POWER_DISABLE)
# define DRV_CLCDC_POWER_DISABLE
#endif
#if ! defined (DRV_CLCDC_RELEASE)
# define DRV_CLCDC_RELEASE
#endif
#if ! defined (DRV_CLCDC_WAKE)
# define DRV_CLCDC_WAKE
#endif
#if ! defined (DRV_CLCDC_SLEEP)
# define DRV_CLCDC_SLEEP
#endif

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
#define BIT_DEPTH	(16)
#define BITS_PER_PEL_2	BPP16
#define LEFT_MARGIN	(16)
#define RIGHT_MARGIN	(21)
#define TOP_MARGIN	(8)	/* 8 */
#define BOTTOM_MARGIN	(5)
#define HSYNC_WIDTH	(61)
#define VSYNC_WIDTH	NS_TO_CLOCK (60, PEL_CLOCK)
#define INVERT_PIXEL_CLOCK
#endif

#if defined (CONFIG_LCD_5_7_QVGA_10)
	/* Sharp PN LQ057Q3DC02, QVGA mode */
#define PANEL_NAME	"LCD 5.7\" QVGA"
#define PEL_CLOCK_EST	(7000000)     /* ?-6.3MHz-7MHz */
#define PEL_CLOCK_DIV	CLOCK_TO_DIV(PEL_CLOCK_EST, HCLK)
#define PEL_CLOCK	(HCLK/PEL_CLOCK_DIV)
#define PEL_WIDTH	(320)
#define PEL_HEIGHT	(240)
#define BIT_DEPTH	(16)
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
/* The full horozontal cycle (Th) is clock/770/800/900. */
/* The full vertical   cycle (Tv) is line/515/525/560. */
#define PANEL_NAME	"LCD 6.4\" VGA"
#define PEL_CLOCK_EST	(28330000)     /* ?-25.18MHz-28.33MHz */
#define PEL_CLOCK_DIV	CLOCK_TO_DIV(PEL_CLOCK_EST, HCLK)
#define PEL_CLOCK	(HCLK/PEL_CLOCK_DIV)
#define PEL_WIDTH	(640)
#define PEL_HEIGHT	(480)
#define BIT_DEPTH	(16)
#define BITS_PER_PEL_2	BPP16

#define LEFT_MARGIN	(32)
#define RIGHT_MARGIN	(800-32-640-96)
#define TOP_MARGIN	(32)	/* 34 */
#define BOTTOM_MARGIN	(540-32-480-2)
#define HSYNC_WIDTH	(96)	/* 2-96-200 clocks */
#define VSYNC_WIDTH	(2)	/* 2-?-34 lines */

#define INVERT_HSYNC
#define INVERT_VSYNC
#endif

#if defined (CONFIG_LCD_10_4_VGA_10)
	/* Sharp PN LQ10D368 */
/* The full horozontal cycle (Th) is clock/750/800/900. */
/* The full vertical   cycle (Tv) is line/515/525/560. */
#define PANEL_NAME	"LCD 10.4\" VGA"
#define PEL_CLOCK_EST	(28330000)     /* ?-25.18MHz-28.33MHz */
#define PEL_CLOCK_DIV	CLOCK_TO_DIV(PEL_CLOCK_EST, HCLK)
#define PEL_CLOCK	(HCLK/PEL_CLOCK_DIV)
#define PEL_WIDTH	(640)
#define PEL_HEIGHT	(480)
#define BIT_DEPTH	(16)
#define BITS_PER_PEL_2	BPP16
#define LEFT_MARGIN	(21)
#define RIGHT_MARGIN	(800-21-640-96)
#define TOP_MARGIN	(34)			/* lines/34 (480 line mode)*/
#define BOTTOM_MARGIN	(540-34-480-2)
#define HSYNC_WIDTH	(96)			/* clocks/2/96/200 */
#define VSYNC_WIDTH	(2)			/* lines/1/?/34 */
#define INVERT_HSYNC
#define INVERT_VSYNC
#endif

#if defined (CONFIG_LCD_12_1_SVGA_10)
	/* Sharp PN LQ121S1DG41, was LQ121S1DG31 */
#define PANEL_NAME	"LCD 12.1\" SVGA"
/* *** FIXME: this target frequency range isn't achievable with HCLK
   *** at 99993900 Hz. */
#define PEL_CLOCK_EST	(25000000)     /* 35MHz-40MHz-42MHz */
#define PEL_CLOCK_DIV	CLOCK_TO_DIV(PEL_CLOCK_EST, HCLK)
#define PEL_CLOCK	(HCLK/PEL_CLOCK_DIV)
#define PEL_WIDTH	(800)
#define PEL_HEIGHT	(600)
#define BIT_DEPTH	(16)
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

#if defined (CONFIG_LCD_NL2432HC22_40A)
	/* NEC QVGA 2432HC22-40A 240x320 8.9cm*/
/* The full horozontal cycle (Th) is clock/?/256/?. */
/* The full vertical   cycle (Tv) is line/?/324/?. */
#define PANEL_NAME	"NEC LCD 8.9cm Portrait QVGA"
#define PEL_CLOCK_EST	(5*1000*1000)     /* 5MHz */
#define PEL_CLOCK_DIV	CLOCK_TO_DIV(PEL_CLOCK_EST, HCLK)
#define PEL_CLOCK	(HCLK/PEL_CLOCK_DIV)
#define PEL_WIDTH	(240)
#define PEL_HEIGHT	(320)
#define BIT_DEPTH	(16)
#define BITS_PER_PEL_2	BPP16
#define LEFT_MARGIN	(4)
#define RIGHT_MARGIN	(256-8-240-4)
#define TOP_MARGIN	(1)			/* lines/?*/
#define BOTTOM_MARGIN	(324-2-320-1)
#define HSYNC_WIDTH	(8)			/* clocks/?/8/? */
#define VSYNC_WIDTH	(2)			/* lines/?/1/? */

#define INVERT_HSYNC
#define INVERT_VSYNC
#endif

#if defined (CONFIG_LCD_OSD035TTEA1)
	/* Sharp QVGA 2432HC22-40A 320x240 70.08mmx52.56mm*/
/* The full horozontal cycle (Th) is clock/326/440/472. */
/* The full vertical   cycle (Tv) is line/246/264/282. */
#define PANEL_NAME	"Sharp LCD Landscape QVGA"
#define PEL_CLOCK_EST	(8*1000*1000)		/* MHz/4/8/8.65 */
#define PEL_CLOCK_DIV	CLOCK_TO_DIV(PEL_CLOCK_EST, HCLK)
#define PEL_CLOCK	(HCLK/PEL_CLOCK_DIV)
#define PEL_WIDTH	(320)
#define PEL_HEIGHT	(240)
#define BIT_DEPTH	(16)
#define BITS_PER_PEL_2	BPP16
#define LEFT_MARGIN	(42)			/* clock/2/42/256 */
#define RIGHT_MARGIN	(440-42-320-38)		/* clock/2/40.256 */
#define TOP_MARGIN	(8)			/* lines/2/8/14 */
#define BOTTOM_MARGIN	(264-8-240-8)		/* lines/2/8/14 */
#define HSYNC_WIDTH	(38)			/* clocks/2/38/256 */
#define VSYNC_WIDTH	(8)			/* lines/2/8/14 */

#define INVERT_HSYNC
#define INVERT_VSYNC
#endif

#if defined (CONFIG_LCD_LQ036Q1DA01)
	/* Sharp QVGA LQ036Q1DA01 320x240 9.1cm w/ASIC */
#define PANEL_NAME	"Sharp LCD Landscape QVGA w/ASIC"

#define PANEL_TIMING0	(0x140a114c)
#define PANEL_TIMING1	(0x040404ef)
#define PANEL_TIMING2	(0x013f3016)
#define PANEL_CONTROL	(0x00010929)

#define PANEL_ALI_SETUP		(0x33fd)
#define PANEL_ALI_CONTROL	(0x0003)
#define PANEL_ALI_TIMING1	(0x082d)
#define PANEL_ALI_TIMING2	(0x420d)

#define P_PEL_CLOCK_DIV		((PANEL_TIMING2 & 0x1f) + 2)
//#define P_PEL_WIDTH		((((PANEL_TIMING0 >> 2) & 0x7f) + 1)*16)
#define P_PEL_WIDTH		(((PANEL_TIMING2 >> 16) & 0x3ff) + 1)
#define P_PEL_HEIGHT		((PANEL_TIMING1 & 0x1ff) + 1)
#define P_LEFT_MARGIN		(((PANEL_TIMING0 >> 24) & 0xff) + 1)
#define P_RIGHT_MARGIN		(((PANEL_TIMING0 >> 16) & 0xff) + 1)
#define P_TOP_MARGIN		((PANEL_TIMING1 >> 24) & 0xff)
#define P_BOTTOM_MARGIN		((PANEL_TIMING1 >> 16) & 0xff)

#define P_HSYNC_WIDTH		(((PANEL_TIMING0 >> 8) & 0x7f) + 1)
#define P_VSYNC_WIDTH		(((PANEL_TIMING1 >> 10) & 0x3f) + 1)

#define P_INVERT_PIXEL_CLOCK	(PANEL_TIMING2  & (1<<13))
#define P_INVERT_HSYNC		(PANEL_TIMING2  & (1<<12))
#define P_INVERT_VSYNC		(PANEL_TIMING2  & (1<<11))

//#define PEL_CLOCK_EST	(12*1000*1000)		/* MHz/4.5/12/? */
//#define PEL_CLOCK_DIV	CLOCK_TO_DIV(PEL_CLOCK_EST, HCLK)
#define PEL_CLOCK_DIV	P_PEL_CLOCK_DIV
#define PEL_CLOCK_EST	PEL_CLOCK_DIV
#define PEL_CLOCK	(HCLK/PEL_CLOCK_DIV)
#define PEL_WIDTH	P_PEL_WIDTH
#define PEL_HEIGHT	P_PEL_HEIGHT
#define BIT_DEPTH	16
#define BITS_PER_PEL_2	BPP16
#define LEFT_MARGIN	P_LEFT_MARGIN
#define RIGHT_MARGIN	P_RIGHT_MARGIN
#define TOP_MARGIN	P_TOP_MARGIN
#define BOTTOM_MARGIN	P_BOTTOM_MARGIN
#define HSYNC_WIDTH	P_HSYNC_WIDTH
#define VSYNC_WIDTH	P_VSYNC_WIDTH

# if P_INVERT_PIXEL_CLOCK
#  define INVERT_PIXEL_CLOCK
# endif

# if P_INVERT_HSYNC
#  define INVERT_HSYNC
# endif

# if P_INVERT_VSYNC
#  define INVERT_VSYNC
# endif

#endif

#if defined (CONFIG_LCD_LQ035Q7DH06)
	/* Sharp QVGA LQ035Q7DH06 240x320 */
#define PANEL_NAME	"Sharp LCD Portrait QVGA w/ASIC"

#define PANEL_TIMING0	(0x480f203c)
#define PANEL_TIMING1	(0x080f153f)
#define PANEL_TIMING2	(0x271f1806)
#define PANEL_CONTROL	(0x00010829)

#define PANEL_ALI_SETUP		(0x27f0)
#define PANEL_ALI_CONTROL	(0x0000)
#define PANEL_ALI_TIMING1	(0x0000)
#define PANEL_ALI_TIMING2	(0x0000)

#define P_PEL_CLOCK_DIV		((PANEL_TIMING2 & 0x1f) + 2)
//#define P_PEL_WIDTH		((((PANEL_TIMING0 >> 2) & 0x7f) + 1)*16)
#define P_PEL_WIDTH		(((PANEL_TIMING2 >> 16) & 0x3ff) + 1)
#define P_PEL_HEIGHT		((PANEL_TIMING1 & 0x1ff) + 1)
#define P_LEFT_MARGIN		(((PANEL_TIMING0 >> 24) & 0xff) + 1)
#define P_RIGHT_MARGIN		(((PANEL_TIMING0 >> 16) & 0xff) + 1)
#define P_TOP_MARGIN		((PANEL_TIMING1 >> 24) & 0xff)
#define P_BOTTOM_MARGIN		((PANEL_TIMING1 >> 16) & 0xff)

#define P_HSYNC_WIDTH		(((PANEL_TIMING0 >> 8) & 0x7f) + 1)
#define P_VSYNC_WIDTH		(((PANEL_TIMING1 >> 10) & 0x3f) + 1)

#define P_INVERT_PIXEL_CLOCK	(PANEL_TIMING2  & (1<<13))
#define P_INVERT_HSYNC		(PANEL_TIMING2  & (1<<12))
#define P_INVERT_VSYNC		(PANEL_TIMING2  & (1<<11))

//#define PEL_CLOCK_EST	(12*1000*1000)		/* MHz/4.5/12/? */
//#define PEL_CLOCK_DIV	CLOCK_TO_DIV(PEL_CLOCK_EST, HCLK)
#define PEL_CLOCK_DIV	P_PEL_CLOCK_DIV
#define PEL_CLOCK_EST	PEL_CLOCK_DIV
#define PEL_CLOCK	(HCLK/PEL_CLOCK_DIV)
#define PEL_WIDTH	P_PEL_WIDTH
#define PEL_HEIGHT	P_PEL_HEIGHT
#define BIT_DEPTH	16
#define BITS_PER_PEL_2	BPP16
#define LEFT_MARGIN	P_LEFT_MARGIN
#define RIGHT_MARGIN	P_RIGHT_MARGIN
#define TOP_MARGIN	P_TOP_MARGIN
#define BOTTOM_MARGIN	P_BOTTOM_MARGIN
#define HSYNC_WIDTH	P_HSYNC_WIDTH
#define VSYNC_WIDTH	P_VSYNC_WIDTH

# if P_INVERT_PIXEL_CLOCK
#  define INVERT_PIXEL_CLOCK
# endif

# if P_INVERT_HSYNC
#  define INVERT_HSYNC
# endif

# if P_INVERT_VSYNC
#  define INVERT_VSYNC
# endif

#endif

#if defined (CONFIG_LCD_TD035TTEA1)
	/* One Stop Display QVGA TD035TTEA1 320x240 */
#define PANEL_NAME	"OSB LCD Landscape QVGA"
#define PEL_CLOCK_EST	(865*1000*1000/100)     /* 4/8/8.65 MHz */
//#define PEL_CLOCK_EST	(600*1000*1000/100)     /* 4/8/8.65 MHz */
#define PEL_CLOCK_DIV	CLOCK_TO_DIV(PEL_CLOCK_EST, HCLK)
#define PEL_CLOCK	(HCLK/PEL_CLOCK_DIV)
#define PEL_WIDTH	(320)
#define PEL_HEIGHT	(240)
#define BIT_DEPTH	(16)
#define BITS_PER_PEL_2	BPP16
#define V_CYCLE		(264)			/* line/246/264/282 */
#define H_CYCLE		(440)			/* clock/326/440/472 */
#define HSYNC_WIDTH	(38)			/* clocks/2/38/256 */
#define VSYNC_WIDTH	(8)			/* lines/2/8/14 */
#define LEFT_MARGIN	(40)			/* clocks/2/40/256 */
#define TOP_MARGIN	(8)			/* lines/2/8/14 */
#define RIGHT_MARGIN	(H_CYCLE-PEL_WIDTH -LEFT_MARGIN-HSYNC_WIDTH)
#define BOTTOM_MARGIN	(V_CYCLE-PEL_HEIGHT-TOP_MARGIN -VSYNC_WIDTH)

#define INVERT_HSYNC
#define INVERT_VSYNC
#define INVERT_PIXEL_CLOCK
#endif

#define HBP(v)	((((v) - 1) & 0xff)<<24)
#define HFP(v)	((((v) - 1) & 0xff)<<16)
#define HSW(v)	((((v) - 1) & 0xff)<<8)
#define PPL(v)	((((v)/16 - 1) & 0x7f)<<2)

#define VBP(v)	(((v) & 0xff)<<24)
#define VFP(v)	(((v) & 0xff)<<16)
#define VSW(v)	((((v) - 1) & 0x3f)<<10)
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


#define CB_BUFFER ((PEL_WIDTH*PEL_HEIGHT*BIT_DEPTH)/8)

/* The component shifters are an optimzation for the splash decoder.
   We don't want to shift twice for every component, so we aggregate
   the shifts and include the operator.  This make a big difference in
   the ARM assembler. */

#if defined (CONFIG_LCD_BGR)

# define RED_SHIFT		10
# define GREEN_SHIFT		5
# define BLUE_SHIFT		0

# define RED_SHIFT_COMP		<<7
# define GREEN_SHIFT_COMP	<<2
# define BLUE_SHIFT_COMP	>>3

#else

# define RED_SHIFT		0
# define GREEN_SHIFT		5
# define BLUE_SHIFT		10

# define RED_SHIFT_COMP		>>3
# define GREEN_SHIFT_COMP	<<2
# define BLUE_SHIFT_COMP	<<7

#endif

unsigned short* buffer;


/* *** FIXME: need to sort out msleep so that we properly cope with
   *** the inline implementation. */

/* _msleep

   only works after the timer has been initialized.

*/

static void _msleep (int ms)
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


#define RGBI(r,g,b,i) ( (((r) & 0xf8) >>  3)\
		       |(((g) & 0xf8) <<  2)\
		       |(((b) & 0xf8) <<  7)\
		       |(((i) & 1) << 15))

static void clcdc_init (void)
{
  buffer = alloc_uncached (CB_BUFFER, 1024*1024);

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

  DRV_CLCDC_SETUP;

  CLCDC_TIMING0 = HBP (LEFT_MARGIN) | HFP (RIGHT_MARGIN) | HSW (HSYNC_WIDTH)
    | PPL (PEL_WIDTH);
  CLCDC_TIMING1 = VBP (TOP_MARGIN) | VFP (BOTTOM_MARGIN) | VSW (VSYNC_WIDTH)
    | LPP (PEL_HEIGHT);
  CLCDC_TIMING2   = CPL
#if defined (INVERT_PIXEL_CLOCK)
    | IPC
#endif
#if defined (INVERT_HSYNC)
    | IHS
#endif
#if defined (INVERT_VSYNC)
    | IVS
#endif
    | PCD (PEL_CLOCK_DIV);

  CLCDC_UPBASE    = (unsigned long) buffer;
  CLCDC_CTRL      = WATERMARK | TFT | BITS_PER_PEL_2
#if defined (CONFIG_LCD_BGR)
    | BGR
#endif
    ;

#if defined (CONFIG_ARCH_LH7952X)
  ALI_SETUP	  = 0x00000efd;
#endif

#if defined (CONFIG_LCD_3_5_QVGA_20)

# if defined (CONFIG_ARCH_LH79520)
  ALI_CTRL	  = 0x00000003; /* SPS & CLS */
# endif
# if defined (CONFIG_ARCH_LH79524) || defined (CONFIG_ARCH_LH79525)
  ALI_CTRL	  = 0x00000013;	/* VEEEN & SPS & CLS*/
# endif

# if defined (CONFIG_ARCH_LH7952X)
  ALI_TIMING1     = 0x0000087d;
  ALI_TIMING2     = 0x00009ad0;
# endif

# if defined (CONFIG_ARCH_LH7A400)
#  if defined (PANEL_ALI_SETUP)
  HRTFTC_SETUP   = PANEL_ALI_SETUP;
  HRTFTC_CON     = PANEL_ALI_CONTROL;
  HRTFTC_TIMING1 = PANEL_ALI_TIMING1;
  HRTFTC_TIMING2 = PANEL_ALI_TIMING2;
#  else
  HRTFTC_SETUP   = 0x00002efd;
  HRTFTC_CON     = 0x00000003;
  HRTFTC_TIMING1 = 0x0000087d;
  HRTFTC_TIMING2 = 0x00009ad0;
#  endif
# endif

# if defined (CONFIG_ARCH_LH7A404)
#  if defined (PANEL_ALI_SETUP)
  ALI_SETUP   = PANEL_ALI_SETUP;
  ALI_CONTROL = PANEL_ALI_CONTROL;
  ALI_TIMING1 = PANEL_ALI_TIMING1;
  ALI_TIMING2 = PANEL_ALI_TIMING2;
#  else
  ALI_SETUP   = 0x00002efd;
  ALI_CONTROL = 0x00000003;
  ALI_TIMING1 = 0x0000087d;
  ALI_TIMING2 = 0x00009ad0;
#  endif
# endif

#endif

}

static void clcdc_release (void)
{
  /* *** This sequence, as well as that which is in the initialization
     function, should be better documented. */

  DRV_CLCDC_SLEEP;

  DRV_CLCDC_BACKLIGHT_DISABLE;

  CLCDC_CTRL &= ~PWR;		/* Remove power */
  _msleep (20);			/* Wait 20ms */
  CLCDC_CTRL &= ~LCDEN;		/* Disable CLCDC controller output */
  DRV_CLCDC_DISABLE;

  DRV_CLCDC_POWER_DISABLE;

  /* Clobber the configuration registers just so we know that the
     kernel is configuring the controller. */
  CLCDC_TIMING0 = CLCDC_TIMING1 = CLCDC_TIMING2 = 0;
  CLCDC_UPBASE    = 0;

  /* Make sure not to access the CLCDC controller after
     DRV_CLCDC_RELEASE as the driver is likely to disable clocks
     making the registers unavailable.  */
  DRV_CLCDC_RELEASE;
}

#if !defined (CONFIG_SMALL)
static void clcdc_report (void)
{
  unsigned long clk = HCLK/((CLCDC_TIMING2 & 0x1f) + 2);
  printf ("  clcd:   buffer 0x%p  red %d<<%d  green %d<<%d  blue %d<<%d\n",
	  buffer, 5, RED_SHIFT, 5, GREEN_SHIFT, 5,  BLUE_SHIFT);
  printf ("          ctrl 0x%lx\n", CLCDC_CTRL);
  printf ("          timing0 0x%08lx  timing1 0x%08lx  timing2 0x%08lx\n",
	  CLCDC_TIMING0, CLCDC_TIMING1, CLCDC_TIMING2);
  if (clk < 1000000)
    printf ("          clk %ldHz\n", clk);
  else
    printf ("          clk %ld.%03ldMHz\n",
	    clk/(1000*1000), (clk/1000)%1000);
}
#endif

static __service_7 struct service_d lh7_clcdc_service = {
  .init    = clcdc_init,
  .release = clcdc_release,
#if !defined (CONFIG_SMALL)
  .report = clcdc_report,
#endif
};


#if defined (CONFIG_CMD_CLCDC_SPLASH)

int cmd_splash (const char* region)
{
  int result;
  struct descriptor_d d;
  void* pv;
  struct png_header hdr;
  unsigned short* ps = buffer;
  int cPalette = 0;
  unsigned char* rgbPalette;
  int i;
  int j;

  if (   (result = parse_descriptor (region, &d))
      || (result = open_descriptor (&d))) {
    printf ("Unable to open source %s\n", region);
    goto fail_early;
  }

  pv = open_pngr (&d);
  if (pv == NULL)
    goto fail_early;

  if (read_pngr_ihdr (pv, &hdr))
    goto fail_close;

  if (hdr.bit_depth != 8)
    goto fail_close;


  memset (buffer, 0, CB_BUFFER);

  {
    for (i = hdr.height; i--; ps += PEL_WIDTH - hdr.width) {
      const unsigned char* pb = read_pngr_row (pv);
      if (pb == NULL) {
	printf ("%s: read failed at row %d\n", __FUNCTION__, i);
	goto fail_close;
      }

      switch (hdr.color_type) {

      case 2:		/* RGB */
	for (j = 0; j < hdr.width; ++j, ++ps)
	  *ps = ((pb[j*3    ] & 0xf8) RED_SHIFT_COMP)
	    +   ((pb[j*3 + 1] & 0xf8) GREEN_SHIFT_COMP)
	    +   ((pb[j*3 + 2] & 0xf8) BLUE_SHIFT_COMP)
	    ;
	break;

      case 3:		/* Palettized */
	if (!cPalette)
	  cPalette = palette_pngr (pv, &rgbPalette);
	switch (hdr.bit_depth) {
	case 1:
	case 2:
	case 4:
	  break;		/* unimplemented depths */
	case 8:
	  for (j = 0; j < hdr.width; ++j, ++ps) {
	    unsigned char* color = &rgbPalette[pb[j]*3];
	    *ps = ((color[0] & 0xf8) RED_SHIFT_COMP)
	      +   ((color[1] & 0xf8) GREEN_SHIFT_COMP)
	      +   ((color[2] & 0xf8) BLUE_SHIFT_COMP)
	    ;
	  }
	}
	break;

      case 6:		/* RGBA */
	for (j = 0; j < hdr.width; ++j, ++ps)
	  *ps = ((pb[j*4    ] & 0xf8) RED_SHIFT_COMP)
	    +   ((pb[j*4 + 1] & 0xf8) GREEN_SHIFT_COMP)
	    +   ((pb[j*4 + 2] & 0xf8) BLUE_SHIFT_COMP);
	break;
      }
    }
  }

 fail_close:
  close_pngr (pv);

 fail_early:
  close_descriptor (&d);

  return result;
}

#endif


int cmd_clcdc (int argc, const char** argv)
{
  if (argc == 1) {
    printf ("clcdc: initialized %s at 0x%p\n", PANEL_NAME, buffer);
    printf ("  clk %d/%d->%d (%d)  sync (%d %d)  porch (%d %d %d %d)\n",
	    HCLK, PEL_CLOCK_DIV, PEL_CLOCK, PEL_CLOCK_EST,
	    HSYNC_WIDTH, VSYNC_WIDTH,
	    LEFT_MARGIN, RIGHT_MARGIN, TOP_MARGIN, BOTTOM_MARGIN);
    return 0;
  }

#if defined (CONFIG_CMD_CLCDC_SPLASH)
  if (strcmp (argv[1], "splash") == 0) {
    if (argc != 3)
      return ERROR_PARAM;
    return cmd_splash (argv[2]);
  }
#endif

#if defined (CONFIG_CMD_CLCDC_TEST)
  if (strcmp (argv[1], "bars") == 0) {
    int i;
    for (i = 0; i < PEL_HEIGHT*PEL_WIDTH; ++i) {
      if (i < PEL_WIDTH
	  || i%PEL_WIDTH == 0
	  || i%PEL_WIDTH == PEL_WIDTH - 1
	  || i > (PEL_HEIGHT - 1)*PEL_WIDTH)
	buffer[i] = 0xffff;
      else if (i < PEL_WIDTH*2
	  || i%PEL_WIDTH == 1
	  || i%PEL_WIDTH == PEL_WIDTH - 2
	  || i > (PEL_HEIGHT - 2)*PEL_WIDTH)
	buffer[i] = 0x0000;
      else if (i > 3*(PEL_HEIGHT*PEL_WIDTH)/4)
	buffer[i] = 0xffff;
      else if (i > 2*(PEL_HEIGHT*PEL_WIDTH)/4)
	buffer[i] = I(0x1f,(i%PEL_WIDTH)*255/255)<<BLUE_SHIFT;
      else if (i > 1*(PEL_HEIGHT*PEL_WIDTH)/4)
	buffer[i] = I(0x1f,(i%PEL_WIDTH)*255/255)<<GREEN_SHIFT;
      else if (i > 0*(PEL_HEIGHT*PEL_WIDTH)/4)
	buffer[i] = I(0x1f,(i%PEL_WIDTH)*255/255)<<RED_SHIFT;
    }
    return 0;
  }
  if (strcmp (argv[1], "white") == 0) {
    memset (buffer, 0xff, PEL_HEIGHT*PEL_WIDTH*(BIT_DEPTH/8));
    return 0;
  }
  if (strcmp (argv[1], "black") == 0) {
    memset (buffer, 0, PEL_HEIGHT*PEL_WIDTH*(BIT_DEPTH/8));
    return 0;
  }
# if (BIT_DEPTH==16)
  if (strcmp (argv[1], "red") == 0) {
    int i;
    for (i = 0; i < PEL_HEIGHT*PEL_WIDTH; ++i)
      buffer[i] = 0x1f << RED_SHIFT;
    return 0;
  }
  if (strcmp (argv[1], "green") == 0) {
    int i;
    for (i = 0; i < PEL_HEIGHT*PEL_WIDTH; ++i)
      buffer[i] = 0x1f << GREEN_SHIFT;
    return 0;
  }
  if (strcmp (argv[1], "blue") == 0) {
    int i;
    for (i = 0; i < PEL_HEIGHT*PEL_WIDTH; ++i)
      buffer[i] = 0x1f << BLUE_SHIFT;
    return 0;
  }
# endif
#endif

  if (strcmp (argv[1],"on") == 0) {
    CLCDC_CTRL      |= LCDEN;	/* Enable CLCDC */
    DRV_CLCDC_POWER_ENABLE;
    _msleep (20);		/* Wait 20ms for digital signals  */
    CLCDC_CTRL      |= PWR;	/* Apply power */

    DRV_CLCDC_WAKE;

    /* *** FIXME: this value is calculable based on the LCD controller
       parameters and the frame size. 40ms is the time for two frame
       displays at 800x600 with a 25MHz pixel clock.  100ms was the
       shortest time found that gave a solid image when the backlight
       came on (with above parameters). */
    _msleep (100);		/* Wait for the display image to settle */
    DRV_CLCDC_BACKLIGHT_ENABLE;
    return 0;
  }

  if (strcmp (argv[1],"off") == 0) {
    DRV_CLCDC_BACKLIGHT_DISABLE;

    DRV_CLCDC_SLEEP;

    CLCDC_CTRL &= ~PWR;		/* Remove power */
    _msleep (20);		/* Wait 20ms */
    CLCDC_CTRL &= ~LCDEN;	/* Disable CLCDC controller */
    DRV_CLCDC_DISABLE;

    DRV_CLCDC_POWER_DISABLE;
    return 0;
  }

  return ERROR_PARAM;
}

static __command struct command_d c_clcdc = {
  .command = "clcdc",
  .func = cmd_clcdc,
  COMMAND_DESCRIPTION ("commands for controlling the color LCD controller")
  COMMAND_HELP(
"clcdc [SUBCOMMAND [PARAMETER]]\n"
" Without a SUBCOMMAND, it displays the LCD setup parameters.\n"
" on         - enable the controller and LCD backlight\n"
" off        - disable the controller and LCD backlight\n"
#if defined (CONFIG_CMD_CLCDC_TEST)
" bars       - draw color bars into frame buffer\n"
" white      - draw white into frame buffer\n"
" black      - draw black into frame buffer\n"
# if BIT_DEPTH==16
" red        - draw red into frame buffer\n"
" green      - draw green into frame buffer\n"
" blue       - draw blue into frame buffer\n"
# endif
#endif
#if defined (CONFIG_CMD_CLCDC_SPLASH)
" splash SRC - draw splash image (PNG) into frame buffer\n"
#endif
"\n"
"  e.g.  clcdc on\n"
#if defined (CONFIG_CMD_CLCDC_SPLASH)
"        clcdc splash fat://1/logo.png\n"
#endif
  )
};
