/* lh79520.h
     $Id$

   written by Marc Singer
   14 Nov 2004

   Copyright (C) 2004 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__LH79520_H__)
#    define   __LH79520_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#define RCPC_PHYS	0xfffe2000

#define RCPC_CTRL			0x00
#define RCPC_REMAP			0x08
#define RCPC_SOFTRESET			0x0c
#define RCPC_RESETSTATUS		0x10
#define RCPC_RESETSTATUSCLR		0x14
#define RCPC_HCLKCLKPRESCALE		0x18
#define RCPC_CPUCLKPRESCALE		0x1C
#define RCPC_PERIPHCLKCTRL		0x24
#define RCPC_PERIPHCLKCTRL2		0x28
#define RCPC_AHBCLKCTRL			0x2C
#define RCPC_PERIPHCLKSEL		0x30
#define RCPC_PERIPHCLKSEL2		0x34
#define RCPC_CORECLKCONFIG		0x88

#define RCPC_AHBCLK_SDC			(1<<1)
#define RCPC_AHBCLK_DMA			(1<<0)
#define RCPC_PERIPHCLK_T02		(1<<5)
#define RCPC_PERIPHCLK_T01		(1<<4)
#define RCPC_PERIPHCLK_U2		(1<<2)
#define RCPC_PERIPHCLK_U1		(1<<1)
#define RCPC_PERIPHCLK_U0		(1<<0)

#define RCPC_CPUCLKPRESCALE_78		0x2
#define RCPC_CPUCLKPRESCALE_52		0x3
#define RCPC_CPUCLKPRESCALE_39		0x4
#define RCPC_HCLKPRESCALE_52		0x3
#define RCPC_HCLKPRESCALE_39		0x4
//#define RCPC_CPUCLK_PRESCALE_DEFAULT		RCPC_CPUCLK_PRESCALE_52
//#define RCPC_HCLK_PRESCALE_DEFAULT		RCPC_HCLK_PRESCALE_52

#if 0 // run the CPU at 52 MHz, and the bus at 52 MHz
# define RCPC_CPUCLKPRESCALE_V	RCPC_CPUCLKPRESCALE_52
# define RCPC_HCLKPRESCALE_V	RCPC_HCLKPRESCALE_52
# define RCPC_CORECLKCONFIG_V	3			/* FastBus */
# define SDRC_SDRAM_REFTIMER_V	REFTIMER_52
# define HCLK_FREQ		(309657600/6)
#else // run the CPU at 78 MHz, and the Bus at 52 MHz
# define RCPC_CPUCLKPRESCALE_V	RCPC_CPUCLKPRESCALE_78
# define RCPC_HCLKPRESCALE_V	RCPC_HCLKPRESCALE_52
# define RCPC_CORECLKCONFIG_V	0			/* std mode, async */
# define SDRC_SDRAM_REFTIMER_V	REFTIMER_78
# define HCLK_FREQ		(309657600/6)
#endif

#define IOCON_PHYS	0xfffe5000
#define IOCON_MEMMUX	0x00
#define IOCON_UARTMUX	0x10

#define TIMER0_PHYS	0xfffc4000
#define TIMER1_PHYS	0xfffc4020
#define TIMER2_PHYS	0xfffc5000
#define TIMER3_PHYS	0xfffc4020

#define TIMER_LOAD	0x00
#define TIMER_VALUE	0x04
#define TIMER_CONTROL	0x08
#define TIMER_CLEAR	0x0c

#define TIMER_ENABLE	(1<<7)
#define TIMER_PERIODIC	(1<<6)	// !FREERUNNING
#define TIMER_CASCADE	(1<<4)
#define TIMER_SCALE_MASK (3<<2)
#define TIMER_SCALE_1	 (0<<2)
#define TIMER_SCALE_16	 (1<<2)
#define TIMER_SCALE_256	 (2<<2)


#endif  /* __LH79520_H__ */
