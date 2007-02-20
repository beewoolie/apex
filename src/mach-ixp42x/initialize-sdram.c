/* initialize-sdram.c

   written by Marc Singer
   17 Feb 2007

   Copyright (C) 2007 Marc Singer

   -----------
   DESCRIPTION
   -----------

   This is a special command/function to initialize SDRAM after the
   system has been started.  While it would be nice to be able to
   perform this magic before the system were running commands on the
   command line, there isn't really a good way to do this.  From the
   XScale Core Developer's Manual:

    There are several requirements for locking down code:
      7.[sic] the routine used to lock lines down in the cache must be
	placed in non-cacheable memory, which means the MMU is
	enabled. As a corollary: no fetches of cacheable code should
	occur while locking instructions into the cache.the code being
	locked into the cache must be cacheable.

      8.[sic] the instruction cache must be enabled and invalidated
	prior to locking down lines

    Failure to follow these requirements will produce unpredictable
    results when accessing the instruction cache.

    System programmers should ensure that the code to lock
    instructions into the cache does not reside closer than 128 bytes
    to a non-cacheable/cacheable page boundary. If the processor
    fetches ahead into a cacheable page, then the first requirement
    noted above could be violated.

  So, the plan is to copy APEX to a new region of SDRAM, 0x0, make
  this memory uncached, then jump to it.  The routine the locks the
  original copy of itself into both caches and returns to that version
  of itself.  At this point, we can reinitialize the SDRAM controller
  at will as long as we don't leave the function we're executing and
  we don't attempt to use the stack.

*/

#include <config.h>
#include <asm/bootstrap.h>
#include <apex.h>
#include <command.h>
#include <service.h>
#include "hardware.h"
#include <asm/mmu.h>

#include <debug_ll.h>

static int cmd_initialize_sdram (int argc, const char** argv)
{
  unsigned long s = (unsigned long) &APEX_VMA_COPY_START;
  unsigned long d = (unsigned long) 0;
  const unsigned long diff = d - s;

  PUTC ('A');

  CACHE_CLEAN;
//  __asm volatile ("mcr p15, 0, %0, c7, c5, 0" : : "r" (0));  // Inv. I cache
//  __asm volatile ("mcr p15, 0, %0, c7, c6, 0" : : "r" (0));  // Inv. D cache
//  __asm volatile ("mcr p15, 0, %0, c8, c7, 0" : : "r" (0));  // Inv. TLBs

  mmu_protsegment ((void*) d, 0, 0);			  /* Disable caching */
  TLB_PURGE;

  PUTC ('B');

		/* Copy APEX to uncacheable memory  */
  __asm volatile (
	       "0: ldmia %0!, {r3-r10}\n\t"
		  "stmia %1!, {r3-r10}\n\t"
		  "cmp %0, %2\n\t"
		  "bls 0b\n\t"
	       : "+r" (s),
		 "+r" (d)
	       :  "r" (&APEX_VMA_COPY_END)
	       : "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "cc"
	       );
  CACHE_CLEAN;			/* Force copy to SDRAM */

  /* *** Jump to 0x0 copy of APEX  */
  __asm volatile ("add pc, pc, %0\n\t" :: "r" (diff - 4));
  PUTC ('C');

  /* *** Perform lockdown on this function.  Also should read it into
     *** dcache.  */
  {
    unsigned long p = (unsigned long) (&cmd_initialize_sdram) & ~3;
    unsigned long c = 1024;

    //    CACHE_INVALIDATE_IBTB;
    for (; c; p += 32, c -= 32)
      CACHE_LOCK_I (p);
  }

  /* *** Jump back to original copy of APEX. */

   __asm volatile ("sub pc, pc, %0\n\t" :: "r" (diff + 4));
  PUTC ('X');
  PUTC ('Y');

  /* *** Reinitialize SDRAM. */
  /* *** Unlock caches */
  /* *** Reenable caching at 0x0 */
  /* *** Return. */

  PUTC ('E');

  mmu_protsegment ((void*) d, 1, 1);			  /* Enable caching */
  TLB_PURGE;

  PUTC ('\r');
  PUTC ('\n');

  return 0;
}

static __command struct command_d c_initialize_sdram = {
  .command = "sdram-init",
  .func = cmd_initialize_sdram,
  COMMAND_DESCRIPTION ("reinitialize the SDRAM controller")
  COMMAND_HELP(
"sdram-init\n"
"  reinitialize SDRAM controller using cache to hold the \n"
"  program code while SDRAM is off-line.\n"
  )
};
