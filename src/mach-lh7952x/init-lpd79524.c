/* init-lpd79524.c
     $Id$

   written by Marc Singer
   28 Oct 2004

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

   Hardware initializations for the LPD79524.  Some initializations
   may be left to drivers, such as the serial interface and timer.

*/

#include <config.h>
#include <asm/bootstrap.h>
#include <service.h>

#include "hardware.h"

//#define USE_SLOW

#if defined (USE_SLOW)
# define EMC_RASCAS_V	((3<<0)|(3<<8))	// RAS3, CAS3
#else
# define EMC_RASCAS_V	((3<<0)|(2<<8))	// RAS3, CAS2
#endif

#if defined (USE_SLOW)
# define SDRAM_CHIP_MODE	(0x32<<11)	// CAS3 BURST4
#else
# define SDRAM_CHIP_MODE	(0x22<<11)	// CAS2 BURST4
#endif

	// SDRAM
#define SDRAM_RASCAS		EMC_RASCAS_V
#define SDRAM_CFG_SETUP		((1<<14)|(1<<12)|(3<<9)|(1<<7)) /*32LP;16Mx16*/
#define SDRAM_CFG		(SDRAM_CFG_SETUP | (1<<19))


/* usleep

   this function accepts a count of microseconds and will wait at
   least that long before returning.  The timer clock must be
   activated by the initialization code before using usleep.

   When in C, this function was one instruction longer than the
   hand-coded assembler.  For some reason, the compiler would reload
   the TIMER1_PHYS at the top of the while loop.

   Note that this function is neither __naked nor static.  It is
   available to the rest of the application as is.

   Keep in mind that it has a limit of about 32ms.  Anything longer
   (and less than 16 bits) will return immediately.

   To be precise, the interval is HCLK/64 or 1.2597us.  In other
   words, it is about 25% longer than desired.  The code compensates
   by removing 25% of the requested delay.  The clever ARM instruction
   set makes this a single operation.

 */

void __section (.bootstrap) usleep (unsigned long us)
{
  __asm ("str %1, [%0, #0]\n\t"
	 "str %2, [%0, #0xc]\n\t"
	 "str %3, [%0, #0]\n\t"
      "0: ldr %2, [%0, #0xc]\n\t"
	 "tst %2, %4\n\t"
	 "beq 0b\n\t"
	 :
	 : "r" (TIMER1_PHYS), 
	   "r" (0),
	   "r" ((unsigned long) 0x8000 - (us - (us>>4))), /* timer counts up */
	   "r" ((1<<1)|(5<<2)),
	   "i" (0x8000)
	 );
}


/* initialize_bootstrap

   performs vital SDRAM initialization as well as some other memory
   controller initializations.  It will perform no work if we are
   already running from SDRAM.

   The assembly output of this implementation of the initialization is
   more dense than the assembler version using a table of
   initializations.  This is primarily due to the compiler's ability
   to keep track of the register set offsets and value being output.

   The return value is true if SDRAM has been initialized and false if
   this initialization has already been performed.  Note that the
   non-SDRAM initializations are performed regardless of whether or
   not we're running in SDRAM.

   *** FIXME: The memory region 0x1000000-0x20000000 will be
   *** incorrectly detected as SDRAM with code below.  It is OK as
   *** long as the loader never runs from high nCS0 memory.

*/

