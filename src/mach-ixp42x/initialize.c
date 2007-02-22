/* initialize.c

   written by Marc Singer
   14 Jan 2005

   Copyright (C) 2005 Marc Singer

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

   Hardware initializations for the ixp42x.  Some initializations may
   be left to drivers, such as the serial interface and timer.

   The COPROCESSOR_WAIT macro comes from redboot.  It isn't clear to
   me that we need to do this, though the cache and tlb flushing might
   require it.

   o Cache Lockdown.  In order to allow reinitialization of SDRAM when
     APEX is not the primary boot loader, we use the I-cache to hold a
     portion of the APEX code while the SDRAM controller is off-line.
     The constraints of the Xscale architecture require that we only
     lock code that we are not executing.  So, we copy the first N-KiB
     of APEX to another place in memory, lock it down, jump to it,
     perform the updates, jump back, and unlock the cache.

*/

#include <config.h>
#include <asm/bootstrap.h>
#include <service.h>
#include <debug_ll.h>
#include <sdramboot.h>
//#include "../src/arch-arm/entry/mmu.h"

#include "hardware.h"
#include "sdram.h"

/* usleep

   this function accepts a count of microseconds and will wait at
   least that long before returning.  The base counter rate is
   66.66MHz or a 15ns cycle time.  1us takes 67 counts.

   The 32 bit counter wraps after about 64s.

   Note that this function is neither __naked nor static.  It is
   available to the rest of APEX.

 */

void __section (.bootstrap) usleep (unsigned long us)
{
  unsigned long s;
  __asm volatile ("ldr %0, [%1]" : "=r" (s) : "r" (OST_TS_PHYS));
  us *= 67;

  __asm volatile ("0: ldr %0, [%1]\n\t"
		     "sub %0, %0, %2\n\t"
		     "cmp %0, %3\n\t"
		     "bls 0b\n\t"
		  : : "r" (0),
		      "r" (OST_TS_PHYS),
		      "r" (s),
		      "r" (us)
		  );
}


/* initialize_bootstrap

   performs the critical bootstrap initializaton before APEX is copied
   to SDRAM.

   It *should* perform no work if we are already running from SDRAM.

   The return value is true if SDRAM has been initialized and false if
   this initialization has already been performed.  Note that the
   non-SDRAM initializations are performed regardless of whether or
   not we're running in SDRAM.

*/

void __naked __section (.bootstrap) initialize_bootstrap (void)
{
  unsigned long lr;

#if 0
  __asm volatile ("mov r1, #0xc8000000\n\t"
		  "add r1, r1, #0x4000\n\t"
		  "ldr r0, [r1, #4]\n\t"
		  "bic r0, r0, #0xf\n\t"
		  "str r0, [r1,#4]\n\t"
		  "ldr r0, [r1, #0]\n\t"
		  "bic r0, r0, #0xf\n\t"
		  "orr r0, r0, #5\n\t"
		  "0: str r0, [r1, #0]\n\t"
		  "eor r0, r0, #0xf\n\t"
		  "b 0b");
#endif

  __asm volatile ("mov %0, lr" : "=r" (lr));

#if defined (CONFIG_STARTUP_UART)
  UART_LCR  = UART_LCR_WLS_8 | UART_LCR_STB_1 | UART_LCR_DLAB;
  UART_DLL  = 8;	// divisor_l;
  UART_DLH  = 0;	// divisor_h;
  UART_LCR  = UART_LCR_WLS_8 | UART_LCR_STB_1;

  UART_IER  = UART_IER_UUE;	/* Enable UART, mask all interrupts */
				/* Clear interrupts? */
  PUTC('A');
#endif

  _L(LEDf);			/* Start with all on */
  GPIO_ER &= ~0xf;		/* Enable LEDs as outputs */

  /* *** FIXME: this CPSR and CP15 manipulation should not be
     *** necessary as we don't allow warm resets.  This is here until
     *** we get a working loader. */

	/* Disable interrupts and set supervisor mode */
  __asm volatile ("msr cpsr, %0" : : "r" ((1<<7)|(1<<6)|(0x13<<0)));

	/* Drain write buffer */
  __asm volatile ("mcr p15, 0, r0, c7, c10, 4" : : : "r0");
  COPROCESSOR_WAIT;

	/* Invalidate caches (I&D) and branch buffer (BTB) */
  __asm volatile ("mcr p15, 0, r0, c7, c7, 0" : : : "r0");
  COPROCESSOR_WAIT;

	/* Invalidate TLBs (I&D) */
  __asm volatile ("mcr p15, 0, r0, c8, c7, 0" : : : "r0");
  COPROCESSOR_WAIT;

	/* Disable write buffer coalescing */
#if 1
  {
    unsigned long v;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 1\n\t"
		    "orr %0, %0, #(1<<0)\n\t"
		    "mcr p15, 0, %0, c1, c0, 1"
		    : "=r" (v));
  }

  COPROCESSOR_WAIT;
