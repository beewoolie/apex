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
#if defined CONFIG_NAND_LPD
  while ((__REG8 (CPLD_REG_FLASH) & RDYnBSY) == 0)
    ;
#else
  do {
    __REG8 (NAND_CLE) = Status;
  } while ((__REG8 (NAND_DATA) & Ready) == 0);
#endif
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

#if 0
void __naked __section (.bootstrap) relocate_apex (void)
{
  extern unsigned long reloc;
  __asm volatile ("mov r0, lr\n\t"
		  "bl reloc\n\t"
	   "reloc: subs ip, %0, lr\n\t"
	   ".globl reloc\n\t"
		  "moveq pc, lr\n\t" /* Simple return if we're reloc'd  */
		  "mov lr, r0\n\t"
		  :
		  : "r" (&reloc)
		  : "r0", "ip");
  


}
#else
void __naked __section (.bootstrap) relocate_apex (void)
{
  unsigned long lr;
  extern char reloc;
  __asm volatile ("mov r0, lr\n\t"
		  "bl reloc\n\t"
	   "reloc: subs r1, %1, lr\n\t"
	   ".globl reloc\n\t"
		  "moveq pc, r0\n\t" /* Simple return if we're reloc'd  */
		  "add %0, r0, r1\n\t"
		  : "=r" (lr)
		  : "r" (&reloc)
		  : "r0", "r1");

  {
    int cPages = (&APEX_VMA_COPY_END - &APEX_VMA_COPY_START + 511)/512;
    //    int page;
    void* pv = &APEX_VMA_ENTRY;

    __REG8 (NAND_CLE) = Reset;
    wait_on_busy ();

    __REG8 (NAND_CLE) = Read1;
    __REG8 (NAND_ALE) = 0;
//  __REG8 (NAND_ALE) = ( page        & 0xff);
    __REG8 (NAND_ALE) = 0;
//  __REG8 (NAND_ALE) = ((page >>  8) & 0xff);
    __REG8 (NAND_ALE) = 0;
    wait_on_busy ();

    while (cPages--) {
      int cb;
//    for (page = 0; page < cPages; ++page) {
      for (cb = 512; cb--; )
	*((char*) pv++) = __REG8 (NAND_DATA); /* *** Optimize with assembler */
      {
	unsigned char ch;
	for (cb = 16; cb--; )
	  ch = __REG8 (NAND_DATA);
      }
      wait_on_busy ();
      __REG8 (NAND_CLE) = Read1;
    }
  }

  __asm volatile ("0: b 0b");
  __asm volatile ("mov pc, %0" :: "r" (lr));

  /* *** Starting at the top is OK as long as we don't call
     *** relocate_apex again. */
//  __asm ("mov pc, %0" : : "r" (&APEX_VMA_ENTRY));
}
#endif


