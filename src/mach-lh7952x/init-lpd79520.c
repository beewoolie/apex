/* init-lpd79524.c
     $Id$

   written by Marc Singer
   14 Nov 2004

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

   Hardware initializations for the LPD79520.  Some initializations
   may be left to drivers, such as the serial interface and timer.

*/

#include <config.h>
#include <asm/bootstrap.h>

#include "lh79520.h"
#include "lpd79520.h"

//#define USE_SLOW

#if defined (USE_SLOW)
# define SDRC_CFG_V	0x01a40088	// RAS3, REStoCAS3
#else
# define SDRC_CFG_V	0x01f40088	// RAS2, RAStoCAS2
#endif

#if defined (USE_SLOW)
# define SDRAM_CHIP_MODE	(0x32<<12)	// CAS3 BURST4
#else
# define SDRAM_CHIP_MODE	(0x22<<12)	// CAS2 BURST4
#endif

#define SDRAM_NORMAL_COMMAND (SDRAM_INIT_NORMAL | SDRAM_WBE | SDRAM_RBE)

	// SDRAM
//#define SDRAM_RASCAS		SDRC_RASCAS_V
//#define SDRAM_CFG_SETUP		((1<<14)|(1<<12)|(3<<9)|(1<<7)) /*32LP;16Mx16*/
//#define SDRAM_CFG		(SDRAM_CFG_SETUP | (1<<19))


/* usleep

   this function accepts a count of microseconds and will wait at
   least that long before returning.  

   Note that this function is neither __naked nor static.  It is
   available to the rest of the application as is.

   The timer interval is (309657600/6)/256 which is about 4.96us.

 */

void __section(bootstrap) usleep (unsigned long us)
{
  __asm ("str %1, [%0, #8]\n\t"	/* Stop timer */
	 "str %2, [%0, #0]\n\t"
	 "str %3, [%0, #8]\n\t"
      "0: ldr %2, [%0, #4]\n\t"
	 "tst %2, %4\n\t"
	 "beq 0b\n\t"
	 :
	 : "r" (TIMER0_PHYS), 
	   "r" (0),
	   "r" ((unsigned long) 0x8000 - (us/5)), /* timer counts up */
	   "r" (TIMER_ENABLE | TIMER_PRESCALE_256)
	   "i" (0x8000)
	 );
}


/* initialize_bootstrap

   performs vital SDRAM initialization as well as some other memory
   controller initializations.  It will perform no work if we are
   already running from SDRAM.  It will 

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

void __naked __section(bootstrap) initialize_bootstrap (void)
{
  unsigned long lr;
  __asm volatile ("mov %0, lr" : "=r" (lr));


	/* Setup HCLK, FCLK and peripheral clocks */
  __REG (RCPC_PHYS | RCPC_CTRL) |= 0x200; /* Unlock */
  __REG (RCPC_PHYS | RCPC_CPUCLKPRESCALE) = RCPC_CPUCLKPRESCALE_V;
  __REG (RCPC_PHYS | RCPC_HCLKCLKPRESCALE) = RCPC_HCLKCLKPRESCALE_V;
  __REG (RCPC_PHYS | RCPC_CORECLKCONFIG) = RCPC_CORECLKCONFIG_V;
  __REG (RCPC_PHYS | RCPC_AHBCLKCTRL) &= ~RCPC_AHBCLK_SDC;
  __REG (RCPC_PHYS | RCPC_PERIPHCLKCTRL) &= ~(  RCPC_PERPIHCLK_UART1
					      | RCPC_PERIPHCLK_T01);

  __REG (RCPC_PHYS | RCPC_CTRL) &= ~0x200; /* Lock */

  /* Stop watchdog? */

  __REG (IOCON_PHYS | IOCON_MEMMUX) = 0x3fff; /* 32 bit, SDRAM, all SRAM */ 

  /* *** FIXME: move this? */
  __REG (IOCON_PHYS | IOCON_UARTMUX) = 0xf; /* All UARTS available */

