/* relocate.c
     $Id$

   written by Marc Singer
   9 Nov 2004

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

*/

#include <config.h>
#include <asm/bootstrap.h>

#if defined (CONFIG_MACH_IXP42X)
# include <mach/hardware.h>
#endif

#include <debug_ll.h>

//# define USE_LDR_COPY		/* Simpler copy loop, more free registers */

#if defined (CONFIG_DEBUG_LL)
# define USE_LDR_COPY		/* Simpler copy loop, more free registers */
# define USE_COPY_VERIFY
#endif


/* relocate_apex

   performs a memory move of the whole loader, presumed to be from NOR
   flash into SDRAM.  The LMA is determined at runtime.  The relocator
   will put the loader at the VMA and then return to the newly
   relocated return address.

   *** FIXME: it might be prudent to check for the VMA being greater
   *** than or less than the LMA.  As it stands, we depend on
   *** twos-complement arithmetic to make sure that offset works out
   *** when we jump to the relocated code.

   This function does not check for source overlapping destination.

*/

void __naked __section (.bootstrap) relocate_apex (void)
{
  unsigned long lr;
  extern unsigned long reloc;
  unsigned long offset = (unsigned long) &reloc;

  PUTC_LL ('R');
  __asm volatile ("mov %0, lr\n\t"
		  "bl reloc\n\t"
	   "reloc: subs %1, %1, lr\n\t"
	   ".globl reloc\n\t"
		  "moveq pc, %0\n\t"	/* Return when already reloc'd  */
		  "add %0, %0, %1"	/* Adjust lr for function return */
		  :  "=&r" (lr),
		     "+r" (offset)
		  :: "lr", "cc");

  PUTC_LL ('c');

  {
    unsigned long d = (unsigned long) &APEX_VMA_COPY_START;
    unsigned long s = (unsigned long) &APEX_VMA_COPY_START - offset;
#if defined USE_LDR_COPY
    unsigned long index
      = (&APEX_VMA_COPY_END - &APEX_VMA_COPY_START + 3 - 4) & ~3;
    unsigned long v;
    __asm volatile (
		    "0: ldr %3, [%0, %2]\n\t"
		       "str %3, [%1, %2]\n\t"
		       "subs %2, %2, #4\n\t"
		       "bne 0b\n\t"
		       : "+r" (s),
		         "+r" (d),	
			 "+r" (index),
			 "=&r" (v)
		       :: "cc"
		    );		  
#else
  __asm volatile (
	       "0: ldmia %0!, {r3-r10}\n\t"
		  "stmia %1!, {r3-r10}\n\t"
		  "cmp %0, %2\n\t"
		  "bls 0b\n\t"
		  : "+r" (s), 
		    "+r" (d)
		  :  "r" (&APEX_VMA_COPY_END)
		  : "r0", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "cc"
		  );		  
#endif
  }

  PUTC_LL ('\r');
  PUTC_LL ('\n');
  PUTHEX_LL (offset);
  PUTC_LL ('+');
  PUTHEX_LL (lr);
  PUTC_LL ('=');
  PUTC_LL ('>');
  PUTHEX_LL (*(unsigned long*) (lr - offset));	/* Original */
  PUTC_LL ('=');
  PUTC_LL ('=');
  PUTHEX_LL (*(unsigned long*) (lr));		/* Copy */
  PUTC_LL ('\r');
  PUTC_LL ('\n');

#if defined (USE_COPY_VERIFY)
  if (*(unsigned long*) lr != *(unsigned long*) (lr - offset)) {
    PUTC_LL ('@');
    PUTC_LL ('F');
    PUTC_LL ('A');
    PUTC_LL ('I');
    PUTC_LL ('L');
    __asm volatile ("0: b 0b");
  }  
#endif

  PUTC_LL ('j');

#if defined (CONFIG_MACH_IXP42X)
	/* Drain write buffer */
  {
    unsigned long v;
    __asm volatile ("mcr p15, 0, r0, c7, c10, 4" : "=r" (v) );
  }
//  COPROCESSOR_WAIT;

	/* Invalidate caches (I&D) and branch buffer (BTB) */
 {
   unsigned long v;
   __asm volatile ("mcr p15, 0, %0, c7, c7, 0" : "=r" (v));
   COPROCESSOR_WAIT;
 }
#endif

				/* Return to SDRAM */
  //  __asm volatile ("add pc, %0, %1" : : "r" (offset), "r" (lr));
  __asm volatile ("mov pc, %0" : : "r" (lr));
}
