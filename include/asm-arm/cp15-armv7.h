/* cp15-armv7.h

   written by Marc Singer
   24 Feb 2007

   Copyright (C) 2007 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   *** FIXME: these macros have not been edited for the platform

   We need:

    CLEANALL_DCACHE;
    INVALIDATE_ICACHE;
    INVALIDATE_DCACHE;
    INVALIDATE_TLB;
    DRAIN_WRITE_BUFFER;
    CP15_WAIT;

*/

#    define   __CP15_ARMV7_H__

#if !defined (__CP15_ARMV7_H__)
#    define   __CP15_ARMV7_H__

	/* ---- Cache control */

#define INVALIDATE_ICACHE\
  __asm volatile (
//	"mcr p15, 0, %0, c7, c1, 0\n\t" // inv I-cache inner sharable (SMP)
	"mcr p15, 0, %0, c7, c5, 0\n\t" // I+BTB cache invalidate
	:: "r" (0))
#define INVALIDATE_ICACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c5, 1\n\t" :: "r" (a))	// XXX
#define INVALIDATE_ICACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c5, 2\n\t" :: "r" (i))	// XXX
#define FLUSH_PREFETCH\
  __asm volatile ("mcr p15, 0, %0, c7, c5, 4\n\t" :: "r" (0))	// XXX
#define FLUSH_BRANCH_CACHE\
  __asm volatile ("mcr p15, 0, %0, c7, c5, 6\n\t" :: "r" (0))	// XXX
#define FLUSH_BRANCH_CACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c5, 7\n\t" :: "r" (a))	// XXX

#define INVALIDATE_DCACHE\
  __asm volatile ("mcr p15, 0, %0, c7, c6, 0\n\t" :: "r" (0))	// XXX
#define INVALIDATE_DCACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c6, 1\n\t" :: "r" (a))	// XXX
#define INVALIDATE_DCACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c6, 2\n\t" :: "r" (i))	// XXX

#define INVALIDATE_CACHE\
  __asm volatile ("mcr p15, 0, %0, c7, c7, 0\n\t" :: "r" (0))	// XXX

#define CLEAN_DCACHE\
  __asm volatile ("mcr p15, 0, %0, c7, c10, 0\n\t" :: "r" (0))	// XXX
#define CLEAN_DCACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c10, 1\n\t" :: "r" (a))	// XXX
#define CLEAN_DCACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c10, 2\n\t" :: "r" (i))	// XXX
#define DATA_SYNCHRONIZATION_BARRIER\
  __asm volatile ("mcr p15, 0, %0, c7, c10, 4\n\t" :: "r" (0))	// XXX
#define DATA_MEMORY_BARRIER\
  __asm volatile ("mcr p15, 0, %0, c7, c10, 5\n\t" :: "r" (0))	// XXX
#define LOAD_CACHE_DIRTY_STATUS(v)\
  __asm volatile ("mrc p15, 0, %0, c7, c10, 6\n\t" :  "=r" (v))	// XXX

#define LOAD_BLOCK_TRANSFER_STATUS(v)\
  __asm volatile ("mrc p15, 0, %0, c7, c12, 4\n\t" :  "=r" (v))	// XXX
#define STOP_PREFETCH_RANGE(v)\
  __asm volatile ("mcr p15, 0, %0, c7, c12, 5\n\t" :: "r" (0))	// XXX

#define PREFETCH_ICACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c13, 1\n\t" :: "r" (a))	// XXX

#define CLEAN_INV_DCACHE\
  __asm volatile ("mcr p15, 0, %0, c7, c14, 0\n\t" :: "r" (0))	// XXX
#define CLEAN_INV_DCACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c14, 1\n\t" :: "r" (a))	// XXX
#define CLEAN_INV_DCACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c14, 2\n\t" :: "r" (i))	// XXX

	/* ---- Cache lockdown */

//*** FIXME: it isn't clear whether or not we need to lock the cache
//*** or if we can.  Documentation for CP15 on the MX51 is absent.
#define UNLOCK_CACHE
  //  __asm volatile ("mcr p15, 0, %0, c9, c1, 1\n\t"
  //		  "mcr p15, 0, %0, c9, c2, 1\n\t" :: "r" (0))

	/* ---- TLB control */

#define INVALIDATE_TLB\
  __asm volatile ("mcr p15, 0, %0, c8, c7, 0\n\t" :: "r" (0))  // ok
#define INVALIDATE_ITLB\
  __asm volatile ("mcr p15, 0, %0, c8, c5, 0\n\t" :: "r" (0))	// XXX
#define INVALIDATE_ITLB_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c8, c5, 1\n\t" :: "r" (a))	// XXX
#define INVALIDATE_DTLB\
  __asm volatile ("mcr p15, 0, %0, c8, c6, 0\n\t" :: "r" (0))	// XXX
#define INVALIDATE_DTLB_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c8, c6, 1\n\t" :: "r" (a))	// XXX

/* --- ARMV7 */

#define CLEANALL_DCACHE CLEAN_INV_DCACHE

#undef DRAIN_WRITE_BUFFER
#define DRAIN_WRITE_BUFFER DATA_SYNCHRONIZATION_BARRIER
#undef INVALIDATE_CACHE_VA
#undef INVALIDATE_CACHE_I
#undef CLEAN_CACHE_VA
#undef CLEAN_CACHE_I
#undef PREFETCH_ICACHE_VA

#define STORE_REMAP_PERIPHERAL_PORT(v)\
  __asm volatile ("mcr p15, 0, %0, c15, c2, 4\n\t" :: "r" (v))	// XXX
#define LOAD_REMAP_PERIPHERAL_PORT(v)\
  __asm volatile ("mrc p15, 0, %0, c15, c2, 4\n\t" : "=r" (v))	// XXX


#endif  /* __CP15_ARMV7_H__ */
