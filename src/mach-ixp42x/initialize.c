/* initialize.c
     $Id$

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

*/

#include <config.h>
#include <asm/bootstrap.h>
#include <service.h>

#include "hardware.h"


#define COPROCESSOR_WAIT\
 ({ unsigned long v; \
    __asm volatile ("mrc p15, 0, %0, c2, c0, 0\n\t" \
		    "mov %0, %0\n\t" \
		    "sub pc, pc, #4" : "=r" (v)); })

/* *** FIXME: sdram config constants should go here.  The Integrated
   Circuit Solution Inc IC42S16800 DRAM can do CAS2.  Later. */


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

  __asm volatile ("mov %0, lr" : "=r" (lr));

  GPIO_ER &= ~0xf;
  _L(LEDf);			/* Start with all on */

  /* *** FIXME: this CPSR and CP15 manipulation should not be
     *** necessary as we don't allow warm resets. */

     	/* Fixup the CP15 control word.  This is done for the cases
	   where we are bootstrapping from redboot which does not
	   cleanup before jumping to code.  */
  __asm volatile ("mrc p15, 0, r0, c1, c0, 0\n\t"
		  "bic r0, r0, #(1<<0)\n\t" /* Disable MMU */
		  "bic r0, r0, #(1<<1)\n\t" /* Disable alignment check */
		  "bic r0, r0, #(1<<4)\n\t" /* Disable write buffer */
		  "mcr p15, 0, r0, c1, c0, 0");

	/* Disable interrupts and set supervisor mode */
  __asm volatile ("msr cpsr, %0" : : "r" ((1<<7)|(1<<6)|(0x13<<0)));
 
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

	/* Configure flash access, slowest timing */
  /* *** FIXME: do we really need this?  We're already running in
     *** flash.  Moreover, I'd rather make it fast instead of slow. */
  EXP_TIMING_CS0 = 0
    | (( 3 & EXP_T1_MASK)	<<EXP_T1_SHIFT)
    | (( 3 & EXP_T2_MASK)	<<EXP_T2_SHIFT)
    | ((15 & EXP_T3_MASK)	<<EXP_T3_SHIFT)
    | (( 3 & EXP_T4_MASK)	<<EXP_T4_SHIFT)
    | ((15 & EXP_T5_MASK)	<<EXP_T4_SHIFT)
    | ((15 & EXP_CNFG_MASK)	<<EXP_CNFG_SHIFT)	/* 16MiB window */
    | (( 0 & EXP_CYC_TYPE_MASK)	<<EXP_CYC_TYPE_SHIFT)	/* Intel cycling */
    | EXP_BYTE_RD16
    | EXP_WR_EN
    | EXP_CS_EN
    ;

	/* Enable instruction cache */
  { 
    unsigned long v;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t"
		    "orr %0, %0, #(1<<12)\n\t"
		    "mcr p15, 0, %0, c1, c0, 0" : "=r" (v));
    COPROCESSOR_WAIT;
  }

	  /* Exit now if executing in SDRAM */ 
  if (EXP_CNFG0 & (1<<31)) {
    _L(LED1);
    /* Boot mode */
    __asm volatile ("cmp %0, %2\n\t"
		    "bls 1f\n\t"
		    "cmp %0, %1\n\t"
		    "movls r0, #0\n\t"
		    "movls pc, %0\n\t"
		    "1:": : "r" (lr), "i" (0x40000000), "i" (256*1024*1024));
  }
  else {
    _L(LED2);
    /* Non-boot mode */
    __asm volatile ("cmp %0, %1\n\t"
		    "movls r0, #0\n\t"
		    "movls pc, %0\n\t"
		    : : "r" (lr), "i" (0x40000000));
  }

  _L(LED3);

  usleep (1000);		/* Wait for CPU to stabilize SDRAM signals */

  SDR_CONFIG = SDR_CONFIG_CAS3 | SDR_CONFIG_2x16Mx16;
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

  __asm volatile ("mov r0, #-1\t\n"
		  "mov pc, %0" : : "r" (lr));
}


/* target_init

   performs the rest of the hardware initialization that didn't have
   to be performed during the bootstrap phase.

*/

static void target_init (void)
{
  EXP_CNFG0 &= ~EXP_CNFG0_MEM_MAP; /* Disable boot-mode for EXP_CS0  */
  __REG(EXP_PHYS + 0x28) |= (1<<15);	/* Undocumented, but set in redboot */
}

#if 0
static void target_release (void)
{
}
#endif

static __service_0 struct service_d ixp42x_target_service = {
  .init    = target_init,
  //  .release = target_release,
};
