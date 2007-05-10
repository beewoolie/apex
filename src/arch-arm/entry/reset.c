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
   is entry () which is linked in ../lib/entry.c.  Some of the symbols
   used herein may be overridden by target specific implementations.
   Refer to the documentation for details.

   preinitialization and reset_finish
   ----------------------------------

   In order to support the NAND boot on the lh7 processors where the
   processor may only load 512 bytes from flash, we support a
   preinitialization() call to move more data from flash to memory
   before initialization the SDRAM.  In order to maximize the size of
   the preinitialization function, we put it in it's own segment,
   immediately following .reset, and followed by the reset of the
   bootstrap segment.  The reset_finish() function handles the rest of
   the reset-made calls.

*/

#include <config.h>
#include <asm/bootstrap.h>
#include <debug_ll.h>
#include <arch-arm.h>
#include <memtest.h>
#include <asm/cp15.h>

extern void reset (void);
extern int  initialize_bootstrap (void);
extern void setup_c (void);
extern void init (void);
extern void reset_finish (void);


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
		    "orr %0, %0, #(1<<7)\n\t"	      /* Switch to bigendian */
		    "mcr p15, 0, %0, c1, c0, 0" : "=&r" (v));
    CP15_WAIT;
  }
#endif

#if defined (CONFIG_LITTLEENDIAN)
  {
    unsigned long v;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t"
		    "bic %0, %0, #(1<<7)\n\t"	   /* Switch to littleendian */
		    "mcr p15, 0, %0, c1, c0, 0" : "=&r" (v));
    CP15_WAIT;
    /* *** FIXME: the redboot code performed a read from the ttb
       register as a delay.  Not sure why. */
  }
#endif

  /* We coerce the system into supervisor mode and disable interrupts
     in the first few instructions.  The CPU comes out of reset with
     interrupts enabled, but in an undefined operating mode.  Should
     we be starting in a mode that cannot reload CPSR, this won't do
     anything and we won't be able to execute anything. */
  {
    unsigned long l;
    __asm volatile ("mov %0, #0xd3\n\t"
		    "msr cpsr_c, %0" : "=&r" (l));
  }

#if defined (CONFIG_DISABLE_MMU_AT_BOOT)

  UNLOCK_CACHE;

  /* This disables the MMU, though there should be no reason to do so.
     Still, there are some instances where it is necessary because: a
     preexisting bootloader fails to disable the MMU before calling
     other programs, the OS jumps to the bootloader to restart the
     machine, something else is screwy.  If the MMU mappings are
     wonky, disabling the MMU may have dire consequences.

     This is really much worse than that.  In the case of a system
     that doesn't disable the MMU, we need to perform a batch of CP15
     instructions to make sure the cache is correctly flushed.  Just
     disabling the MMU isn't going to be enough.  This will have to be
     linked in with the mmu code so that everything necessary is done.

     It's also not clear how much of this code needs to be
     architecture specific.

     The ARMv6 implementation of this function has the MMU disable
     after the flushes/invalidates.  I think that the ARMv4 (or
     Xscale) versions may put the disable first.

  */

  CLEANALL_DCACHE;
  INVALIDATE_ICACHE;
  INVALIDATE_DCACHE;
  INVALIDATE_TLB;
  DRAIN_WRITE_BUFFER;
  CP15_WAIT;
  __asm volatile ("mcr p15, 0, %0, c2, c0" : : "r" (0)); /* Clear ttbl */

  {
    unsigned long l;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t"
		    "bic %0, %0, #(1<<0)\n\t"		/* MMU enable */
		    "bic %0, %0, #(1<<1)\n\t"		/* Alignment */
		    "bic %0, %0, #(1<<2)\n\t"		/* DCache */
		    "bic %0, %0, #(1<<12)\n\t"		/* ICache */
		    "mcr p15, 0, %0, c1, c0, 0\n\t"
		    "nop\n\t"
		    "nop\n\t"
		    "nop\n\t"
		    : "=&r" (l)
		    );
    CP15_WAIT;
  }

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
  CP15_WAIT;
#endif

#if defined (CONFIG_PREINITIALIZATION)
  /* There is a little bit of trickery here so that the
     preinitialization code is guaranteed to be as close to the top of
     the program as possible.  The preinitialization funtion must be
     in the .reset segment as is the reset() function.  The reset of
     the reset function, aptly named reset_finish, is placed in the
     .bootstrap segment.  The linker will put the preinitialization()
     code between these two pieces of reset().  In the event that we
     are not using preiniitalization, we can simply elide the
     reset_finish() declaration and reunite reset() with
     reset_finish().  */

  preinitialization ();		/* Special hook for init's before SDRAM */

  __asm volatile ("b reset_finish");
}

void __naked __section (.bootstrap) reset_finish (void)
{
#endif

  /* The initialize_bootstrap () function *must* return TRUE when it
     initialized SDRAM; otherwise, we may clobber ourself in the
     memory test. */

  if (initialize_bootstrap ())	/* Initialization critical to relocate */
#if !defined (CONFIG_BOOTSTRAP_MEMTEST)
  ;
#else
  {
    unsigned long result = 0;

    PUTC ('M');

    result = memory_test_0 (CONFIG_BOOTSTRAP_MEMTEST_BASE,
			    CONFIG_BOOTSTRAP_MEMTEST_SIZE);
    if (result) {
      PUTC ('!');
      PUTHEX (result);
      __asm volatile ("0: b 0b");
    }
    PUTC ('m');
  }
#endif

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
     console driver.

     *** FIXME: the best solution to this problem is to change to the
     *** linker script method for organizing the bootstrap code.  With
     *** it, we can use procedure blocks to isolate register
     *** allocation aliases.

 */
  __asm__ __volatile__ ("" :::
			"r3", "r4", "r5", "r6",
			"r7", "r8", "r9", "r10");
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
