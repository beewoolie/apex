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
#include <debug_ll.h>


/* relocate_apex

   performs a memory move of the whole loader, presumed to be from NOR
   flash into SDRAM.  The LMA is determined at runtime.  The relocator
   will put the loader at the VMA and then return to the relocated address.

   *** FIXME: it might be prudent to check for the VMA being greater
   *** than or less than the LMA.  As it stands, we depend on
   *** twos-complement arithmetic to make sure that offset works out
   *** when we jump to the relocated code.

*/

void __naked __section (.bootstrap) relocate_apex (void)
{
  extern unsigned long reloc;
  unsigned long offset;
  unsigned long lr;

  PUTC_LL ('R');
  __asm volatile ("mov r0, lr\n\t"
		  "bl reloc\n\t"
	   "reloc: subs %0, %2, lr\n\t"
	   ".globl reloc\n\t"
		  "moveq pc, r0\n\t"	/* Return when already reloc'd  */
		  "mov %2, r0\n\t"
		  "mov %1, lr\n\t"
		  : "=r" (offset), "=r" (lr)
		  : "r" (&reloc)
		  : "r0", "lr");

  PUTC_LL ('c');

  __asm volatile (
		  "sub r0, %0, %2\n\t"
	       "0: ldmia r0!, {r3-r10}\n\t"
		  "stmia %0!, {r3-r10}\n\t"
		  "cmp %0, %1\n\t"
		  "bls 0b\n\t"
		  :
		  : "r" (&APEX_VMA_COPY_START), "r" (&APEX_VMA_COPY_END),
		    "r" (offset)
		  : "r0", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10"
		  );		  

  PUTHEX_LL (offset);
  PUTC_LL ('+');
  PUTHEX_LL (lr);

  PUTC_LL ('j');
				/* Return to SDRAM */
  __asm volatile ("add pc, %0, %1" : : "r" (offset), "r" (lr));
}


