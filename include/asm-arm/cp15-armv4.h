/* cp15-armv4.h

   written by Marc Singer
   13 Jan 2006

   Copyright (C) 2006 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__CP15_ARMV4_H__)
#    define   __CP15_ARMV4_H__

#define _DLEN(l)	((l>>(12+0))&3)
#define _DSIZE(l)	((l>>(12+6))&7)
#define _DASSOC(l)	((l>>(12+3))&7)
#define _DM(l)		((l>>(12+2))&1)

/* *** This code may not work on the ARM720T.  According to the TRM
   *** documentation, the only operation available in the cache is
   *** invalidatation of the combined cache in one statement.
   *** Cleaning isn't addressed at all. */

#define CLEANALL_DCACHE\
  ({ unsigned long cache; int set, index; int linelen; int assoc;\
    __asm volatile ("mrc p15, 0, %0, c0, c0, 1" : "=r" (cache));\
    linelen = _DLEN(cache) + 3;\
    assoc = 32 - _DASSOC(cache);\
/*    if (DM(l)) */ \
/*      assoc = 3*assoc/2; */ \
    for (set = 1<<(_DSIZE(cache) + 6 - _DASSOC(cache) - _DLEN(cache)); \
	 set--; )\
      for (index = 1<<_DASSOC(cache); index--;) {\
	 __asm volatile ("mcr p15, 0, %0, c7, c10, 2" \
			:: "r" ((index<<assoc)|(set<<linelen))); } })


	/* ---- Some function are not available */

#undef INVALIDATE_ICACHE_I
#undef INVALIDATE_DCACHE_I
#undef INVALIDATE_CACHE_VA
#undef INVALIDATE_CACHE_I
#undef CLEAN_CACHE_VA
#undef CLEAN_CACHE_I
#undef CLEAN_INV_CACHE_VA
#undef CLEAN_INV_CACHE_I

#if defined (CONFIG_CPU_ARM720T)
# undef INVALIDATE_ICACHE_VA
# undef INVALIDATE_ICACHE_i
# undef INVALIDATE_DCACHE_VA
# undef INVALIDATE_DCACHE_i
#endif


#endif  /* __CP15_ARMV4_H__ */
