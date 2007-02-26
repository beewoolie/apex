/* mmu.h

   written by Marc Singer
   25 Feb 2007

   Copyright (C) 2007 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__MMU_H__)
#    define   __MMU_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

void* alloc_uncached (size_t cb, size_t alignment);
void* alloc_uncached_top_retain (size_t cb, size_t alignment);
void mmu_protsegment (void* pv, int cacheable, int bufferable);

#endif  /* __MMU_H__ */
