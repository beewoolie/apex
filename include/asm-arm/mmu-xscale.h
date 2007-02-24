/* mmu-xscale.h
     $Id$

   written by Marc Singer
   13 Jan 2006

   Copyright (C) 2006 Marc Singer

   -----------
   DESCRIPTION
   -----------

   This cache flush alrogithm comes from Nicolas Pitre's work in the
   Linux kernel as well as from email he sent to the linux arm ports
   list.  We flush twice since we don't know how the caller, perhaps
   RedBoot, was flushing cache.  Therefore, we perform the ping-pong
   flush as described by Nico so that we are certain that the cache
   has been purged.

*/

#if !defined (__MMU_XSCALE_H__)
#    define   __MMU_XSCALE_H__

/* ----- Includes */

#include "mach/memory.h"

/* This coprocessor wait is require on the XScale when it is necessary
   to guarantee that the coprocessor has finished before continuing.
   AFAIK, this is not required on other ARM architectures. */

#undef CP15_WAIT
#define CP15_WAIT\
 ({ unsigned long v; \
    __asm volatile ("mrc p15, 0, %0, c2, c0, 0\n\t" \
		    "mov %0, %0\n\t" \
		    "sub pc, pc, #4" : "=r" (v)); })


  /* The cache cleaning algorithm comes from the XScale documentation
     as well as from the Linux kernel.  */
#undef  MVA_CACHE_CLEAN
#define MVA_CACHE_CLEAN		(0xfffe0000)

#define CACHE_LINE_SIZE		(32)
#define CACHE_SIZE		(32768)

#define _CLEANALL_DCACHE(b) \
  ({ unsigned long p = MVA_CACHE_CLEAN ^ (b ? CACHE_SIZE : 0);\
     int line;\
     for (line = CACHE_SIZE/CACHE_LINE_SIZE; line--; p += CACHE_LINE_SIZE) \
       __asm volatile ("mcr p15, 0, %0, c7, c2, 5" :: "r" (p)); })

#define CLEANALL_DCACHE\
  ({ _CLEANALL_DCACHE (0) ; _CLEANALL_DCACHE (1); })

#define ENABLE_DCACHE_LOCK\
    ({ __asm volatile ("mcr p15, 0, %0, c9, c2, 0\n\t" :: "r" (1));\
       CP15_WAIT; })
#define DISABLE_DCACHE_LOCK\
    ({ __asm volatile ("mcr p15, 0, %0, c9, c2, 0\n\t" :: "r" (0));\
       CP15_WAIT; })

#define LOCK_DCACHE_VA(a)\
    __asm volatile ("mcr p15, 0, %0, c7, c10, 1\n\t"\
		    "mcr p15, 0, %0, c7,  c6, 1\n\t" :: "r" (a))

#define LOCK_ICACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c9, c1, 0\n\t" :: "r" (a))


#endif  /* __MMU_XSCALE_H__ */