void __naked __section (.bootstrap) initialize_bootstrap (void)
{
  unsigned long lr;
  __asm volatile ("mov %0, lr" : "=r" (lr));

#if defined (CONFIG_DEBUG_LL)
  /* *** Need to initialize the serial port here. */

  UART_LCR  = UART_LCR_WLS_8 | UART_LCR_STB_1 | UART_LCR_DLAB;
  UART_DLL  = 8;	// divisor_l;
  UART_DLH  = 0;	// divisor_h;
  UART_LCR  = UART_LCR_WLS_8 | UART_LCR_STB_1;

  UART_IER  = UART_IER_UUE;	/* Enable UART, mask all interrupts */
				/* Clear interrupts? */
  PUTC_LL('A');
#endif

	/* Setup HCLK, FCLK and peripheral clocks */
  RCPC_CTRL      |= RCPC_CTRL_UNLOCK;

  /* *** We may want to move the UART stuff into the serial driver */
  RCPC_AHBCLKCTRL = RCPC_AHBCLKCTRL_V;
  RCPC_PCLKCTRL0  = RCPC_PCLKCTRL0_V;
  RCPC_PCLKCTRL1  = RCPC_PCLKCTRL1_V;
  RCPC_PCLKSEL0   = RCPC_PCLKSEL0_V;

  RCPC_CORECONFIG = RCPC_CORECONFIG_FASTBUS;
  RCPC_SYSPLLCNTL = RCPC_SYSPLLCNTL_V;
  RCPC_SYSCLKPRE  = RCPC_SYSCLKPRE_V;
  RCPC_CPUCLKPRE  = RCPC_CPUCLKPRE_V;
  RCPC_CORECONFIG = RCPC_CORECONFIG_V;

  RCPC_CTRL      &= ~RCPC_CTRL_UNLOCK;

	/* Setup IO pin multiplexers */
  IOCON_MUXCTL5  = IOCON_MUXCTL5_V; 	/* UART */
  IOCON_MUXCTL6  = IOCON_MUXCTL6_V;	/* UART */
  IOCON_MUXCTL10 = IOCON_MUXCTL10_V;	/* D */
  IOCON_MUXCTL11 = IOCON_MUXCTL11_V;	/* D */
  IOCON_MUXCTL12 = IOCON_MUXCTL12_V;	/* D */
  IOCON_MUXCTL19 = IOCON_MUXCTL19_V;	/* D */
  IOCON_MUXCTL20 = IOCON_MUXCTL20_V;	/* D */

	/* NAND flash, 8 bit */
  EMC_SCONFIG0    = 0x80;
  EMC_SWAITWEN0   = 1;
  EMC_SWAITOEN0   = 1;
  EMC_SWAITRD0    = 2;
  EMC_SWAITPAGE0  = 2;
  EMC_SWAITWR0    = 2;
  EMC_STURN0      = 2;
  //  EMC_SWAITWEN0   = 1;
  //  EMC_SWAITOEN0   = 3;
  //  EMC_SWAITRD0    = 5;
  //  EMC_SWAITPAGE0  = 2;
  //  EMC_SWAITWR0    = 3;
  //  EMC_STURN0      = 1;

	/* NOR flash, 16 bit */
  EMC_SCONFIG1    = 0x81;
  EMC_SWAITWEN1   = 1;
  EMC_SWAITOEN1   = 1;
  EMC_SWAITRD1    = 6;
  EMC_SWAITPAGE1  = 2;
  EMC_SWAITWR1    = 6;
  EMC_STURN1      = 1;

	/* CPLD, 16 bit */
  EMC_SCONFIG3    = 0x81;

#if defined (CONFIG_NAND_LPD)
  IOCON_MUXCTL7  = IOCON_MUXCTL7_V;		/* A */
  IOCON_RESCTL7  = IOCON_RESCTL7_V;		/* no pull */
  IOCON_MUXCTL14 = IOCON_MUXCTL14_V;		/* nCS0 norm */
  CPLD_FLASH	&= ~(CPLD_FLASH_NANDSPD);	/* Fast NAND */
#endif

  __asm volatile ("tst %0, #0xf0000000\n\t"
		  "beq 1f\n\t"
		  "cmp %0, %1\n\t"
		  "movls r0, #0\n\t"
		  "movls pc, %0\n\t"
		"1:" :: "r" (lr), "i" (SDRAM_BANK1_PHYS));

	/* SDRAM */
  EMC_READCONFIG  = EMC_READCONFIG_CMDDELAY;
  EMC_DYNCFG0     = SDRAM_CFG_SETUP;
  EMC_DYNRASCAS0  = SDRAM_RASCAS;
  EMC_DYNCFG1     = SDRAM_CFG_SETUP;
  EMC_DYNRASCAS1  = SDRAM_RASCAS;

  EMC_PRECHARGE   = NS_TO_HCLK(20);
  EMC_DYNM2PRE    = NS_TO_HCLK(60);
  EMC_REFEXIT     = NS_TO_HCLK(120);
  EMC_DOACTIVE    = NS_TO_HCLK(120);
  EMC_DIACTIVE    = NS_TO_HCLK(120);
  EMC_DWRT	  = NS_TO_HCLK(40);
  EMC_DYNACTCMD   = NS_TO_HCLK(120);
  EMC_DYNAUTO     = NS_TO_HCLK(120);
  EMC_DYNREFEXIT  = NS_TO_HCLK(120);
  EMC_DYNACTIVEAB = NS_TO_HCLK(40);
  EMC_DYNAMICTMRD = NS_TO_HCLK(40);

  EMC_DYNMCTRL    = ((1<<1)|(1<<0));
  usleep (200);
  EMC_DYNMCTRL    = (1<<1)|(1<<0)|(3<<7); /* NOP command */
  usleep (200);
  EMC_DYNMCTRL    = (1<<1)|(1<<0)|(2<<7); /* PRECHARGE ALL*/
  EMC_DYNMREF     = NS_TO_HCLK(100)/16 + 1;
  usleep (250);
  EMC_DYNMREF     = NS_TO_HCLK(7812)/16;
  EMC_DYNMCTRL    = (1<<1)|(1<<0)|(1<<7); /* MODE command */

  __REG (SDRAM_BANK0_PHYS + SDRAM_CHIP_MODE);
  __REG (SDRAM_BANK1_PHYS + SDRAM_CHIP_MODE);

  EMC_DYNMCTRL    = (1<<1)|(1<<0)|(0<<7); /* NORMAL */
  EMC_DYNCFG0     = SDRAM_CFG;
  EMC_DYNCFG1     = SDRAM_CFG;
  
  __asm volatile ("mov r0, #-1\t\n"
		  "mov pc, %0" : : "r" (lr));
}


