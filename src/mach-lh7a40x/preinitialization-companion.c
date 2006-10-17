/* preinitialization-companion.c

   written by Marc Singer
   16 Oct 2006

   Copyright (C) 2006 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#include <config.h>
#include <apex.h>
#include <asm/bootstrap.h>
#include "mach/hardware.h"
#include "mach/drv-nand.h"

#include <debug_ll.h>


static __naked __section (.preinit) void wait_on_busy (void)
{
  while (NAND_ISBUSY)
    ;
  __asm volatile ("mov pc, lr");
}

void __naked __section (.preinit) preinitialization (void)
{
  unsigned long lr;

  __asm volatile ("mov %0, lr\n\t" : "=r" (lr));

#if defined (CONFIG_STARTUP_UART)
  UART_BRCON = 0x3;
  UART_FCON = UART_FCON_FEN | UART_FCON_WLEN8;
  UART_INTEN = 0x00; /* Mask interrupts */
  UART_CON = UART_CON_ENABLE;
#endif

	/* Also, only perform this special relocation when we're
	   running from the base of SRAM.  */
  if ((lr >> 12) != (0xb0000000>>12))
    __asm volatile ("mov pc, %0" : : "r" (lr));

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

  __asm volatile ("mov pc, %0" : : "r" (lr));
}
