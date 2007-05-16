/* preinitialization-companion.c

   written by Marc Singer
   16 Oct 2006

   Copyright (C) 2006 Marc Singer

   -----------
   DESCRIPTION
   -----------

   The bad news is that the NAND flash IO code with SDRAM init code is
   too large to fit in 512 bytes , surprise, surprise.  A feature
   (bug) in the LH7A404 boot ROM means that we can only read 512 bytes
   from NAND flash into SRAM after reset.  Preinitialization executes
   before SDRAM initialization, so we have to copy APEX from NAND
   flash into SRAM which limits us to about 64KiB (IIRC).  Once APEX
   is copied to SRAM, we continue execution, initialize SDRAM and then
   call the bonafide relocation routine that copies the loader from
   NAND flash to SDRAM where we can get crazy.

*/

#include <config.h>
#include <apex.h>
#include <asm/bootstrap.h>
#include "mach/hardware.h"
#include "mach/drv-nand.h"

#include <debug_ll.h>


/* wait_on_busy

   wait for the flash device to become ready.  Using this function
   makes the code smaller.

*/

static void __naked __section (.arel.earlyfn) wait_on_busy (void)
{
  while (NAND_ISBUSY)
    ;
  __asm volatile ("mov pc, lr");
}

void __naked __section (.arel.early) relocate_early (void)
{
  unsigned long pc;

  __asm volatile ("mov %0, pc" : "=r" (pc));

	/* Also, only perform this special relocation when we're
	   running from the base of SRAM.  */
  if ((pc >> 12) != (0xb0000000>>12))
    __asm volatile ("b relocate_early_exit");

  PUTC ('N');

	/* Relocate the whole loader from NAND to SRAM */
  {
    int cPages = (&APEX_VMA_COPY_END - &APEX_VMA_COPY_START + 511)/512;
    void* pv = (void*) (0xb0000000 + 512);
    int cAddr = NAM_DECODE (BOOT_PBC);
    int iPage;

    NAND_CS_ENABLE;

    for (iPage = 1; iPage < cPages; ++iPage) {
      NAND_CLE = NAND_Reset;
      wait_on_busy ();

      NAND_CLE = NAND_Read1;
      NAND_ALE = 0;
      {
	int page = iPage;
	int i;
	for (i = cAddr - 1; i--; ) {
	  NAND_ALE = page & 0xff;
	  page >>= 8;
	}
      }
      wait_on_busy ();

      NAND_CLE = NAND_Read1;

      {
	int cb;
	for (cb = 512; cb--; )
	  *((char*) pv++) = NAND_DATA;
      }

    }
  }

  NAND_CS_DISABLE;

  PUTC ('n');

  __asm volatile ("b relocate_early_exit");
}

void __naked __section (.arel.earlyex) relocate_early_exit (void)
{
}
