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

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#undef  MVA_CACHE_CLEAN
#define MVA_CACHE_CLEAN		(0xfffe0000)

#define CACHE_LINE_SIZE		(32)
#define CACHE_SIZE		(32768)

#define CACHE_CLEAN_D(b) \
  ({ unsigned long p = MVA_CACHE_CLEAN ^ (b ? CACHE_SIZE : 0);\
     int line;\
     for (line = CACHE_SIZE/CACHE_LINE_SIZE; line--; p += CACHE_LINE_SIZE) \
       __asm volatile ("mcr p15, 0, %0, c7, c2, 5" :: "r" (p)); })

	/* Clean all sets in data cache twice as per recommendation. */
#define CACHE_CLEAN\
  ({ CACHE_CLEAN_D (0) ; CACHE_CLEAN_D (1); })

#define CACHE_UNLOCK\
  __asm volatile ("mcr p15, 0, %0, c9, c1, 1\n\t"\
		  "mcr p15, 0, %0, c9, c2, 1\n\t" :: "r" (0))

#define CACHE_LOCK_I(a)\
  __asm volatile ("mcr p15, 0, %0, c9, c1, 0\n\t" :: "r" (a))
#define CACHE_INVALIDATE_I(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c5, 1\n\t" :: "r" (a))

#endif  /* __MMU_XSCALE_H__ */
