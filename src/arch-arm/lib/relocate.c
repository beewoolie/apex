/* relocate.c
     $Id$

   written by Marc Singer
   9 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#include <config.h>
#include <asm/bootstrap.h>


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
  __asm volatile ("mov r0, lr\n\t"
		  "bl reloc\n\t"
	   "reloc: subs ip, %0, lr\n\t"
	   ".globl reloc\n\t"
		  "moveq pc, r0\n\t"
		  "mov lr, r0\n\t"
		  :
		  : "r" (&reloc)
		  : "r0", "ip");

  __asm volatile (
		  "sub r0, r1, ip\n\t"
	       "0: ldmia r0!, {r3-r10}\n\t"
		  "stmia %0!, {r3-r10}\n\t"
		  "cmp %0, %1\n\t"
		  "ble 0b\n\t"
		  "add pc, ip, lr\n\t"
		  :
		  : "r" (&APEX_VMA_COPY_START), "r" (&APEX_VMA_COPY_END)
		  : "r0", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "ip"
		  );		  
}


