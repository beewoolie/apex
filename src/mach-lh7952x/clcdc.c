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

#define USE_PNG

#include <config.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <apex.h>
#include "hardware.h"


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
#define PANEL_NAME	"LCD 6.4\" VGA"
#define PEL_CLOCK_EST	(28330000)     /* ?-25.18MHz-28.33MHz */
#define PEL_CLOCK_DIV	CLOCK_TO_DIV(PEL_CLOCK_EST, HCLK)
#define PEL_CLOCK	(HCLK/PEL_CLOCK_DIV)
#define PEL_WIDTH	(640)
#define PEL_HEIGHT	(480)
#define BIT_DEPTH	(16)
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
#define BIT_DEPTH	(16)
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


//#if defined (USE_PNG)
//#include <png.h>
//const unsigned char rgbPNG[] = {
//# include <splash/apex-png.h>
////# include <splash/cc-png.h>
//};
//#endif

#define CB_BUFFER ((PEL_WIDTH*PEL_HEIGHT*BIT_DEPTH)/8)

unsigned short* buffer;
//static unsigned short __xbss(clcdc) buffer[PEL_WIDTH*PEL_HEIGHT];

static unsigned short __attribute__((section(".clcdc.xbss"))) 
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

#if 0
static void draw_splash (void)
{
#if defined (USE_PNG)
  void* pv = open_pngr (d);	/* *** FIXME: need a descriptor */
  struct png_header hdr;
  unsigned short* ps = buffer;
  int i;
  int j;

  if (pv == 0)
    return;

  if (read_pngr_ihdr (pv, &hdr))
    goto fail;

  if (hdr.bit_depth != 8)
    goto fail;

  {
    for (i = hdr.height; i--; ) {
      const unsigned char* pb = read_pngr_row (pv);
      if (pb == NULL) {
	printf ("%s: read failed at %d\n", __FUNCTION__, i);
	goto fail;
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

fail:
  close_png (pv);

#endif
}
#endif

static void clcdc_init (void)
{
//  PRINTF ("%s\n", __FUNCTION__);

#if !defined (USE_PNG)
  /* Color bars */
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
#endif

  draw_splash ();

  RCPC_CTRL |= (1<<9); /* Unlock */

#if defined (CONFIG_ARCH_LH79520)
  RCPC_PERIPHCLKCTRL2 &= ~(1<<0);
  RCPC_PERIPHCLKSEL2 &= ~(1<<0); /* Use HCLK */
  RCPC_LCDCLKPRESCALE = (1>>1);	/* HCLK/1 */
#endif

#if defined (CONFIG_ARCH_LH79524) || defined (CONFIG_ARCH_LH79525)
  RCPC_PCLKCTRL1 &= ~(1<<0);
  RCPC_AHBCLKCTRL &= ~(1<<4);
//  RCPC_LCDPRE = (8>>1); 	/* HCLK divisor 8 */
  RCPC_LCDPRE = (1>>1); 	/* HCLK divisor 1 */
#endif

  RCPC_CTRL &= ~(1<<9); /* Lock */


#if defined (CONFIG_ARCH_LH79520)
  IOCON_LCDMUX |= (1<<28) | (1<<27) | (1<<26) | (1<<25) | (1<<24)
    | (1<<21) | (1<<20) | (1<<19) | (1<<18) | (1<<15) | (1<<12) | (1<<11)
    | (1<<10) | (1<<3) | (1<<2);
  MASK_AND_SET (IOCON_LCDMUX, 3<<22, 2<<22); /* All for HR-TFT */
  MASK_AND_SET (IOCON_LCDMUX, 3<<16, 2<<16);
  MASK_AND_SET (IOCON_LCDMUX, 3<<13, 2<<13);
  MASK_AND_SET (IOCON_LCDMUX, 3<<8,  2<<8);
  MASK_AND_SET (IOCON_LCDMUX, 3<<6,  2<<6);
  MASK_AND_SET (IOCON_LCDMUX, 3<<4,  0<<4);
  MASK_AND_SET (IOCON_LCDMUX, 3<<0,  2<<0);
//  IOCON_LCDMUX = 0x1fbeda9e;
#endif

#if defined (CONFIG_ARCH_LH79524) || defined (CONFIG_ARCH_LH79525)
  MASK_AND_SET (IOCON_MUXCTL1,
		(3<<2)|(3<<0),
		(1<<2)|(1<<0));
  MASK_AND_SET (IOCON_RESCTL1,
		(3<<2)|(3<<0),
		(0<<2)|(0<<0));

  MASK_AND_SET (IOCON_MUXCTL19,
		(3<<12)|(3<<8)|(3<<4)|(3<<2), 
		/* LCDVEEN, LCDVDDEN, LCDREV, LCDCLS */
		(1<<12)|(1<<8)|(2<<4)|(1<<2));
  MASK_AND_SET (IOCON_RESCTL19,
		(3<<12)|(3<<8)|(3<<4)|(3<<2), 
		(0<<12)|(0<<8)|(0<<4)|(0<<2));

  MASK_AND_SET (IOCON_MUXCTL20, 
		(3<<14)|(3<<10)|(3<<6)|(3<<2)|(3<<0), 
		/* LCDPS, LCDDCLK, LCDHRLP, LCDSPS, LCDSPL */
		(1<<14)|(1<<10)|(2<<6)|(2<<2)|(2<<0)); 
  MASK_AND_SET (IOCON_RESCTL20, 
		(3<<14)|(3<<10)|(3<<6)|(3<<2)|(3<<0), 
		(0<<14)|(0<<10)|(0<<6)|(0<<2)|(0<<0)); 
  
  MASK_AND_SET (IOCON_MUXCTL21, 
		(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0), 
		(1<<10)|(1<<8)|(1<<6)|(1<<4)|(1<<2)|(1<<0));
  MASK_AND_SET (IOCON_RESCTL21, 
		(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0), 
		(1<<10)|(1<<8)|(1<<6)|(1<<4)|(1<<2)|(1<<0));

  MASK_AND_SET (IOCON_MUXCTL22,
		(3<<14)|(3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0), 
		(1<<14)|(1<<12)|(1<<10)|(1<<8)|(1<<6)|(1<<4)|(1<<2)|(1<<0)); 
  MASK_AND_SET (IOCON_RESCTL22, 
		(3<<14)|(3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0), 
		(0<<14)|(0<<12)|(0<<10)|(0<<8)|(0<<6)|(0<<4)|(0<<2)|(0<<0)); 
#endif

  CLCDC_TIMING0   = 0x0e143c38;
  CLCDC_TIMING1   = 0x075f013f;
  CLCDC_TIMING2   = 0x00ef300e;
  CLCDC_UPBASE    = (unsigned long) buffer;
  CLCDC_CTRL      = 0x00010028;
  ALI_SETUP	  = 0x00000efd;
#if defined (CONFIG_ARCH_LH79520)
  ALI_CTRL	  = 0x00000003; /* SPS & CLS */
#endif
#if defined (CONFIG_ARCH_LH79524) || defined (CONFIG_ARCH_LH79525)
  ALI_CTRL	  = 0x00000013;	/* VEEEN & SPS & CLS*/
#endif
  ALI_TIMING1     = 0x0000087d;
  ALI_TIMING2     = 0x00009ad0;

  CLCDC_CTRL     |= (1<<0); /* Enable CLCDC */
#if defined CPLD_CTRL1_LCD_POWER_EN
  MASK_AND_SET (CPLD_CTRL1, 
		CPLD_CTRL1_LCD_POWER_EN | CPLD_CTRL1_LCD_OE,
		CPLD_CTRL1_LCD_POWER_EN);
#endif
  msleep (20);			/* Wait 20ms for digital signals  */
  CLCDC_CTRL     |= (1<<11); /* Apply power */
#if defined CPLD_CTRL1_LCD_BACKLIGHT_EN
  CPLD_CTRL1 |= CPLD_CTRL1_LCD_BACKLIGHT_EN;
#endif
}

static void clcdc_release (void)
{
#if defined CPLD_CTRL1_LCD_BACKLIGHT_EN
  CPLD_CTRL1 &= ~CPLD_CTRL1_LCD_BACKLIGHT_EN;
#endif
  CLCDC_CTRL &= ~(1<<11); /* Remove power */
  msleep (20);			/* Wait 20ms */
  CLCDC_CTRL &= ~(1<<0); /* Disable CLCDC controller */
#if defined CPLD_CTRL1_LCD_POWER_EN
  MASK_AND_SET (CPLD_CTRL1, 
		CPLD_CTRL1_LCD_POWER_EN | CPLD_CTRL1_LCD_OE,
		CPLD_CTRL1_LCD_OE);
#endif

				/*  Shutdown all the clocks */
  RCPC_CTRL	  |=  (1<<9); /* Unlock */

#if defined (CONFIG_ARCH_LH79520)
  RCPC_PERIPHCLKCTRL2 |= (1<<0);
#endif

#if defined (CONFIG_ARCH_LH79524) || defined (CONFIG_ARCH_LH79525)
  RCPC_PCLKCTRL1  |=   1<<0;
  RCPC_AHBCLKCTRL |=   1<<4;
#endif

  RCPC_CTRL	  &= ~(1<<9); /* Lock */
}

static __service_7 struct service_d lpd79524_clcdc_service = {
  .init = clcdc_init,
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

