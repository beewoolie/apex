/* initialize.c
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

   Hardware initializations.  Some initializationmay be left to
   drivers, such as the serial interface initialization.

*/

#include <config.h>
#include <asm/bootstrap.h>
#include <service.h>
#include <sdramboot.h>

#include "hardware.h"
#include <debug_ll.h>

/* Static Memory Controller
   ------------------------

   In order to better support user needs WRT static memory regions, we
   do not change the default timing or alignment for any of the the
   SMC regions.  This means the reading from flash, for example, will
   be slow.  However, it does mean the greatest compatibility.

*/


/* Notes
   -----

   According to the Sharp LH7A400 user's guide, SDRAM initialization
   goes like this.  

   1) Wait 100us for device power to stabilize.  This isn't really a
      problem since we need to wait a couple of seconds for wakeup.
   2) Program SDRC mode registers for each chip select region
   3) Send NOP command.
   4) Wait 200us or another period according to the device datasheet.
   5) Program device mode register
   6) Set the refresh timer to 10 for a 10 HCLK refresh cycle
   7) Wait for eight refresh periods
   8) Reprogram refresh timer
   9) Program SDRAM mode registers
  10) Program the SDRC mode registers according to device mode
      register programming.
  11) Program initialize and MRS to 0 and set to GBLCFG for normal
      operation.  

   According to the LH7A400 EVB start code, here's what we need to do:

   1) Make sure CPU is in the correct mode, SVC32, etc.
   2) Initialize multiplexed pins
   3) Initialize clocks
   4) Initialize memory controllers
   5) Put CPU in SVC mode (or whatever)

   Micron SDRAM can be be initialized with this simple sequence.
   JEDEC is the same except that the wait is 200us and there are at least
   8 AUTO-REFRESH commands.  Changing to the JEDEC way only requires
   lengthening the delays. 

   1) Apply power
   2) Wait 100us while applying NOP/COMMAND INHIBIT
   3) PRECHARGE ALL
   4) AUTO-REFRESH *2, more is OK
   5) LOAD MODE

   For the delay, we use one of the system timers.

*/

/* CONFIG_SDRAM_CONTIGUOUS - This bit changes the way that the SDRAM
   controller maps SDRAM to physical addresses.  In systems with 32MiB
   of RAM, this will make it contguous.  In systems with more memory,
   the memory is not all contiguous though it is supposed to be
   accessible. */


#define SDRAM_CMD_NORMAL	(0x80000000)
#define SDRAM_CMD_PRECHARGEALL	(0x80000001)
#define SDRAM_CMD_MODE		(0x80000002)
#define SDRAM_CMD_NOP		(0x80000003)

// The charging time should be at least 100ns.  Longer is OK as will be
// the case for other HCLK frequencies.

#define SDRAM_REFRESH_CHARGING	(10)		// HCLKs, 10 //100MHz -> 100ns
#define SDRAM_REFRESH		(HCLK/64000 - 1) // HCLK/64KHz - 1

#if defined (USE_SLOW)
# define SDRAM_CHIP_MODE	(0x32<<10)	// CAS3 BURST4
#else
# define SDRAM_CHIP_MODE	(0x22<<10)	// CAS2 BURST4
#endif

// SRAM devices
//#define SMC_BCR0_V		(0x200039af)	// Bootflash
//#define SMC_BCR6_V		(0x1000fbe0)	// CompactFlash
//#define SMC_BCR7_V		(0x1000b2c2)	// CPLD & Ethernet

/* APEX oringinal  */
//#define SMC_BCR0_V		(0x200002a0)	// Bootflash
#define SMC_BCR1_V		(0x1f<<5)	// IO Barrier, slowest timing
//#define SMC_BCR6_V		(0x100003e0)	// CompactFlash
//#define SMC_BCR7_V		(0x100002c2)	// CPLD & Ethernet

/* LOLO 2.0.3 */
#define SMC_BCR0_V		(0x200002a2)	// Bootflash
#define SMC_BCR6_V		(0x100003e2)	// CompactFlash
#define SMC_BCR7_V		(0x10000102)	// CPLD & Ethernet

// Alternative timings.
//#define SMC_BCR0_V		(0x20000200)	// Bootflash
//#define SMC_BCR6_V		(0x100003e2)	// CompactFlash
//#define SMC_BCR7_V		(0x10000102)	// CPLD & Ethernet

#if defined (CONFIG_SDRAM_CONTIGUOUS)
#define SDRAM_MODE_SROMLL	(1<<5)
#else
#define SDRAM_MODE_SROMLL	(0)
#endif

// SDRAM
#if defined (USE_SLOW)
# define _SDRAM_MODE_SETUP	(0x00320008)	// CAS3, RAStoCAS3, 32 bit
# define _SDRAM_MODE		(0x01320008)	// Add autoprecharging
#else
# define _SDRAM_MODE_SETUP	(0x00210008)	// CAS2, RAStoCAS2, 32 bit
# define _SDRAM_MODE		(0x01210008)	// Add autoprecharging
//# define SDRAM_MODE		(0x01210028)	// Add autoprecharging SROMLL
#endif

#define SDRAM_MODE_SETUP	_SDRAM_MODE_SETUP | SDRAM_MODE_SROMLL
#define SDRAM_MODE		_SDRAM_MODE	  | SDRAM_MODE_SROMLL


/* usleep

   this function accepts a count of microseconds and will wait at
   least that long before returning.  It depends on the timer being
   setup.

   Note that this function is neither __naked nor static.  It is
   available to the rest of the application as is.

   The fundamental timer frequency is 508469 Hz which is a period of
   1.9667us.  The rounding (us - us/2) means that the value cannot
   round to zero.  The compiler is efficient and can make the roundoff
   in a single instruction.

 */

