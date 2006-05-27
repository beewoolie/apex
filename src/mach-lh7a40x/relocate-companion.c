/* relocate-companion.c

   written by Marc Singer
   26 May 2006

   Copyright (C) 2006 Marc Singer

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

   Implementation of APEX relocator that copies the loader from I2C
   EEPROM or SD/MMC depending on our needs.  This function will
   override the default version that is part of the architecture
   library.

   Bootstrap Loader Selection
   --------------------------

   There are three ways in which this APEX binary may be loaded.  We
   attempt to make sense of each of them.

   Production always depends on the I2C EEPROM to get the system on
   it's feet.  The CPU ROM copies the first 4K from EEPROM to
   0xb0000000 and jumps to the first instruction.  At that point, we
   may want to either read the whole loader from EEPROM and continue
   execution at the same instruction in SDRAM, or we may want to read
   the loader from an SD/MMC card.  The third way we may want to run
   APEX is to copy it into SRAM (via JTAG) and run that version
   without accessing the EEPROM or SD/MMC.

   Distinguishing between the first two cases may be possible using a
   unique FAT partition table layout that signals to the loader that a
   better version of APEX resides there.

   The third case is a little trickier since we don't have a good way
   to know if we only have 4K of the loader, copied from EEPROM, or a
   whole copy of the loader in SRAM.  Unless we implement some sort of
   checksum operation, we'll probably not be able to do this.  Until
   that time, we'll probably depend on a second entry point to signal
   that the loader is already all present and a simple copy from where
   it is desired.

*/

#include <config.h>
#include <apex.h>
#include <asm/bootstrap.h>
#include "nand.h"
#include "hardware.h"

#define USE_MMC
//#define USE_I2C

//#define EMERGENCY


/* wait_on_busy

   wait for the flash device to become ready.

*/

static __naked __section (.bootstrap) void wait_on_busy (void)
{
#if defined CONFIG_NAND_LPD
  while ((CPLD_FLASH & CPLD_FLASH_RDYnBSY) == 0)
    ;
#else
  do {
    NAND_CLE = Status;
  } while ((NAND_DATA & Ready) == 0);
#endif
  __asm volatile ("mov pc, lr");
}

/* relocate_apex

   performs a copy of the loader
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

#if defined (EMERGENCY)
  IOCON_MUXCTL14 |=  (1<<8);
  GPIO_MN_PHYS &= ~(1<<0);
  IOCON_MUXCTL7  &= ~(0xf<<12);

  /* Boot ROM uses 0xf. If I set this to 0xf, I can read nothing from
     NAND. */

  IOCON_MUXCTL7  |=  (0xa<<12);
  IOCON_RESCTL7  &= ~(0xf<<12);
  IOCON_RESCTL7  |=  (0xa<<12);
#endif

  {
    extern char reloc;
    unsigned long offset = (unsigned long) &reloc;
    __asm volatile ("mov %0, lr\n\t"
		    "bl reloc\n\t"
	     "reloc: subs %1, %1, lr\n\t"
	     ".globl reloc\n\t"
		    "moveq pc, %0\n\t"	   /* Simple return if we're reloc'd */
		    "add %0, %0, %1\n\t"   /* Adjust lr for function return */
		    : "=r" (lr),
		      "+r" (offset)
		    :: "lr", "cc");

  }

#if defined (USE_MMC)

  /* Read loader from SD/MMC starting at 0x4000, the start of the
     first partition.  */

	/* Setup stack, quite trivial. This is only needed for this
	   routine and it's subroutine supports.  The C setup will be
	   repreated once the loader is relocated.  Moreover, once we
	   start copying the loader to SDRAM, the stack will be
	   obliterated since we don't attempt to be careful.  In fact,
	   we cannot be careful because the version in SD/MMC may be
	   different, so the address won't match. */
  __asm volatile ("mov sp, %0" :: "r" (&APEX_VMA_STACK_START));

  if (relocate_mmc ())
    ...
#endif



  {
    int cPages = (&APEX_VMA_COPY_END - &APEX_VMA_COPY_START + 511)/512;
    void* pv = &APEX_VMA_ENTRY;
    int cAddr = NAM_DECODE (BOOT_PBC);

    NAND_CLE = Reset;
    wait_on_busy ();

    NAND_CLE = Read1;
    while (cAddr--)
      NAND_ALE = 0;
    wait_on_busy ();

    while (cPages--) {
      int cb;

      NAND_CLE = Read1;
      for (cb = 512; cb--; )
	*((char*) pv++) = NAND_DATA;
      for (cb = 16; cb--; )
	NAND_DATA;
      wait_on_busy ();
    }
  }

  __asm volatile ("mov pc, %0" : : "r" (lr));
}