#endif

#if 0
	/* Fixup the CP15 control word.  This is done for the cases
	   where we are bootstrapping from redboot which does not
	   cleanup before jumping to code.  */
  /* This isn't done here any more since we can disable the MMU in the
     reset() code with a configuration option.  Caches are enabled
     elsewhere as well based on configuration option. */
  __asm volatile ("mrc p15, 0, r0, c1, c0, 0\n\t"
		  "bic r0, r0, #(1<<0)\n\t" /* Disable MMU */
//		  "bic r0, r0, #(1<<1)\n\t" /* Disable alignment check */
//		  "orr r0, r0, #(1<<4)\n\t" /* Enable write buffer */
//		  "bic r0, r0, #(1<<4)\n\t" /* Disable write buffer */
//		  "orr r0, r0, #(1<<12)\n\t" /* Enable instruction cache */
//		  "bic r0, r0, #(1<<12)\n\t" /* Disable instruction cache */
		  "mcr p15, 0, r0, c1, c0, 0" : : : "r0");
  COPROCESSOR_WAIT;
#endif

	/* Configure flash access */
#if 0
  /* The following is the original flash timing as set by RedBoot.
     These are very pessimistic timings, about 6.6us per access. */
  EXP_TIMING_CS0 = 0
    | (( 3 & EXP_T1_MASK)	<<EXP_T1_SHIFT)
    | (( 3 & EXP_T2_MASK)	<<EXP_T2_SHIFT)
    | ((15 & EXP_T3_MASK)	<<EXP_T3_SHIFT)
    | (( 3 & EXP_T4_MASK)	<<EXP_T4_SHIFT)
    | ((15 & EXP_T5_MASK)	<<EXP_T5_SHIFT)
    | ((15 & EXP_CNFG_MASK)	<<EXP_CNFG_SHIFT)	/* 16MiB window */
    | (( 0 & EXP_CYC_TYPE_MASK)	<<EXP_CYC_TYPE_SHIFT)	/* Intel cycling */
    | EXP_BYTE_RD16
    | EXP_WR_EN
    | EXP_CS_EN
    ;
#endif

  /* Expansion bus clock is 66MHz for a 15ns cycle time.  The flash
     memory has a 120ns minimum read timing requirement.  With more
     aggressive timing we will see about 180ns per access.  This took
     the read of the ramdisk from 4300ms to 2600ms. */
#if 1
  EXP_TIMING_CS0 = 0
    | (( 0 & EXP_T1_MASK)	<<EXP_T1_SHIFT)
    | (( 0 & EXP_T2_MASK)	<<EXP_T2_SHIFT)
    | (( 7 & EXP_T3_MASK)	<<EXP_T3_SHIFT)
    | (( 0 & EXP_T4_MASK)	<<EXP_T4_SHIFT)
    | (( 0 & EXP_T5_MASK)	<<EXP_T5_SHIFT)
    | ((15 & EXP_CNFG_MASK)	<<EXP_CNFG_SHIFT)	/* 16MiB window */
    | (( 0 & EXP_CYC_TYPE_MASK)	<<EXP_CYC_TYPE_SHIFT)	/* Intel cycling */
    | EXP_BYTE_RD16
    | EXP_WR_EN
    | EXP_CS_EN
    ;
