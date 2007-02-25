/* cp15-armv6.h

   written by Marc Singer
   24 Feb 2007

   Copyright (C) 2007 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__CP15_ARMV6_H__)
#    define   __CP15_ARMV6_H__

#define FLUSH_PREFETCH(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c5, 4\n\t" :: "r" (i))
#define FLUSH_BRANCH_CACHE(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c5, 6\n\t" :: "r" (i))
#define FLUSH_BRANCH_CACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c5, 7\n\t" :: "r" (a))

#define CLEAN_DCACHE\
  __asm volatile ("mcr p15, 0, %0, c7, c10, 1\n\t" :: "r" (0))
#define DATA_SYNCHRONIZATION_BARRIER\
  __asm volatile ("mcr p15, 0, %0, c7, c10, 4\n\t" :: "r" (0))
#define DATA_MEMORY_BARRIER\
  __asm volatile ("mcr p15, 0, %0, c7, c10, 5\n\t" :: "r" (0))
#define LOAD_CACHE_DIRTY_STATUS(v)\
  __asm volatile ("mrc p15, 0, %0, c7, c10, 6\n\t" :  "=r" (v))

#define LOAD_BLOCK_TRANSFER_STATUS(v)\
  __asm volatile ("mrc p15, 0, %0, c7, c12, 4\n\t" :  "=r" (v))
#define STOP_PREFETCH_RANGE(v)\
  __asm volatile ("mcr p15, 0, %0, c7, c12, 5\n\t" :: "r" (0))

#define CLEAN_INV_DCACHE\
  __asm volatile ("mcr p15, 0, %0, c7, c14, 1\n\t" :: "r" (0))


#undef DRAIN_WRITE_BUFFER
#define DRAIN_WRITE_BUFFER DATA_SYNCHRONIZATION_BARRIER
#undef INVALIDATE_CACHE_VA
#undef INVALIDATE_CACHE_I
#undef CLEAN_CACHE_VA
#undef CLEAN_CACHE_I
#undef PREFETCH_ICACHE_VA


#endif  /* __CP15_ARMV6_H__ */
