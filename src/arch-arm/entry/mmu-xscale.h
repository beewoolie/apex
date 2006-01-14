/* mmu-xscale.h
     $Id$

   written by Marc Singer
   13 Jan 2006

   Copyright (C) 2006 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__MMU_XSCALE_H__)
#    define   __MMU_XSCALE_H__

/* ----- Includes */

#include "mach/memory.h"

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

	/* Flush all entries in data cache */
#define CACHE_FLUSH\
  ({ int line; unsigned long p = MVA_CACHE_CLEAN;\
     for (line = 0; line < 1024; ++line, p += 32) \
       __asm volatile ("mcr p15, 0, %0, c7, c2, 5" :: "r" (p)); })

#endif  /* __MMU_XSCALE_H__ */
