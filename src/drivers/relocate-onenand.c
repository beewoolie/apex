/* relocate-onenand.c

   written by Marc Singer
   7 May 2007

   Copyright (C) 2007 Marc Singer

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

   Implementation of APEX relocator that copies the loader from
   OneNAND flash into SDRAM.  This function will override the default
   version that is part of the architecture library.  Note that we
   *still* have to cope with the possibility that we're running from
   SDRAM and so we need to relocate within memory.  Thus, we play
   nice.

   o DataBuffer0/DataBuffer1.  In APEX, we don't really care about
     which data buffer we use to access the NAND flash.  This
     relocation code uses DataBuffer0 so that we can get more than 1K
     of the boot loader into contiguous memory before SDRAM is
     initialized.

   o Preinitialization.  In order to get a 3k boot loader image, load
     the second and third KiB from NAND flash into DataRam0.  See the
     comment on the preinitialization function for details.

*/

#include <config.h>
#include <apex.h>
#include <asm/bootstrap.h>
#include "mach/hardware.h"
#include "drv-onenand-base.h"
#include <linux/bitops.h>

#include <debug_ll.h>

#define PAGE_SIZE ONENAND_DATA_SIZE

/* preinitialization

   performs a crucial preload of the second and third KiB of APEX into
   DataRAM0.  This allows us to run with 3KiB of boot loader code
   until we get the SDRAM initialized and the whole loader copied to
   SDRAM.

   Note that we always perform this preinitialization because it
   cannot hurt and it is cheaper to just execute the load than deal
   with trying to figure out where APEX is executing.  We already can
   be sure that the OneNAND flash is available.

*/

void __naked __section (.preinit) preinitialization (void)
{
  unsigned long lr;

  __asm volatile ("mov %0, lr\n\t" : "=r" (lr));

  //  STORE_REMAP_PERIPHERAL_PORT (0x40000000 | 0x15); /* 1GiB @ 1GiB */

  //#if defined (CONFIG_STARTUP_UART)
  //  INITIALIZE_CONSOLE_UART;
  //  PUTC('P');
  //#endif

  ONENAND_ADDRSETUP (0, 2);
  ONENAND_BUFFSETUP (0, 0, 2);
  ONENAND_INTR = 0;
  ONENAND_CMD = ONENAND_CMD_LOAD;

  while (ONENAND_IS_BUSY)
    ;

  ONENAND_ADDRSETUP (1, 0);
  ONENAND_BUFFSETUP (0, 2, 2);
  ONENAND_INTR = 0;
  ONENAND_CMD = ONENAND_CMD_LOAD;

  while (ONENAND_IS_BUSY)
    ;

  __asm volatile ("mov pc, %0" : : "r" (lr));
}


/* relocate_apex_onenand

   performs the relocation from OneNAND into SDRAM.The LMA is
   determined at runtime.  The relocator will put the loader at the
   VMA and then return to the relocated address.

   The passed parameter is the true return address for the
   relocate_apex() function so that we continue execution in SDRAM
   once relocatoin is complete.

*/

void __naked __section (.bootstrap) relocate_apex_onenand (unsigned long lr)
{
  int page_size = PAGE_SIZE;
  int cPages = (&APEX_VMA_COPY_END - &APEX_VMA_COPY_START
		+ page_size - 1);
  int page = 0;
  void* pv = &APEX_VMA_ENTRY;

  {
    int v;
	/* Divide by the page size without resorting to a function call */
    for (v = page_size >> 1; v; v = v>>1)
      cPages >>= 1;
  }

  for (; page < cPages; ++page) {
      /* Use this to see how many blocks we're copying from flash */
//    PUTC_LL('A' + (page&0xf));

    ONENAND_PAGESETUP (page);
    ONENAND_BUFFSETUP (1, 0, 4);
    ONENAND_INTR = 0;
    ONENAND_CMD = ONENAND_CMD_LOAD;

    while (ONENAND_IS_BUSY)
	;

    __asm volatile (
		 "0: ldmia %1!, {r3-r6}\n\t"
		    "stmia %0!, {r3-r6}\n\t"
		    "cmp %0, %2\n\t"
		    "blo 0b\n\t"
		 : "+r" (pv)
		 :  "r" (ONENAND_DATARAM1),
		 "r" (pv + page_size)
		 : "r3", "r4", "r5", "r6", "cc"
		 );

    /* Note that we don't need to increment pv as it is incremented by
       the stmia instruction.  */
  }

  PUTC_LL('!');

  __asm volatile ("mov pc, %0" : : "r" (lr));
}


/* relocate_apex

   relocates the loader from either OneNAND flash or from SDRAM to the
   linked VMA.  The determination of our boot method is based on the
   execution address.  For OneNAND flash boot, we assume that the
   system is executing from a known location in memory based on the
   architecture.  Really, we may need to allow the mach- to inject a
   piece of code to perform the check because it may require some
   other technique to determine the boot mode.

*/

void __naked __section (.bootstrap) relocate_apex (void)
{
  unsigned long lr;
  unsigned long pc;		/* So we can detect the second stage */
  extern unsigned long reloc;
  unsigned long offset = (unsigned long) &reloc;

  PUTC ('>');

	/* Setup bootstrap stack, trivially.  We do this so that we
	   can perform some complex operations here in the bootstrap,
	   The C setup will move the stack into SDRAM just after this
	   routine returns. */

//  __asm volatile ("mov sp, %0" :: "r" (&APEX_VMA_BOOTSTRAP_STACK_START));

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

  PUTHEX_LL (pc);
  PUTC_LL ('>');

  PUTC ('c');
  //  PUTC_LL ('c');

	/* Jump to OneNAND loader only if we could be starting from NAND. */
  if ((pc >> 12) == (CONFIG_DRIVER_ONENAND_BASE>>12)) {
    PUTC ('N');
    __asm volatile ("mov r0, %0" :: "r" (lr)); /* 'Push' lr as first arg */
    __asm volatile ("b relocate_apex_onenand\n");
  }

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
