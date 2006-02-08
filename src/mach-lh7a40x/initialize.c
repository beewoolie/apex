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

   While we must be circumspect about reprogramming the SRAM
   controller, especially the NOR flash region when we are booting
   from it, the default timings are quite slow.  So, we still
   reprogram the SRAM controller regions, but we have to have special
   settings for each target.

   The code could check the bus width and leave the settings intact if
   there is a mismatch.  Still, the boot loader won't do anything if
   it flubs that part, so it seems better to go for speed.

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

#if defined (CONFIG_MACH_TROUNCER)
# define SMC_BCR0_V		(0x1000fce1)	// Bootflash
#endif

#if defined (CONFIG_MACH_LPD7A40X)
# define SMC_BCR0_V		(0x200002a2)	// Bootflash
# define SMC_BCR6_V		(0x100003e2)	// CompactFlash
# define SMC_BCR7_V		(0x10000102)	// CPLD & Ethernet
#endif

// Alternative timings.
//#define SMC_BCR0_V		(0x20000200)	// Bootflash
//#define SMC_BCR6_V		(0x100003e2)	// CompactFlash
//#define SMC_BCR7_V		(0x10000102)	// CPLD & Ethernet

#define SDCSC_CASLAT(v)		((v - 1)<<16) /* CAS latency */
#define SDCSC_RASTOCAS(v)	(v<<20) /* RAS to CAS latency */
#define SDCSC_BANKCOUNT_2	(0<<3) /* BankCount, 2 bank devices */
#define SDCSC_BANKCOUNT_4	(1<<3) /* BankCount, 4 bank devices */
#define SDCSC_EBW_16		(1<<2) /* ExternalBuswidth, 16 bits w/burst length of 8 */
#define SDCSC_EBW_32		(0<<2) /* ExternalBusWidth, 32 bits w/burst length of 4 */
#define SDCRC_AUTOPRECHARGE	(1<24)

#if defined (CONFIG_SDRAM_CONTIGUOUS)
#define SDRAM_MODE_SROMLL	(1<<5)
#else
#define SDRAM_MODE_SROMLL	(0)
#endif

#if defined (USE_SLOW)
# define SDRAM_CAS		3
# define SDRAM_RAS		3
#else
# define SDRAM_CAS		2
# define SDRAM_RAS		2
#endif

#define SDRAM_MODE_SETUP	( ((SDRAM_CAS - 1)<<16)\
				 | (SDRAM_RAS << 20)\
				 | SDCSC_EBW_32\
				 | SDCSC_BANKCOUNT_4\
				 | SDRAM_MODE_SROMLL)
#define SDRAM_MODE		(SDRAM_MODE_SETUP | SDCRC_AUTOPRECHARGE)


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

   The maximum range of this timer is about 64ms.

 */

void __section (.bootstrap) usleep (unsigned long us)
{
  unsigned long c = (us - us/2);
  __asm volatile ("str %2, [%1, #8]\n\t"
		  "str %0, [%1, #0]\n\t"
		  "str %3, [%1, #8]\n\t"
	       "0: ldr %0, [%1, #4]\n\t"
	          "tst %0, %4\n\t"
		  "beq 0b\n\t"
		  : "+r" (c)
		  : "r" (TIMER2_PHYS),
		    "r" (0),
		    "r" ((1<<7)|(1<<3)), /* Enable, Free, 508 KHz */
		    "I" (0x8000)
		  : "cc");
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

	/* Set the running clock speed.  This will increase the HCLK
	   (bus) speed but not let the FCLK (CPU) speed be independent
	   of HCLK until the CLOCKMODE is changed in the CP15 control
	   register. */
  CSC_CLKSET = CSC_CLKSET_V;

	/* There are two bits that control bus clocking modes.
	     iA (1<<31) Asynchronous clock select
	     nF (1<<30) notFastBus
	   The valid combinations are as follows:

	     iA nF
	      0  0	FastBus; mode after system reset
	      0  1	Synchronous
	      1  1	Asynchronous

	    From the ARM 922 TRM, Chapter 5,

	    FastBus mode, GCLK sources from BCLK and FCLK is
	      ignored.  The BCLK signal controls both the ARM core
	      as well as the AMBA AHB interface.
	    Synchronous mode, GCLK is sourced from BCLK or FCLK.
	      FCLK must be faster than BCLK.
	      FCLK must be an integer multiple of BCLK.
	    Asynchronous mode, GCLK is sourced from BCLK or FCLK.
	      FCLK must be faster than BCLK.

	    Use either synchronous or asynchronous mode for normal
	    system operation.  Synchronous is more efficient because
	    there is no cycle penalty for synchronizing the CPU to the
	    bus.

  */

  {
    unsigned long l;
    __asm volatile ("mrc	p15, 0, %0, c1, c0, 0\n\t"
#if (CLOCKMODE == 's')
		    "bic	%0, %0, #(1<<31)\n\t"
		    "orr	%0, %0, #(1<<30)\n\t"
#endif
#if (CLOCKMODE == 'a')
		    "orr	%0, %0, #(1<<31)|(1<<30)\n\t"
#endif
#if (CLOCKMODE == 'f')
		    "bic	%0, %0, #(1<<31)|(1<<30)\n\t"
#endif
		    "mcr	p15, 0, %0, c1, c0, 0"
		    : "=&r" (l));
  }

	/* Enable PCMCIA.  This is a workaround for a buggy CPLD on
	   the LPD boards.  Apparently, the PCMCIA signals float when
	   disabled which breaks the CPLD handling of chip
	   selects. */
  SMC_PCMCIACON |= 0x3;		/* Enable both PCMCIA slots */

#if defined (SMC_BCR0_V)
  SMC_BCR0 = SMC_BCR0_V;
#endif
#if defined (SMC_BCR1_V)
  SMC_BCR1 = SMC_BCR1_V;
#endif
#if defined (SMC_BCR6_V)
  SMC_BCR6 = SMC_BCR6_V;
#endif
#if defined (SMC_BCR7_V)
  SMC_BCR7 = SMC_BCR7_V;
#endif

  __asm volatile ("cmp %0, %1\n\t"
#if defined (CONFIG_SDRAMBOOT_REPORT)
		  "movhi r0, #1\n\t"
		  "strhi r0, [%2]\n\t"
#endif
		  "movhi r0, #0\n\t"
		  "movhi pc, %0\n\t"
		  "1:" :: "r" (lr), "I" (SDRAM_BANK0_PHYS)
#if defined (CONFIG_SDRAMBOOT_REPORT)
		  , "r" (&fSDRAMBoot) 
#endif
		  : "cc");

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

static void target_init (void)
{
#if defined (CONFIG_MACH_LPD7A404)
  /* PE4 must be driven high to disable the CPLD JTAG & prevent CPLD crash */
  GPIO_PEDD &= ~(1<<4);
  GPIO_PED  |=  (1<<4);

  /* PC6 must be driven high to disable the NAND_nCE PCN-285 */
  GPIO_PCDD &= ~(1<<6);
  GPIO_PCD  |=  (1<<6);
#endif
}


static void target_release (void)
{
#if defined (CPLD_FLASH)
  /* Flash is enabled for the kernel */
  CPLD_FLASH |= CPLD_FLASH_FL_VPEN;
#endif
}

static __service_0 struct service_d lh7a40x_target_service = {
  .init    = target_init,
  .release = target_release,
};
