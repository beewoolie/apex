/* relocate-nand.c
     $Id$

   written by Marc Singer
   9 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

   Implementation of apex relocator that copies the loader from NAND
   flash into SDRAM.  This function will override the default version
   that is part of the architecture library.

*/

#include <config.h>
#include <asm/bootstrap.h>
#include "nand.h"
#include "lh79524.h"

/* wait_on_busy

   wait for the flash device to become ready.

*/

static __naked __section (.bootstrap) void wait_on_busy (void)
{
  while ((__REG8 (CPLD_REG_FLASH) & RDYnBSY) == 0)
    ;
  __asm ("mov pc, lr");
}


/* relocate_apex

   performs a memory move of the whole loader, presumed to be from NOR
   flash into SDRAM.  The LMA is determined at runtime.  The relocator
   will put the loader at the VMA and then return to the relocated address.

   *** FIXME: we can read eight bytes at a time to make the transfer
   *** more efficient.  Probably, this isn't a big deal, but it would
   *** be handy.

   Without the high-order byte, we can only read 128KiB from NAND
   flash.  It should be adequate as is.

*/

void __naked __section (.bootstrap) relocate_apex (void)
{
  int cb;
  const int cPages = (&APEX_VMA_COPY_END - &APEX_VMA_COPY_START + 511)/512;
  int page;
  void* pv = &APEX_VMA_ENTRY;

  __REG8 (NAND_CLE) = Reset;
  wait_on_busy ();

  for (page = 0; page < cPages; ++page) {
    __REG8 (NAND_CLE) = Read1;
    __REG8 (NAND_ALE) = 0;
    __REG8 (NAND_ALE) = ( page        & 0xff);
//  __REG8 (NAND_ALE) = ((page >>  8) & 0xff);
    __REG8 (NAND_ALE) = 0;
    wait_on_busy ();

    for (cb = 512; cb--; )
      *((char*) pv++) = __REG8 (NAND_DATA); /* *** Optimize with assembler */
  }

  /* *** Starting at the top is OK as long as we don't call
     *** relocate_apex again. */
  __asm ("mov pc, %0" : : "r" (&APEX_VMA_ENTRY));

#if 0
  extern char reloc;
  __asm volatile ("mov r0, lr\n\t"
		  "bl reloc\n\t"
	   "reloc: sub ip, %0, lr\n\t"
	   ".globl reloc\n\t"
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
#endif
}


