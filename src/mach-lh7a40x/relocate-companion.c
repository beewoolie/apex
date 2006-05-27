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

   Relocating APEX from SD/MMC
   ---------------------------

   Relocation of APEX from SD/MMC has several elements.  The first 4K
   of the loader are copied from I2C memory to address 0xb0000000 by
   the CPU Boot ROM.  The relocate_apex () implementation here is
   guaranteed to be able to execute from that section of the program.

   The relocator reads the partition table of the SD card.  If it
   finds that the first partition is type 0, it reads 80K - 7K (4K
   for the code from I2C, 2K for stack, and 1K for data), from that
   partition to address 0xb0000000+4k and jumps to it.  This
   guarantees that the loader, as read from SD/MMC, is able fully
   initialize the system including SDRAM as needed.

   If a larger boot loader is needed, the temporary stack may be
   relocated to SDRAM, allowing for another 4K of boot loader code.

*/

#include <config.h>
#include <apex.h>
#include <asm/bootstrap.h>
#include <mmc.h>
#include "hardware.h"

#include <debug_ll.h>

#define USE_MMC
//#define USE_I2C

//#define EMERGENCY

#if defined (CONFIG_DEBUG_LL)
//# define USE_LDR_COPY		/* Simpler copy loop, more free registers */
# define USE_SLOW_COPY		/* Simpler copy loop, more free registers */
# define USE_COPY_VERIFY
#endif

#define MMC_BOOTLOADER_SIZE		((80 - 4 - 4 - 1)*1024)
//#define MMC_BOOTLOADER_SIZE		(64*1024)
//#define MMC_BOOTLOADER_LOAD_ADDR	(0xb0000000 + 4*1024)
#define MMC_BOOTLOADER_LOAD_ADDR	(0xc0000000)

int relocate_apex_mmc (void)
{
  struct descriptor_d d;
  size_t cb;
  unsigned char rgb[16];	/* Partition table entry */

  PUTC_LL ('M');

  mmc_init ();
  PUTC_LL ('1');
  mmc_acquire ();
  PUTC_LL ('2');

  if (!mmc_card_acquired ())
    return 0;

  PUTC_LL ('a');

  d.start = 512 - 2 - 4*16;
  d.length = 16;
  d.index = 0;
  cb = mmc_read (&d, (void*) rgb, d.length);
  if (cb != d.length)
    return 0;

  if (rgb[4] != 0)		/* Must be type 0 */
    return 0;

  d.start = *((unsigned long*) &rgb[8])*512;	/* Start of first partition */
  d.length = MMC_BOOTLOADER_SIZE;
  d.index = 0;

  PUTHEX_LL (d.start);
  PUTC_LL ('C');
  cb = mmc_read (&d, (void*) MMC_BOOTLOADER_LOAD_ADDR, d.length);

  PUTC_LL ('c');
  PUTHEX_LL (cb);
  PUTC_LL ('\r');
  PUTC_LL ('\n');

  return (cb == d.length) ? 1 : 0;
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
  unsigned long pc;		/* So we can detect the second stage */
  extern unsigned long reloc;
  unsigned long offset = (unsigned long) &reloc;

  PUTC_LL ('R');

	/* Setup bootstrap stack, trivially.  We do this so that we
	   can perform some complex operations here in the bootstrap,
	   e.g. copying from SD/MMC.  The C setup will move the stack
	   into SDRAM just after this routine returns. */

  __asm volatile ("mov sp, %0" :: "r" (&APEX_VMA_BOOTSTRAP_STACK_START));

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

  __asm volatile ("mov %0, lr\n\t"
		  "bl reloc\n\t"
	   "reloc: mov %2, lr\n\t"
		  "subs %1, %1, lr\n\t"
	   ".globl reloc\n\t"
		  "moveq pc, %0\n\t"	   /* Simple return if we're reloc'd */
		  "add %0, %0, %1\n\t"   /* Adjust lr for function return */
		  : "=r" (lr),
		    "+r" (offset),
		    "=r" (pc)
		  :: "lr", "cc");

  PUTC_LL ('c');

  if (0) {
    /* Dummy test so that the final clause can be an else */
  }

#if defined (USE_MMC)

	/* Read loader from SD/MMC only if we could be starting from I2C. */
  else if ((pc >> 12) == (0xb0000000>>12) && relocate_apex_mmc ()) {
    PUTC_LL ('m');
    lr = MMC_BOOTLOADER_LOAD_ADDR;
  }

#endif

  /* *** FIXME: it might be good to allow this code to exist in a
     subroutine so that we can, optionally, use it here.  The linkage
     tricks preclude this at the moment, but hope is not lost. */

      /* Relocate from current copy in memory, probably SRAM. */
  else {
    unsigned long d = (unsigned long) &APEX_VMA_COPY_START;
    unsigned long s = (unsigned long) &APEX_VMA_COPY_START - offset;
#if defined USE_LDR_COPY
    unsigned long index
      = (&APEX_VMA_COPY_END - &APEX_VMA_COPY_START + 3 - 4) & ~3;
    unsigned long v;
    PUTC_LL ('R');
    __asm volatile (
		    "0: ldr %3, [%0, %2]\n\t"
		       "str %3, [%1, %2]\n\t"
		       "subs %2, %2, #4\n\t"
		       "bpl 0b\n\t"
		       : "+r" (s),
			 "+r" (d),
			 "+r" (index),
			 "=&r" (v)
		       :: "cc"
		    );
#elif defined (USE_SLOW_COPY)
    PUTC_LL ('R');
  __asm volatile (
	       "0: ldmia %0!, {r3}\n\t"
		  "stmia %1!, {r3}\n\t"
		  "cmp %1, %2\n\t"
		  "bls 0b\n\t"
		  : "+r" (s),
		    "+r" (d)
		  :  "r" (&APEX_VMA_COPY_END)
		  : "r3", "cc"
		  );
#else
    PUTC_LL ('R');
  __asm volatile (
	       "0: ldmia %0!, {r3-r10}\n\t"
		  "stmia %1!, {r3-r10}\n\t"
		  "cmp %1, %2\n\t"
		  "bls 0b\n\t"
		  : "+r" (s),
		    "+r" (d)
		  :  "r" (&APEX_VMA_COPY_END)
		  : "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "cc"
		  );
#endif
  }

				/* Return to SDRAM */
  __asm volatile ("mov pc, %0" : : "r" (lr));

}
