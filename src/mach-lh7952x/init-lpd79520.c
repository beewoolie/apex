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

/*
	SDRAM configuration according to JEDEC spec

	WAIT 200us
	NOP->SDRAM
	WAIT 200us
	PRECHARGE_ALL->SDRAM
	MODE_REGISTER_SET->SDRAM_EXTENDED_MODE
	MODE_REGISTER_SET->SDRAM
	WAIT 200 cycles 
	PRECHARGE_ALL->SDRAM
	AUTO_REFRESH->SDRAM *2
	MODE_REGISTER_SET->SDRAM

	SDRAM configuration according to u-boot code

	NOP->SDRAM
	WAIT 200us
	PRECHARGE->SDRAM
	AUTO_REFRESH->SDRAM *2
	LOAD_MODE->SDRAM
	AUTO_REFRESH->SDRAM *8
	NORMAL->SDRAM

	SDRAM initialization according to Sharp LH79520 documentation

	WAIT 200ps
	PRECHARGE_ALL->SDRAM
	AUTO_REFRESH->SDRAM *2   [lh79520 has a auto-refresh mode]
	LOAD_MODE->SDRAM (Program the SDRAM device's LOAD MODE register)

*/

#include <config.h>
#include <asm/bootstrap.h>

#include "hardware.h"

//#define USE_SLOW

#if defined (USE_SLOW)
//RAS3 CAS3
# define SDRC_CONFIG_V\
	(1<<24)|(3<<22)|(3<<20)|(0<<19)|(1<<18)|(0<<17)|(1<<7)|(1<<3)
#else
//RAS2 CAS2
# define SDRC_CONFIG_V\
	(1<<24)|(2<<22)|(2<<20)|(0<<19)|(1<<18)|(0<<17)|(1<<7)|(1<<3)
#endif

#if defined (USE_SLOW)
# define SDRAM_CHIP_MODE	(0x32<<12)	// CAS3 BURST4
#else
# define SDRAM_CHIP_MODE	(0x22<<12)	// CAS2 BURST4
#endif

#define SDRC_COMMAND_NORMAL	((0<<0)|(1<<3) |(1<<2))
#define SDRC_COMMAND_PRECHARGE	(1<<0)
#define SDRC_COMMAND_MODE	(2<<0)
#define	SDRC_COMMAND_NOP	(3<<0)
#define	SDRC_BUSY		(1<<5)


/* usleep

   this function accepts a count of microseconds and will wait at
   least that long before returning.  The timer clock must be
   activated by the initialization code before using usleep.

   Note that this function not static.  It is available to the rest of
   the application as is.

   The timer interval is (309657600/6)/256 which is about 4.96us.  We
   approximate this by dividing by four which means we are about 5%
   longer than requested.

 */

void __naked __section(bootstrap) usleep (unsigned long us)
{
  unsigned long lr;
  __asm volatile ("mov %0, lr" : "=r" (lr));

  __asm ("str %1, [%0, #8]\n\t"	/* Stop timer */
	 "str %2, [%0, #0]\n\t"
	 "str %3, [%0, #8]\n\t"
      "0: ldr %2, [%0, #4]\n\t"
	 "tst %2, %4\n\t"
	 "beq 0b\n\t"
	 :
	 : "r" (TIMER0_PHYS), 
	   "r" (0),
	   "r" ((unsigned long) 0x8000 - (us - us/4)), /* timer counts up */
	   "r" (TIMER_ENABLE | TIMER_SCALE_256),
	   "i" (0x8000)
	 );

  __asm volatile ("mov pc, %0" : : "r" (lr));
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
   *** long as the loader never runs from high BCR0 memory.

*/

void __naked __section(bootstrap) initialize_bootstrap (void)
{
  unsigned long lr;
  __asm volatile ("mov %0, lr" : "=r" (lr));


	/* Setup HCLK, FCLK and peripheral clocks */
  __REG (RCPC_PHYS | RCPC_CTRL) |= RCPC_CTRL_UNLOCK;
  __REG (RCPC_PHYS | RCPC_CPUCLKPRESCALE) = RCPC_CPUCLKPRESCALE_V;
  __REG (RCPC_PHYS | RCPC_HCLKCLKPRESCALE) = RCPC_HCLKCLKPRESCALE_V;
  __REG (RCPC_PHYS | RCPC_CORECLKCONFIG) = RCPC_CORECLKCONFIG_V;
  __REG (RCPC_PHYS | RCPC_AHBCLKCTRL) &= ~RCPC_AHBCLK_SDC;
  /* Enable timer clock so that we can handle SDRAM setup  */
  __REG (RCPC_PHYS | RCPC_PERIPHCLKCTRL) &= ~RCPC_PERIPHCLK_T01;

  __REG (RCPC_PHYS | RCPC_CTRL) &= ~RCPC_CTRL_UNLOCK;

  __REG (SDRC_PHYS | SDRC_REFTIMER) = SDRC_REFTIMER_V; /* Do this early */

  /* Stop watchdog? */

  /* *** 0x17ef was good enough for JTAG */
  __REG (IOCON_PHYS | IOCON_MEMMUX) = 0x3fff; /* 32 bit, SDRAM, all SRAM */ 

  __REG (SMC_PHYS | SMC_BCR0) = 0x100020ef; /* NOR flash */
  __REG (SMC_PHYS | SMC_BCR4) = 0x10007580; /* CompactFlash */
  __REG (SMC_PHYS | SMC_BCR5) = 0x100034c0; /* CPLD */

  __asm volatile ("tst %0, #0xf0000000\n\t"
		  "beq 1f\n\t"
		  "cmp %0, %1\n\t"
		  "movls r0, #0\n\t"
		  "movls pc, %0\n\t"
		"1:" :: "r" (lr), "i" (SDRAM_BANK1_PHYS));


  /* SDRAM Initialization

     I spent some time exploring what could be removed from the
     initialization sequence.  Below is the result.  It appears that
     the LH79520 handles all of the AUTO_REFRESH cycles.  I never
     tried breaking it by clearing the AP bit in the CFG0 register.
     Note that there are no delays, one and only one NOP is required,
     and there is no precharge after setting the mode.
  */

  __REG (SDRC_PHYS | SDRC_CONFIG1) = SDRC_COMMAND_NOP;
  usleep (200);
  __REG (SDRC_PHYS | SDRC_CONFIG1) = SDRC_COMMAND_PRECHARGE;
  usleep (200);
  __REG (SDRC_PHYS | SDRC_CONFIG1) = SDRC_COMMAND_MODE;
  __REG (SDRAM_BANK0_PHYS | SDRAM_CHIP_MODE);
  __REG (SDRAM_BANK1_PHYS | SDRAM_CHIP_MODE);
  __REG (SDRC_PHYS | SDRC_CONFIG0) = SDRC_CONFIG_V;
  __REG (SDRC_PHYS | SDRC_CONFIG1) = SDRC_COMMAND_NORMAL;

  __asm volatile ("mov r0, #-1\t\n"
		  "mov pc, %0" : : "r" (lr));
}


/* target_init

   performs the rest of the hardware initialization that didn't have
   to be performed during the bootstrap phase.

*/

static void target_init (void)
{
  unsigned long lr;
  __asm volatile ("mov %0, lr" : "=r" (lr));
  __asm volatile ("mov pc, %0" : : "r" (lr));
}

static __service_0 struct service_d lh79524_target_service = {
//  .init    = target_init,
//  .release = target_release,
};