#if 0
	/* Setup IO pin multiplexers */
  __REG (IOCON_PHYS | IOCON_MUXCTL5)  = IOCON_MUXCTL5_V; 	/* UART */
  __REG (IOCON_PHYS | IOCON_MUXCTL6)  = IOCON_MUXCTL6_V;	/* UART */
  __REG (IOCON_PHYS | IOCON_MUXCTL10) = IOCON_MUXCTL10_V;	/* D */
  __REG (IOCON_PHYS | IOCON_MUXCTL11) = IOCON_MUXCTL11_V;	/* D */
  __REG (IOCON_PHYS | IOCON_MUXCTL12) = IOCON_MUXCTL12_V;	/* D */
  __REG (IOCON_PHYS | IOCON_MUXCTL19) = IOCON_MUXCTL19_V;	/* D */
  __REG (IOCON_PHYS | IOCON_MUXCTL20) = IOCON_MUXCTL20_V;	/* D */

	/* NAND flash, 8 bit */
  __REG (EMC_PHYS | EMC_SCONFIG0)    = 0x80;
  __REG (EMC_PHYS | EMC_SWAITWEN0)   = 1;
  __REG (EMC_PHYS | EMC_SWAITOEN0)   = 1;
  __REG (EMC_PHYS | EMC_SWAITRD0)    = 2;
  __REG (EMC_PHYS | EMC_SWAITPAGE0)  = 2;
  __REG (EMC_PHYS | EMC_SWAITWR0)    = 2;
  __REG (EMC_PHYS | EMC_STURN0)      = 2;
  //  __REG (EMC_PHYS | EMC_SWAITWEN0)   = 1;
  //  __REG (EMC_PHYS | EMC_SWAITOEN0)   = 3;
  //  __REG (EMC_PHYS | EMC_SWAITRD0)    = 5;
  //  __REG (EMC_PHYS | EMC_SWAITPAGE0)  = 2;
  //  __REG (EMC_PHYS | EMC_SWAITWR0)    = 3;
  //  __REG (EMC_PHYS | EMC_STURN0)      = 1;

	/* NOR flash, 16 bit */
  __REG (EMC_PHYS | EMC_SCONFIG1)    = 0x81;
  __REG (EMC_PHYS | EMC_SWAITWEN1)   = 1;
  __REG (EMC_PHYS | EMC_SWAITOEN1)   = 1;
  __REG (EMC_PHYS | EMC_SWAITRD1)    = 6;
  __REG (EMC_PHYS | EMC_SWAITPAGE1)  = 2;
  __REG (EMC_PHYS | EMC_SWAITWR1)    = 6;
  __REG (EMC_PHYS | EMC_STURN1)      = 1;

	/* CPLD, 16 bit */
  __REG (EMC_PHYS | EMC_SCONFIG3)    = 0x81;

#if defined (CONFIG_NAND_LPD)
  __REG (IOCON_PHYS | IOCON_MUXCTL7)  = IOCON_MUXCTL7_V;	/* A */
  __REG (IOCON_PHYS | IOCON_RESCTL7)  = IOCON_RESCTL7_V;	/* no pull */
  __REG (IOCON_PHYS | IOCON_MUXCTL14) = IOCON_MUXCTL14_V;     /* nCS0 normal */
  __REG16 (CPLD_FLASH)		     &= ~(CPLD_FLASH_NANDSPD); /* Fast NAND */