#endif

  PUTC_LL ('\r');PUTC_LL ('\n');
  PUTHEX_LL (*(unsigned long *) 0x50000000);
  PUTC_LL ('\r');PUTC_LL ('\n');
  PUTHEX_LL (EXP_CNFG0);
  PUTC_LL ('\r');PUTC_LL ('\n');
  PUTHEX_LL (EXP_CNFG1);
  PUTC_LL ('\r');PUTC_LL ('\n');



  if (EXP_CNFG0 & (1<<31)) {	/* Exit now if executing in SDRAM */
    PUTC_LL ('b');
    _L(LED1);

    /* Boot mode */
    __asm volatile ("cmp %0, %2\n\t"
		    "bls 1f\n\t"
		    "cmp %0, %1\n\t"
		    "movls r0, #0\n\t"
		    "movls pc, %0\n\t"
		    "1:": : "r" (lr), "i" (0x40000000), "i" (256*1024*1024));

    PUTC_LL ('f');
    /* Jump to the proper flash address before disabling boot mode */

    {
      __asm volatile ("add %0, %0, %1\n\t"
		      "add pc, pc, %1"
		      : "=r" (lr) : "r" (0x50000000 - 4));
    }

	/* Disable interrupts and set supervisor mode */
//    __asm volatile ("msr cpsr, %0" : : "r" ((1<<7)|(1<<6)|(0x13<<0)));

	/* Drain write buffer */
    __asm volatile ("mcr p15, 0, r0, c7, c10, 4" : : : "r0");
    COPROCESSOR_WAIT;

    PUTC_LL ('+');
    EXP_CNFG0 &= ~EXP_CNFG0_MEM_MAP; /* Disable boot-mode for EXP_CS0  */
    PUTC_LL ('#');
  }
  else {			/* Exit now if executing in SDRAM */
    PUTC_LL ('n');
    _L(LED2);

#if defined (CONFIG_SDRAMBOOT_REPORT)
    fSDRAMBoot = 1;
#endif

    /* Non-boot mode */
    __asm volatile ("cmp %0, %1\n\t"
		    "movls r0, #0\n\t"
		    "movls pc, %0\n\t"
		    : : "r" (lr), "i" (0x40000000));
  }

  PUTC_LL ('X');

  usleep (1000);		/* Wait for CPU to stabilize SDRAM signals */

  SDR_CONFIG = SDR_CONFIG_RAS3 | SDR_CONFIG_CAS3 | SDR_CONFIG_CHIPS;
  SDR_REFRESH = 0;		/* Disable refresh */
  SDR_IR = SDR_IR_NOP;
  usleep (200);			/* datasheet: maintain 200us of NOP */

  /* 133MHz timer cycle count, 0x81->969ns == ~1us */
  /* datasheet: not clear what the refresh requirement is.  */
  SDR_REFRESH = 0x81;		/* *** FIXME: calc this */

  SDR_IR = SDR_IR_PRECHARGE_ALL;

  usleep (1);			/* datasheet: wait for Trp (<=20ns)
				   before starting AUTO REFRESH */

  /* datasheet: needs at least 8 refresh (bus) cycles.  Timing diagram
     shows only two AUTO_REFRESH commands, each is Trc (20ns) long. */

  SDR_IR = SDR_IR_AUTO_REFRESH;
  usleep (1);
  SDR_IR = SDR_IR_AUTO_REFRESH;
  usleep (1);

  SDR_IR = SDR_IR_MODE_CAS3;
  SDR_IR = SDR_IR_NORMAL;
  usleep (1);			/* Must wait for 3 CPU cycles before
				   SDRAM access */

  _L(LED3);

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
  _L(LED6);

  //  EXP_CNFG0 &= ~EXP_CNFG0_MEM_MAP; /* Disable boot-mode for EXP_CS0  */
  //  __REG(EXP_PHYS + 0x28) |= (1<<15); /* Undocumented, but set in redboot */

#if 0
    /* GPIO initialization for the Intel dev board */

  GPIO_OUT_DISABLE (GPIO_I_SLIC_A);
  GPIO_INT_TYPE    (GPIO_I_SLIC_A, GPIO_INT_TYPE_ACTIVELO);
  GPIO_OUT_DISABLE (GPIO_I_SLIC_B);
  GPIO_INT_TYPE    (GPIO_I_SLIC_B, GPIO_INT_TYPE_ACTIVELO);
  GPIO_OUT_DISABLE (GPIO_I_IDE);
  GPIO_INT_TYPE    (GPIO_I_IDE, GPIO_INT_TYPE_ACTIVELO);
  GPIO_OUT_SET	   (GPIO_I_IORESET);
  GPIO_OUT_ENABLE  (GPIO_I_IORESET);
  GPIO_OUT_SET	   (GPIO_I_SPI_CS1);
  GPIO_OUT_ENABLE  (GPIO_I_SPI_CS1);
  GPIO_OUT_SET	   (GPIO_I_SPI_CS0);
  GPIO_OUT_ENABLE  (GPIO_I_SPI_CS0);
  GPIO_OUT_CLEAR   (GPIO_I_SPI_SCK);
  GPIO_OUT_ENABLE  (GPIO_I_SPI_SCK);
  GPIO_OUT_CLEAR   (GPIO_I_SPI_SDI);
  GPIO_OUT_ENABLE  (GPIO_I_SPI_SDI);
  GPIO_OUT_DISABLE (GPIO_I_SPI_SDI); /* Really? */
  GPIO_ISR = 0xfff;		/* Clear all pending interrupts */

