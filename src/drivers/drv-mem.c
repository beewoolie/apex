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

struct mem_descriptor {
  void* pv;
  size_t cb;
  size_t ib;
};

struct mem_region {
  void* pv;
  size_t cb;
};

static struct mem_region regions[32];
static struct mem_descriptor descriptors[2];

#define CB_BLOCK	     (1024*1024)

static int memory_scan (int i, unsigned long start, unsigned long length)
{
	/* Mark */
  for (pl = (unsigned long*) (start + length - CB_BLOCK
			      + (&APEX_VMA_END - &APEX_VMA_START));
       pl >= (unsigned long*) start;
       pl -= CB_BLOCK/sizeof (*pl))
    *pl = (unsigned long) pl;

	/* Identify */
  for (pl = (unsigned long*) (START + (&APEX_VMA_END - &APEX_VMA_START));
       pl < (unsigned long*) (start + length)
	 && i < sizeof (regions)/sizeof (struct mem_region);
       pl += CB_BLOCK/sizeof (*pl)) {
    //    if (testram ((u32) pl) != 0)
    //      continue;
    if (*pl == (unsigned long) pl) {
      if (regions[i].cb == 0)
	regions[i].pv = (void*) pl - (&APEX_VMA_END - &APEX_VMA_START);
      regions[i].cb += CB_BLOCK;
    }
    else
      if (regions[i].cb)
	++i;
  }
}

static int memory_probe (void)
{
  extern char APEX_VMA_START;
  extern char APEX_VMA_END;
  unsigned long* pl;
  int i;

  memset (&regions, 0, sizeof (regions));

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

static unsigned long memory_open (struct open_d* d)
{
  int fh;

  for (fh = 0; fh < sizeof (descriptors)/sizeof(struct mem_descriptor); ++fh)
    if (descriptors[fh].cb == 0)
      break;

  if (fh >= sizeof (descriptors)/sizeof(struct mem_descriptor))
    return -1;

  descriptors[fh].pv = (void*) d->start;
  descriptors[fh].cb = d->length;
  descriptors[fh].ib = 0;
      
  return fh;
}

static void memory_close (unsigned long fh)
{
  descriptors[fh].cb = 0;
} 

static ssize_t memory_read (unsigned long fh, void* pv, size_t cb)
{
  if (descriptors[fh].ib + cb > descriptors[fh].cb)
    cb = descriptors[fh].cb - descriptors[fh].ib;

  memcpy (pv, descriptors[fh].pv + descriptors[fh].ib, cb);
  descriptors[fh].ib += cb;
  
  return cb;
}

static ssize_t memory_write (unsigned long fh, const void* pv, size_t cb)
{
  if (descriptors[fh].ib + cb > descriptors[fh].cb)
    cb = descriptors[fh].cb - descriptors[fh].ib;

  memcpy (descriptors[fh].pv + descriptors[fh].ib, pv, cb);
  descriptors[fh].ib += cb;
  
  return cb;
}

static size_t memory_seek (unsigned long fh, ssize_t ib, int whence)
{
  switch (whence) {
  case SEEK_SET:
    break;
  case SEEK_CUR:
    ib += descriptors[fh].ib;
    break;
  case SEEK_END:
    ib = descriptors[fh].cb - ib;
    break;
  }

  if (ib < 0)
    ib = 0;
  if (ib > descriptors[fh].cb)
    ib = descriptors[fh].cb;

  descriptors[fh].ib = ib;

  return (size_t) ib;
}


static __driver_0 struct driver_d memory_driver = {
  .name = "memory",
  .description = "memory driver (SDRAM/DRAM/SRAM)",
  //  .flags = DRIVER_ | DRIVER_CONSOLE,
  .probe = memory_probe,
  .open = memory_open,
  .close = memory_close,
  .read = memory_read,
  .write = memory_write,
  .seek = memory_seek,
};

