/* drv-mem.h

   written by Marc Singer
   23 Oct 2006

   Copyright (C) 2006 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__DRV_MEM_H__)
#    define   __DRV_MEM_H__

/* ----- Includes */

/* ----- Types */

struct mem_region {
  unsigned long start;
  size_t length;
};

extern struct mem_region memory_regions[32];

int memory_scan (struct mem_region* regions, int c,
		 unsigned long start, unsigned long length);

/* ----- Globals */

/* ----- Prototypes */



#endif  /* __DRV_MEM_H__ */