#endif

#if defined (CONFIG_MACH_NSLU2)

  EXP_TIMING_CS4 = 0
    | (( 3 & EXP_T1_MASK)	<<EXP_T1_SHIFT)
    | (( 3 & EXP_T2_MASK)	<<EXP_T2_SHIFT)
    | ((15 & EXP_T3_MASK)	<<EXP_T3_SHIFT)
    | (( 3 & EXP_T4_MASK)	<<EXP_T4_SHIFT)
    | ((15 & EXP_T5_MASK)	<<EXP_T5_SHIFT)
    | ((0 & EXP_CNFG_MASK)	<<EXP_CNFG_SHIFT)	/* 512 b window */
    | EXP_WR_EN
    | EXP_CS_EN
    | EXP_BYTE_EN;

  EXP_TIMING_CS5 = 0
    | (( 3 & EXP_T1_MASK)	<<EXP_T1_SHIFT)
    | (( 3 & EXP_T2_MASK)	<<EXP_T2_SHIFT)
    | ((15 & EXP_T3_MASK)	<<EXP_T3_SHIFT)
    | (( 3 & EXP_T4_MASK)	<<EXP_T4_SHIFT)
    | ((15 & EXP_T5_MASK)	<<EXP_T5_SHIFT)
    | ((0 & EXP_CNFG_MASK)	<<EXP_CNFG_SHIFT)	/* 512 b window */
    | EXP_WR_EN
    | EXP_CS_EN
    | EXP_BYTE_EN;

  EXP_TIMING_CS7 = 0
    | (( 3 & EXP_T1_MASK)	<<EXP_T1_SHIFT)
    | (( 3 & EXP_T2_MASK)	<<EXP_T2_SHIFT)
    | ((15 & EXP_T3_MASK)	<<EXP_T3_SHIFT)
    | (( 3 & EXP_T4_MASK)	<<EXP_T4_SHIFT)
    | ((15 & EXP_T5_MASK)	<<EXP_T5_SHIFT)
    | ((0 & EXP_CNFG_MASK)	<<EXP_CNFG_SHIFT)	/* 512 b window */
    | EXP_WR_EN
    | EXP_MUX_EN
    | EXP_CS_EN;

  //    *IXP425_EXP_CS4 = (EXP_ADDR_T(3) | EXP_SETUP_T(3) | EXP_STROBE_T(15) | EXP_HOLD_T(3) | EXP_RECOVERY_T(15) | EXP_SZ_512 | EXP_WR_EN | EXP_CS_EN | EXP_BYTE_EN);
  //    *IXP425_EXP_CS5 = (EXP_ADDR_T(3) | EXP_SETUP_T(3) | EXP_STROBE_T(15) | EXP_HOLD_T(3) | EXP_RECOVERY_T(15) | EXP_SZ_512 | EXP_WR_EN | EXP_CS_EN | EXP_BYTE_EN);
  //    *IXP425_EXP_CS7 = (EXP_ADDR_T(3) | EXP_SETUP_T(3) | EXP_STROBE_T(15) | EXP_HOLD_T(3) | EXP_RECOVERY_T(15) | EXP_SZ_512 | EXP_MUX_EN | EXP_WR_EN | EXP_CS_EN);

#endif
  _L(LED7);
}

static void target_release (void)
{
  /* *** FIXME: I don't think this is necessary.  I'm trying to figure
     *** out why the kernel fails to boot.  */

	/* Invalidate caches (I&D) and BTB (?) */
  __asm volatile ("mcr p15, 0, r0, c7, c7, 0" : : : "r0");
  COPROCESSOR_WAIT;

	/* Invalidate TLBs (I&D) */
  __asm volatile ("mcr p15, 0, r0, c8, c7, 0" : : : "r0");
  COPROCESSOR_WAIT;

	/* Drain write buffer */
  __asm volatile ("mcr p15, 0, r0, c7, c10, 4" : : : "r0");
  COPROCESSOR_WAIT;

	/* Disable write buffer coalescing */
  {
    unsigned long v;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 1\n\t"
		    "orr %0, %0, #(1<<0)\n\t"
		    "mcr p15, 0, %0, c1, c0, 1"
		    : "=r" (v));
    COPROCESSOR_WAIT;
  }
}

static __service_0 struct service_d ixp42x_target_service = {
  .init    = target_init,
  .release = target_release,
};
