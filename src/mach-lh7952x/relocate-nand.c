/* relocate-nand.c
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

*/

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

#if !defined (CONFIG_NAND_LPD)
  __REG (IOCON_PHYS | IOCON_MUXCTL7)  &= ~(0xf000);
//  __REG (IOCON_PHYS | IOCON_MUXCTL7)  |=   0xa000; /* nFRE/nFWE select */
  __REG (IOCON_PHYS | IOCON_MUXCTL7)  |=   0x5000; /* nFRE/nFWE select */

  __REG (IOCON_PHYS | IOCON_RESCTL7)  &= ~(0xf000);
//  __REG (IOCON_PHYS | IOCON_RESCTL7)  |=   0xa000; /* Pull-up */
  __REG (IOCON_PHYS | IOCON_RESCTL7)  |=   0x5000;

//  __REG (IOCON_PHYS | IOCON_MUXCTL14) &= ~(3<<8); /* nCS0 override */
//  __REG (IOCON_PHYS | IOCON_MUXCTL14) |=  (1<<8);
  __REG (IOCON_PHYS | IOCON_MUXCTL14) &= ~(3<<8); /* nCS0 normalize */

//  __REG (GPIO_MN_PHYS | 0x08) &= ~(1<<0); /* PM0 output */
//  __REG (GPIO_MN_PHYS | 0x00) &= ~(1<<0); /* PM0 low */
#endif

  {
    int cPages = (&APEX_VMA_COPY_END - &APEX_VMA_COPY_START + 511)/512;
    void* pv = &APEX_VMA_ENTRY;

    __REG8 (NAND_CLE) = Reset;
    wait_on_busy ();

    __REG8 (NAND_CLE) = Read1;
    __REG8 (NAND_ALE) = 0;
    __REG8 (NAND_ALE) = 0;
    __REG8 (NAND_ALE) = 0;
    wait_on_busy ();

    while (cPages--) {
      int cb;

      __REG8 (NAND_CLE) = Read1;
      for (cb = 512; cb--; )
	*((char*) pv++) = __REG8 (NAND_DATA); /* *** Optimize with assembler */
      {
	unsigned char ch;
	for (cb = 16; cb--; )
	  ch = __REG8 (NAND_DATA);
      }
      wait_on_busy ();
    }
  }

  //  __asm volatile ("0: b 0b");

  __asm volatile ("mov pc, %0" :: "r" (lr));
}
