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


void get_memory_map(void)
{
  	u32* pl;
	int i;

	/* Clear all memory region identifiers */
	for(i = 0; i < NUM_MEM_AREAS; i++)
		memory_map[i].used = 0;

	/* Mark the blocks in reverse order */
	for (pl = (u32*) (MEMORY_END - CB_BLOCK);
	     pl >= (u32*) MEMORY_START;
	     pl -= CB_BLOCK/sizeof (*pl)) {
	  *pl = (u32) pl;
	}

	/* Identify contiguous, unaliases blocks  */
	i = 0;
	for (pl = (u32*) MEMORY_START;
	     pl != (u32*) MEMORY_END && i < NUM_MEM_AREAS;
	     pl += CB_BLOCK/sizeof (*pl)) {
		if (testram ((u32) pl) != 0)
			continue;
		if (*pl == (u32) pl) {
		  	if (memory_map[i].used == 0) {
				memory_map[i].start = (u32) pl;
				memory_map[i].used = 1;
				memory_map[i].len = CB_BLOCK;
			}
			else
			  memory_map[i].len += CB_BLOCK;
		}
		else
			if (memory_map[i].used)
				++i;
	}

	/* Show the memory map */
	SerialOutputString("Memory map:\n");
	for(i = 0; i < NUM_MEM_AREAS; i++) {
		if(memory_map[i].used) {
#if defined (DO_CLEAR_MEM)
		  	clear_mem (memory_map[i].start, memory_map[i].len);
#endif
			SerialOutputString("  @ 0x");
			SerialOutputHex(memory_map[i].start);
			SerialOutputString(" 0x");
			SerialOutputHex(memory_map[i].len);
			SerialOutputString(" (");
			SerialOutputDec(memory_map[i].len / (1024 * 1024));
			SerialOutputString(" MiB)\n");
		}
	}
}


static int memory_probe (void)
{
  extern char APEX_VMA_START;
  extern char APEX_END;
  unsigned long* pl;
  int i;

  memset (&regions, 0, sizeof (regions));

	/* Mark */
  for (pl = (unsigned long*) (CONFIG_MEM_BANK0_START
			      + CONFIG_MEM_BANK0_LENGTH
			      - CB_BLOCK
			      + (&APEX_END - &APEX_VMA_START));
       pl >= (unsigned long*) CONFIG_MEM_BANK0_START;
       pl -= CB_BLOCK/sizeof (*pl))
    *pl = (unsigned long) pl;

	/* Identify */
  i = 0;
  for (pl = (unsigned long*) (CONFIG_MEM_BANK0_START
			      + (&APEX_END - &APEX_VMA_START));
       pl < (unsigned long*) (CONFIG_MEM_BANK0_START + CONFIG_MEM_BANK0_LENGTH)
	 && i < sizeof (regions)/sizeof (struct mem_region);
       pl += CB_BLOCK/sizeof (*pl)) {
    //    if (testram ((u32) pl) != 0)
    //      continue;
    if (*pl == (unsigned long) pl) {
      if (region[i].cb == 0) {
	region[i].pv = (void*) pl - (&APEX_END - &APEX_VMA_START);
	region[i].cb = CB_BLOCK;
      }
      else
	region[i].cb += CB_BLOCK;
    }
    else
      if (region[i].cb)
	++i;
  }

  printf ("\r\nmemory\r\n");

  for (i = 0; i < sizeof (regions)/sizeof (struct mem_region); ++i) {
    printf (" 0x%08p 0x%08x (%d MiB)\r\n", 
	    regions[i].pv, regions[i].cb, regions[i].cb/(1024*1024));

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

