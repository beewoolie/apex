/* relocate-nand.c

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

   Implementation of APEX relocator that copies the loader from NAND
   flash into SDRAM.  This function will override the default version
   that is part of the architecture library.

*/

#include <config.h>
#include <apex.h>
#include <asm/bootstrap.h>
#include "mach/hardware.h"
#include "mach/drv-nand.h"

#include <debug_ll.h>

//#define EMERGENCY
#define USE_NAND


/* wait_on_busy

   wait for the flash device to become ready.

   *** I think that the reason for a function has to do with register
   *** usage.  I'm not really sure, though, and we could probably move
   *** the while() into the reading function.

*/

static __naked __section (.bootstrap) void wait_on_busy (void)
{
  while (NAND_ISBUSY)
    ;
  __asm volatile ("mov pc, lr");
}


/* relocate_apex

   performs a memory move of the whole loader, presumed to be from
   NAND flash into SDRAM.  The LMA is determined at runtime.  The
   relocator will put the loader at the VMA and then return to the
   relocated address.

   *** FIXME: we can read eight bytes at a time to make the transfer
   *** more efficient.  Probably, this isn't a big deal, but it would
   *** be handy.

*/

void __naked __section (.bootstrap) relocate_apex_nand (void)
{
  unsigned long lr;

  {
    extern char reloc_nand;
    unsigned long offset = (unsigned long) &reloc_nand;
    __asm volatile ("mov %0, lr\n\t"
		    "bl reloc_nand\n\t"
	     "reloc_nand: subs %1, %1, lr\n\t"
	     ".globl reloc_nand\n\t"
		    "moveq pc, %0\n\t"	   /* Simple return if we're reloc'd */
		    "add %0, %0, %1\n\t"   /* Adjust lr for function return */
		    : "=r" (lr),
		      "+r" (offset)
		    :: "lr", "cc");

  }

  {
    int cPages = (&APEX_VMA_COPY_END - &APEX_VMA_COPY_START + 511)/512;
    void* pv = &APEX_VMA_ENTRY;
    int cAddr = NAM_DECODE (BOOT_PBC);
    PUTC_LL('0' + cAddr);

    NAND_CLE = NAND_Reset;
    wait_on_busy ();

    NAND_CLE = NAND_Read1;
    while (cAddr--)
      NAND_ALE = 0;
    wait_on_busy ();

    while (cPages--) {
      int cb;

      NAND_CLE = NAND_Read1;
      for (cb = 512; cb--; )
	*((char*) pv++) = NAND_DATA;
      for (cb = 16; cb--; )
	NAND_DATA;
      wait_on_busy ();
    }
  }

  __asm volatile ("mov pc, %0" : : "r" (lr));
}


/* relocate_apex

   performs a copy of the loader, but it first checks to see if we are
   running from RAM already.  The system hardware (LH7) copies 4K of
   the loader from NAND flash to SRAM at 0xb0000000.  If we are
   executing from there, we assume that we need to perform the NAND
   flash relocation.  Otherwise, we perform a straightforward copy
   from our present location to SDRAM because we've been loaded via
   JTAG.

*/

void __naked __section (.bootstrap) relocate_apex (void)
{
  unsigned long lr;
  unsigned long pc;		/* So we can detect the second stage */
  extern unsigned long reloc;
  unsigned long offset = (unsigned long) &reloc;

  PUTC ('>');
  PUTC ('>');

	/* Setup bootstrap stack, trivially.  We do this so that we
	   can perform some complex operations here in the bootstrap,
	   The C setup will move the stack into SDRAM just after this
	   routine returns. */

  __asm volatile ("mov sp, %0" :: "r" (&APEX_VMA_BOOTSTRAP_STACK_START));

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

  PUTHEX (pc);
  PUTC ('>');

  PUTC ('c');
  //  PUTC_LL ('c');

  if (0) {
    /* Dummy test so that the final clause can be an else */
  }

#if defined (USE_NAND)

	/* Jump to NAND loader only if we could be starting from NAND. */
  else if ((pc >> 12) == (0xb0000000>>12)) {
    PUTC ('N');
    __asm volatile ("mov pc, %0" :: "r" (&relocate_apex_nand));
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
    PUTC ('R');
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
    PUTC ('R');
  __asm volatile (
	       "0: ldmia %0!, {r3}\n\t"
		  "stmia %1!, {r3}\n\t"		  "cmp %1, %2\n\t"
		  "bls 0b\n\t"
		  : "+r" (s),
		    "+r" (d)
		  :  "r" (&APEX_VMA_COPY_END)
		  : "r3", "cc"
		  );
#else
    PUTC ('R');
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
  PUTC ('@');			/* Let 'em know we're jumping */
  __asm volatile ("mov pc, %0" : : "r" (lr));

}
