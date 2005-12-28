/* entry.c

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

   Default entry for APEX loader on ARM.  Most of the symbols herein
   are weak and may be overridden by target specific implementations.
   Refer to the documentation for details.

*/

#include <config.h>
#include <asm/bootstrap.h>
#include <debug_ll.h>
#include <mach/coprocessor.h>

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

   implements the reset exception vector.  All symbols but the init ()
   call must not depend on a stack or any RAM whatsoever.

*/

void __naked __section (.reset) reset (void)
{
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

  /* This disables the MMU, but there should be no reason to include
     it.  However, there are some instances where it is necessary
     because: a preexisting bootloader fails to disable the MMU before
     calling other programs, the OS jumps to the bootloader to restart
     the machine, something else is screwy.  If the MMU mappings are
     wonky, disabling the MMU may have dire consequences. */
  {
    unsigned long l;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t"
		    "bic %0, %0, #1\n\t"
		    "mcr p15, 0, %0, c1, c0, 0\n\t" : "=&r" (l)
		    );
    COPROCESSOR_WAIT;
  }
#endif

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
		   : "=&r" (p)
		   :   "r" (&APEX_VMA_STACKS_END), "r" (0xe5e5e5e5));
 }
#endif

	/* Clear BSS */
 {
   void* p = &APEX_VMA_BSS_START;
   __asm volatile (
		   "0: cmp %0, %1\n\t"
		      "stmlsia %0!, {%2}\n\t"
		      "bls 0b\n\t"
		   : "=&r" (p)
		   :   "r" (&APEX_VMA_BSS_END), "r" (0));
 }

  __asm volatile ("mov pc, lr");
}

void __div0 (void)
{
  /* *** FIXME: perhaps we should reset */

  while (1)
    ;
}