/* target_init

   performs the rest of the hardware initialization that didn't have
   to be performed during the bootstrap phase.

*/

static void target_init (void)
{
#if !defined (CONFIG_NAND_LPD)
	/* IOCON to clear special NAND modes.  These modes must be
	   controlled explicitly within driver code. */
  IOCON_MUXCTL7  = IOCON_MUXCTL7_V;   /* A23,A22 */
  IOCON_RESCTL7  = IOCON_RESCTL7_V;   /* pull-down */
  IOCON_MUXCTL14 = IOCON_MUXCTL14_V;  /* nCS0 normalize */
#else
  /* *** FIXME: I'm not sure why I need these.  It ought to default to
     *** these modes when not using NAND. */
  IOCON_MUXCTL7  = IOCON_MUXCTL7_V;   /* A23,A22 */
  IOCON_RESCTL7  = IOCON_RESCTL7_V;   /* pull-down */
  IOCON_MUXCTL14 = IOCON_MUXCTL14_V;  /* nCS0 normalize */
#endif

	/* CompactFlash, 16 bit */
  EMC_SCONFIG2    = 0x81;
  //  EMC_SWAITWEN2   = 2;
  //  EMC_SWAITOEN2   = 2;
  //  EMC_SWAITRD2    = 6;
  //  EMC_SWAITPAGE2  = 2;
  //  EMC_SWAITWR2    = 6;
  //  EMC_STURN2      = 1;
  EMC_SWAITWEN2   = 2;
  EMC_SWAITOEN2   = 2;
  EMC_SWAITRD2    = 0x1f;
  EMC_SWAITPAGE2  = 0x1f;
  EMC_SWAITWR2    = 0x1f;
  EMC_STURN2      = 0xf;

	/* CPLD, 16 bit */
  EMC_SWAITWEN3   = 2;
  EMC_SWAITOEN3   = 2;
  EMC_SWAITRD3    = 5;
  EMC_SWAITPAGE3  = 2;
  EMC_SWAITWR3    = 5;
  EMC_STURN3      = 2;

  BOOT_CS1OV &= ~(1<<0);
}

static void target_release (void)
{
  /* Flash is enabled for the kernel */
  CPLD_FLASH |= CPLD_FLASH_FL_VPEN;
}

static __service_0 struct service_d lh79524_target_service = {
  .init    = target_init,
  .release = target_release,
};
