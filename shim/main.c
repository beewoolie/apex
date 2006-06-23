/* main.c

   written by Marc Singer
   23 Jun 2006

   Copyright (C) 2006 Marc Singer

   -----------
   DESCRIPTION
   -----------

   ARM kernel boot shim.

   This code may be prepended to a 2.6 Linux kernel in order to setup
   the ATAGS and the machine type in the EXTREME circumstance that the
   platform bootloader cannot be replaced.

*/

#define __KERNEL__

#include "config.h"
#include "types.h"		/* include/asm-arm/types.h */
#include "setup.h"		/* include/asm-arm/setup.h */

#define H_SIZE(pv)	(((struct tag_header*)pv)->size)
#define H_TAG(pv)	(((struct tag_header*)pv)->tag)

#define P_CORE(pv)	((struct tag_core*)(pv + sizeof (struct tag_header)))
#define P_MEM32(pv)	((struct tag_mem32*)(pv + sizeof (struct tag_header)))
#define P_CMDLINE(pv)	((struct tag_cmdline*)(pv + sizeof (struct tag_header)))

#define NAKED		__attribute__((naked))

const char __attribute__((section(".rodata"))) cmdline[] = COMMANDLINE;

void NAKED __attribute__((section(".boot"))) boot (u32 r0, u32 r1, u32 r2)
{
  __asm volatile (" nop");
}

int NAKED start (void)
{
  void* pv = (void*) PHYS_PARAMS;
  extern char SHIM_VMA_END;

	/* Always start with the CORE tag */
  H_SIZE(pv)		= tag_size (tag_core);
  H_TAG(pv)		= ATAG_CORE;
  P_CORE(pv)->flags = 0;
  P_CORE(pv)->pagesize = 0;
  P_CORE(pv)->rootdev = 0;
  pv += H_SIZE(pv)*4;

	/* Memory tags are always second */
  H_SIZE(pv)		= tag_size (tag_mem32);
  H_TAG(pv)		= ATAG_MEM;
  P_MEM32(pv)->size	= RAM_BANK0_LENGTH;
  P_MEM32(pv)->start	= RAM_BANK0_START;
  pv += H_SIZE(pv)*4;

#if defined (RAM_BANK1_START)
  H_SIZE(pv)		= tag_size (tag_mem32);
  H_TAG(pv)		= ATAG_MEM;
  P_MEM32(pv)->size	= RAM_BANK1_LENGTH;
  P_MEM32(pv)->start	= RAM_BANK1_START;
  pv += H_SIZE(pv)*4;
#endif

	/* Command line */
  H_SIZE(pv)		= tag_size(tag_cmdline) + (sizeof (cmdline)+1+3)/4;
  H_TAG(pv)		= ATAG_CMDLINE;
  {
    int i;
    for (i = 0; i < sizeof (cmdline); ++i)
      P_CMDLINE(pv)->cmdline[i] = cmdline[i];
  }
  pv += H_SIZE(pv)*4;

	/* End */
  H_SIZE(pv)		= 0;
  H_TAG(pv)		= ATAG_NONE;

	/* Pass control to the kernel */
  boot (0, MACH_TYPE, PHYS_PARAMS);
}

