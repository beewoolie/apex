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
  /* This would disable the MMU, but there is little reason to include
     it.  The only way this would be called would be if the CPU jumped
     to the loader while running something that needed the MMU.  In
     the unusual situation where the MMU maps the loader in a place
     where it can execute, this might be necessary.  I'll leave it out
     until there is a legitimate use.  */
  __asm volatile ("mrc p15, 0, r0, c1, c0, 0\n\t"
		  "bic r0, r0, #1\n\t"
		  "mcr p15, 0, r0, c1, c0, 0");
#endif

#if defined (CONFIG_BIGENDIAN)
  {
    unsigned long v;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t"
		    "orr %0, %0, #(1<<7)\n\t" /* Switch to bigendian */
		    "mcr p15, 0, %0, c1, c0, 0" : "=r" (v));
    __asm volatile ("mov r0, r0");
    /* *** FIXME: the redboot code performed a read from the ttb
       register as a delay.  Not sure why. */
  }
#endif

  initialize_bootstrap ();	/* Initialization critical to relocate */
  relocate_apex ();
  setup_c ();			/* Setups before executing C code */

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

	/* Clear BSS */
  if (&APEX_VMA_BSS_START != &APEX_VMA_BSS_END)
    __asm volatile (
	   "0: stmia %0!, {%2}\n\t"
	   "   cmp %0, %1\n\t"
	   "   bls 0b\n\t"
	   : : "r" (&APEX_VMA_BSS_START), "r" (&APEX_VMA_BSS_END), "r" (0));

  __asm volatile ("mov pc, lr");
}

void __div0 (void)
{
  /* *** FIXME: perhaps we should reset */

  while (1)
    ;
}
