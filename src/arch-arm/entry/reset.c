/* reset.c

   written by Marc Singer
   28 October 2004

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

   True, default entry for APEX loader on ARM.  The real entry point
   is entry () which is linked in ../lib/entry.c.  Most of the symbols
   herein are weak and may be overridden by target specific
   implementations.  Refer to the documentation for details.

*/

#include <config.h>
#include <asm/bootstrap.h>
#include <debug_ll.h>
#include <mach/coprocessor.h>
#include "mmu.h"

#if !defined (COPROCESSOR_WAIT)
# define COPROCESSOR_WAIT
#endif

extern void reset (void);
extern int  initialize_bootstrap (void);
extern void initialize_target (void);
extern void initialize (void);
extern void relocate_apex (void);
extern void setup_c (void);
extern void init (void);


/* reset

   implements the reset exception vector.  All before init() MUST NOT
   DEPEND on a stack or any RAM whatsoever.

*/

void __naked __section (.reset) reset (void)
{
#if defined (CONFIG_BIGENDIAN)

  {
    unsigned long v;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t"
		    "orr %0, %0, #(1<<7)\n\t" /* Switch to bigendian */
		    "mcr p15, 0, %0, c1, c0, 0" : "=&r" (v));
    COPROCESSOR_WAIT;
    /* *** FIXME: the redboot code performed a read from the ttb
       register as a delay.  Not sure why. */
  }
#endif

#if defined (CONFIG_LITTLEENDIAN)

  {
    unsigned long v;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t"
		    "bic %0, %0, #(1<<7)\n\t" /* Switch to littleendian */
		    "mcr p15, 0, %0, c1, c0, 0" : "=&r" (v));
    COPROCESSOR_WAIT;
    /* *** FIXME: the redboot code performed a read from the ttb
       register as a delay.  Not sure why. */
  }
#endif

#if 0
  /* Like most code that seems like a good idea, this isn't used
     because it cannot make a difference.  The only way it is valid for
     the loader to start is when the processor is already in
     supervisor mode with interrupts disabled. */

  /* First things first.  We coerce the system into supervisor mode
     and disable interrupts in the first few instructions.  Really,
     this cannot be done if we're not already in supervisor mode.  The
     important thing is that we want to disable interrupts.
  */
  {
    unsigned long l;
    __asm volatile ("mov %0, #0xd3\n\t"
		    "msr cpsr_c, %0" : "=&r" (l));
  }
#endif

#if defined (CONFIG_DISABLE_MMU_AT_BOOT)

  CACHE_UNLOCK;

  /* This disables the MMU, but there should be no reason to include
     it.  However, there are some instances where it is necessary
     because: a preexisting bootloader fails to disable the MMU before
     calling other programs, the OS jumps to the bootloader to restart
     the machine, something else is screwy.  If the MMU mappings are
     wonky, disabling the MMU may have dire consequences.

     This is really much worse than that.  In the case of a system
     that doesn't disable the MMU, we need to perform a batch of CP15
     instructions to make sure the cache is correctly flushed.  Just
     disabling the MMU isn't going to be enough.  This will have to be
     linked in with the mmu code so that everything necessary is done.
     I'm leaving this [somewhat broken] code here for the time being.
  */

  {
    unsigned long l;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t"
		    "bic %0, %0, #1\n\t"
		    "mcr p15, 0, %0, c1, c0, 0\n\t" : "=&r" (l)
		    );
    COPROCESSOR_WAIT;
  }

  __asm volatile ("mcr p15, 0, %0, c7, c5, 0" : : "r" (0));  // Inv. I cache
  __asm volatile ("mcr p15, 0, %0, c7, c6, 0" : : "r" (0));  // Inv. D cache
  __asm volatile ("mcr p15, 0, %0, c8, c7, 0" : : "r" (0));  // Inv. TLBs
  COPROCESSOR_WAIT;

  CACHE_FLUSH;

#endif

  /* *** This fragment exists to help locate problems with code
     jumping into the wrong place and other bothersome errors.  In
     general, the loader should be free of alignment issues, so we
     don't need to check for alignment errors.  */
#if defined (CONFIG_ALIGNMENT_TRAP) && 0
  {
    unsigned long l;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t"
		    "orr %0, %0, #(1<<1)\n\t" /* Enable alignment trap */
		    "mcr p15, 0, %0, c1, c0, 0\n\t" : "=&r" (l)
		    );
  }
  COPROCESSOR_WAIT;
#endif

  initialize_bootstrap ();	/* Initialization critical to relocate */
  PUTC_LL ('E');
  PUTC_LL ('r');
  relocate_apex ();
  /* This early executed code can be very temperamental. The
     relocate_apex() function will tend to clobber some of the
     registers.  Even with careful coding, it will clobber r4 and
     could affect r5.  The PUTC_LL macro instructions may be split
     across the relocate_apex () call such that a load before
     relocate_apex() is lost by the time the value is needed.  So, the
     work-around is to force the compiler to place those constants in
     higher numbered registers.  That way, relocate_apex () won't
     stomp the needed register.  Also note that the symptom of this
     error was difficult to trace as it became a write to a random
     memory location.  In one case, this was the address of the
     console driver. */
  __asm__ __volatile__ ("" : : : "r3", "r4", "r5");
  PUTC_LL ('s');
  setup_c ();			/* Setups before executing C code */

  PUTC_LL ('e');

	/* Start loader proper which doesn't return */
  __asm volatile ("mov pc, %0" :: "r" (&init));
}


/* setup_c

   performs setup necessary to make standard C (APCS) happy, a stack,
   a clear BSS, and data variables in RAM.  The later is usually
   handled by the relocate.

*/

void __naked setup_c (void)
{
	/* Setup stack, quite trivial */
  __asm volatile ("mov sp, %0" :: "r" (&APEX_VMA_STACK_START));

#if defined (CONFIG_CLEAR_STACKS)
	/* Clear stacks */
 {
   void* p = &APEX_VMA_STACKS_START;
   __asm volatile (
		   "0: stmia %0!, {%2}\n\t"
		   "   cmp %0, %1\n\t"
		   "   bls 0b\n\t"
		   : "+r" (p)
		   :  "r" (&APEX_VMA_STACKS_END), "r" (0xe5e5e5e5));
 }
#endif

	/* Clear BSS */
 {
   void* p = &APEX_VMA_BSS_START;
   __asm volatile (
		   "0: cmp %0, %1\n\t"
		      "stmlsia %0!, {%2}\n\t"
		      "bls 0b\n\t"
		   : "+r" (p)
		   :  "r" (&APEX_VMA_BSS_END), "r" (0));
 }

  __asm volatile ("mov pc, lr");
}

void __div0 (void)
{
  /* *** FIXME: perhaps we should reset */

  while (1)
    ;
}