void __section (.bootstrap) usleep (unsigned long us)
{
  __asm volatile ("str %1, [%0, #8]\n\t"
		  "str %2, [%0, #0]\n\t"
		  "str %3, [%0, #8]\n\t"
	       "0: ldr %2, [%0, #4]\n\t"
	          "tst %2, %4\n\t"
		  "beq 0b\n\t"
		  "mov pc, lr\n\t"
		  :
		  : "r" (TIMER2_PHYS),
		    "r" (0),
		    "r" (us - us/2),
		    "r" ((1<<7)|(1<<3)), /* Enable, Free, 508 KHz */
		    "r" (0x8000));
}


/* initialize_bootstrap

   performs vital SDRAM initialization as well as some other memory
   controller initializations.  It will perform no work if we are
   already running from SDRAM. 

   The return value is true if SDRAM has been initialized and false if
   this initialization has already been performed.  Note that the
   non-SDRAM initializations are performed regardless of whether or
   not we're running in SDRAM.

*/

void __naked __section (.bootstrap) initialize_bootstrap (void)
{
  unsigned long lr;
  __asm volatile ("mov %0, lr" : "=r" (lr));

#if defined (CONFIG_DEBUG_LL)
  UART_BRCON = 0x3;
  UART_FCON = UART_FCON_FEN | UART_FCON_WLEN8;
  UART_INTEN = 0x00; /* Mask interrupts */
  UART_CON = UART_CON_ENABLE;

  PUTC_LL('A');
#endif

	/* Enable Asynchronous Bus mode, NotFast and iA bits */
  {
    unsigned long l;
    __asm volatile ("mrc	p15, 0, %0, c1, c0, 0\n\t"
		    "orr	%0, %0, #(1<<31)|(1<<30)\n\t"
//		    "bic	%0, %0, #(1<<31)|(1<<30)\n\t"
		    "mcr	p15, 0, %0, c1, c0, 0"
		    : "=r" (l));
  }

	/* Set the running clock speed */
  CSC_CLKSET = CSC_CLKSET_V;

	/* Enable PCMCIA.  This is a workaround for a buggy CPLD on
	   the LPD boards.  Apparently, the PCMCIA signals float when
	   disabled which breaks the CPLD handling of chip
	   selects. */
  SMC_PCMCIACON |= 0x3;		/* Enable both PCMCIA slots */
  SMC_BCR0 = SMC_BCR0_V;
  SMC_BCR1 = SMC_BCR1_V;
  SMC_BCR6 = SMC_BCR6_V;
  SMC_BCR7 = SMC_BCR7_V;

  __asm volatile ("cmp %0, %1\n\t"
#if defined (CONFIG_SDRAMBOOT_REPORT)
		  "movhi r0, #1\n\t"
		  "strhi r0, [%2]\n\t"
#endif
		  "movhi r0, #0\n\t"
		  "movhi pc, %0\n\t"
		  "1:" :: "r" (lr), "i" (SDRAM_BANK0_PHYS)
#if defined (CONFIG_SDRAMBOOT_REPORT)
		  , "r" (&fSDRAMBoot) 
#endif
		        : "r0");

  PUTC_LL ('S');

	/* Initialize SDRAM */
  SDRC_SDCSC0 = SDRAM_MODE_SETUP;
  SDRC_GBLCNFG = SDRAM_CMD_NOP;
  usleep (200);
  SDRC_GBLCNFG = SDRAM_CMD_PRECHARGEALL;
  SDRC_RFSHTMR = SDRAM_REFRESH_CHARGING;
  usleep (8);
  SDRC_RFSHTMR = SDRAM_REFRESH;
  SDRC_GBLCNFG = SDRAM_CMD_MODE;
#if defined (CONFIG_SDRAM_BANK0)
  __REG (SDRAM_BANK0_PHYS + SDRAM_CHIP_MODE);
#endif
#if defined (CONFIG_SDRAM_BANK1)
  __REG (SDRAM_BANK1_PHYS + SDRAM_CHIP_MODE);
#endif
#if defined (CONFIG_SDRAM_BANK2)
  __REG (SDRAM_BANK2_PHYS + SDRAM_CHIP_MODE);
#endif
  SDRC_GBLCNFG = SDRAM_CMD_NORMAL;
  SDRC_SDCSC0 = SDRAM_MODE;
  
  PUTC_LL ('s');

#if defined (CONFIG_SDRAMBOOT_REPORT)
  barrier ();
  fSDRAMBoot = 0;
#endif

  __asm volatile ("mov r0, #-1\t\n"
		  "mov pc, %0" : : "r" (lr));
}


/* target_init

   performs the rest of the hardware initialization that didn't have
   to be performed during the bootstrap phase.

*/

void __naked target_init (void)
{
  unsigned long lr;
  __asm volatile ("mov %0, lr" : "=r" (lr));

#if defined (CONFIG_MACH_LPD7A404)
  /* PE4 must be driven high to disable the CPLD JTAG & prevent CPLD crash */
  GPIO_PEDD &= ~(1<<4);
  GPIO_PED  |=  (1<<4);

  /* PC6 must be driven high to disable the NAND_nCE PCN-285 */
  GPIO_PCDD &= ~(1<<6);
  GPIO_PCD  |=  (1<<6);
#endif

  __asm volatile ("mov pc, %0" : : "r" (lr));
}


static void target_release (void)
{
  /* Flash is enabled for the kernel */
  CPLD_FLASH |=  CPLD_FLASH_FL_VPEN;
}

static __service_0 struct service_d lh7a40x_target_service = {
  .init    = target_init,
  .release = target_release,
};
