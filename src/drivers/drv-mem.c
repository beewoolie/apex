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

static struct mem_descriptor descriptors[2];

static int memory_probe (void)
{
  /* Need to probe memory here so that we can pass information to
     kernel.  Also, it should be used to bound open() calls.   */

  printf ("probing 0x%08x # 0x%08x\r\n", 
	  CONFIG_MEM_BANK0_START, CONFIG_MEM_BANK0_LENGTH);

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