#endif

  __asm volatile ("tst %0, #0xf0000000\n\t"
		  "beq 1f\n\t"
		  "cmp %0, %1\n\t"
		  "movls r0, #0\n\t"
		  "movls pc, %0\n\t"
		  "1:" :: "r" (lr), "i" (SDRAM_BANK1_PHYS));

	/* SDRAM */
  __REG (EMC_PHYS | EMC_READCONFIG)  = EMC_READCONFIG_CMDDELAY;
  __REG (EMC_PHYS | EMC_DYNCFG0)     = SDRAM_CFG_SETUP;
  __REG (EMC_PHYS | EMC_DYNRASCAS0)  = SDRAM_RASCAS;
  __REG (EMC_PHYS | EMC_DYNCFG1)     = SDRAM_CFG_SETUP;
  __REG (EMC_PHYS | EMC_DYNRASCAS1)  = SDRAM_RASCAS;

  __REG (EMC_PHYS | EMC_PRECHARGE)   = NS_TO_HCLK(20);
  __REG (EMC_PHYS | EMC_DYNM2PRE)    = NS_TO_HCLK(60);
  __REG (EMC_PHYS | EMC_REFEXIT)     = NS_TO_HCLK(120);
  __REG (EMC_PHYS | EMC_DOACTIVE)    = NS_TO_HCLK(120);
  __REG (EMC_PHYS | EMC_DIACTIVE)    = NS_TO_HCLK(120);
  __REG (EMC_PHYS | EMC_DWRT)	     = NS_TO_HCLK(40);
  __REG (EMC_PHYS | EMC_DYNACTCMD)   = NS_TO_HCLK(120);
  __REG (EMC_PHYS | EMC_DYNAUTO)     = NS_TO_HCLK(120);
  __REG (EMC_PHYS | EMC_DYNREFEXIT)  = NS_TO_HCLK(120);
  __REG (EMC_PHYS | EMC_DYNACTIVEAB) = NS_TO_HCLK(40);
  __REG (EMC_PHYS | EMC_DYNAMICTMRD) = NS_TO_HCLK(40);

  __REG (EMC_PHYS | EMC_DYNMCTRL)    = ((1<<1)|(1<<0));
  usleep (200);
  __REG (EMC_PHYS | EMC_DYNMCTRL)    = (1<<1)|(1<<0)|(3<<7); /* NOP command */
  usleep (200);
  __REG (EMC_PHYS | EMC_DYNMCTRL)    = (1<<1)|(1<<0)|(2<<7); /* PRECHARGE ALL*/
  __REG (EMC_PHYS | EMC_DYNMREF)     = NS_TO_HCLK(100)/16 + 1;
  usleep (250);
  __REG (EMC_PHYS | EMC_DYNMREF)     = NS_TO_HCLK(7812)/16;
  __REG (EMC_PHYS | EMC_DYNMCTRL)    = (1<<1)|(1<<0)|(1<<7); /* MODE command */

  __REG (SDRAM_BANK0_PHYS | SDRAM_CHIP_MODE);
  __REG (SDRAM_BANK1_PHYS | SDRAM_CHIP_MODE);

  __REG (EMC_PHYS | EMC_DYNMCTRL)    = (1<<1)|(1<<0)|(0<<7); /* NORMAL */
  __REG (EMC_PHYS | EMC_DYNCFG0)     = SDRAM_CFG;
  __REG (EMC_PHYS | EMC_DYNCFG1)     = SDRAM_CFG;
  
#endif

  __asm volatile ("mov r0, #-1\t\n"
		  "mov pc, %0" : : "r" (lr));
}


/* initialize

   performs the rest of the hardware initialization that didn't have
   to be performed during the bootstrap phase.

*/

void __naked initialize_target (void)
{
  unsigned long lr;
  __asm volatile ("mov %0, lr" : "=r" (lr));

#if !defined (CONFIG_NAND_LPD)
	/* IOCON to clear special NAND modes.  These modes must be
	   controlled explicitly within driver code. */
  __REG (IOCON_PHYS | IOCON_MUXCTL7)  = IOCON_MUXCTL7_V;   /* A23,A22 */
  __REG (IOCON_PHYS | IOCON_MUXCTL14) = IOCON_MUXCTL14_V;  /* nCS0 normalize */
#endif

	/* CompactFlash, 16 bit */
  __REG (EMC_PHYS | EMC_SCONFIG2)    = 0x81;
  __REG (EMC_PHYS | EMC_SWAITWEN2)   = 2;
  __REG (EMC_PHYS | EMC_SWAITOEN2)   = 2;
  __REG (EMC_PHYS | EMC_SWAITRD2)    = 6;
  __REG (EMC_PHYS | EMC_SWAITPAGE2)  = 2;
  __REG (EMC_PHYS | EMC_SWAITWR2)    = 6;
  __REG (EMC_PHYS | EMC_STURN2)      = 1;

	/* CPLD, 16 bit */
  __REG (EMC_PHYS | EMC_SWAITWEN3)   = 2;
  __REG (EMC_PHYS | EMC_SWAITOEN3)   = 2;
  __REG (EMC_PHYS | EMC_SWAITRD3)    = 5;
  __REG (EMC_PHYS | EMC_SWAITPAGE3)  = 2;
  __REG (EMC_PHYS | EMC_SWAITWR3)    = 5;
  __REG (EMC_PHYS | EMC_STURN3)      = 2;

  __REG (BOOT_PHYS | BOOT_CS1OV)    &= ~(1<<0);

  __asm volatile ("mov pc, %0" : : "r" (lr));
}
