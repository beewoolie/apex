/* drv-mem.c
     $Id$

   written by Marc Singer
   3 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

   Memory block IO driver.

*/

#include <driver.h>
#include <linux/string.h>
#include <apex.h>
#include <config.h>

#if defined (CONFIG_ATAG)
# include <atag.h>
#endif

struct mem_region {
  unsigned long start;
  size_t length;
};

static struct mem_region regions[32];

#define CB_BLOCK	     (1024*1024)

static int memory_scan (int i, unsigned long start, unsigned long length)
{
  extern char APEX_VMA_START;
  extern char APEX_VMA_END;
  unsigned long* pl;

	/* Mark */
  for (pl = (unsigned long*) (start + length - CB_BLOCK
			      + (&APEX_VMA_END - &APEX_VMA_START));
       pl >= (unsigned long*) start;
       pl -= CB_BLOCK/sizeof (*pl))
    *pl = (unsigned long) pl;

	/* Identify */
  for (pl = (unsigned long*) (start + (&APEX_VMA_END - &APEX_VMA_START));
       pl < (unsigned long*) (start + length)
	 && i < sizeof (regions)/sizeof (struct mem_region);
       pl += CB_BLOCK/sizeof (*pl)) {
    //    if (testram ((u32) pl) != 0)
    //      continue;
    if (*pl == (unsigned long) pl) {
      if (regions[i].length == 0)
	regions[i].start
	  = (unsigned long) pl - (&APEX_VMA_END - &APEX_VMA_START);
      regions[i].length += CB_BLOCK;
    }
    else
      if (regions[i].length)
	++i;
  }
  return i;
}

static int memory_probe (void)
{
  int i;

  i = 0;
#if defined (CONFIG_MEM_BANK0_START)
  i = memory_scan (i, CONFIG_MEM_BANK0_START, CONFIG_MEM_BANK0_LENGTH);
#endif

#if defined (CONFIG_MEM_BANK1_START)
  i = memory_scan (i, CONFIG_MEM_BANK1_START, CONFIG_MEM_BANK1_LENGTH);
#endif

#if defined (CONFIG_MEM_BANK2_START)
  i = memory_scan (i, CONFIG_MEM_BANK2_START, CONFIG_MEM_BANK2_LENGTH);
#endif

#if defined (CONFIG_MEM_BANK3_START)
  i = memory_scan (i, CONFIG_MEM_BANK3_START, CONFIG_MEM_BANK3_LENGTH);
#endif

#if 0
  printf ("\r\nmemory\r\n");

  for (i = 0; i < sizeof (regions)/sizeof (struct mem_region); ++i)
    if (regions[i].cb)
      printf (" 0x%p 0x%08x (%d MiB)\r\n", 
	      regions[i].pv, regions[i].cb, regions[i].cb/(1024*1024));
#endif

  return 0;			/* Present and initialized */
}

static int memory_open (struct descriptor_d* d)
{
  /* Should do a bounds check */
  return 0;			/* OK */
}

static ssize_t memory_read (struct descriptor_d* d, void* pv, size_t cb)
{
  if (d->index + cb > d->length)
    cb = d->length - d->index;

  memcpy (pv, (void*) (d->start + d->index), cb);
  d->index += cb;
  
  return cb;
}

static ssize_t memory_write (struct descriptor_d* d, const void* pv, size_t cb)
{
  if (d->index + cb > d->length)
    cb = d->length - d->index;

  memcpy ((void*) (d->start + d->index), pv, cb);
  d->index += cb;
  
  return cb;
}


static __driver_0 struct driver_d memory_driver = {
  .name = "memory",
  .description = "memory driver (SDRAM/DRAM/SRAM)",
  .probe = memory_probe,
  .open = memory_open,
  .close = close_descriptor,
  .read = memory_read,
  .write = memory_write,
  .seek = seek_descriptor,
};


#if defined (CONFIG_ATAG)

static struct tag* atag_memory (struct tag* p)
{
  int i;

  for (i = 0; i < sizeof (regions)/sizeof (struct mem_region); ++i) {
    if (!regions[i].length)
      break;

    p->hdr.tag = ATAG_MEM;
    p->hdr.size = tag_size (tag_mem32);

    p->u.mem.start = regions[i].start;
    p->u.mem.size  = regions[i].length;
			
//    printf (" mem 0x%08x # 0x%08x\r\n", p->u.mem.start, p->u.mem.size);
    p = tag_next (p);
  }

  return p;
}

static __atag_1 struct atag_d _atag_memory = { atag_memory };


#endif
