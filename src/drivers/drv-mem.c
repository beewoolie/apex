/* drv-mem.c
     $Id$

   written by Marc Singer
   3 Nov 2004

   Copyright (C) 2004 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA.

   -----------
   DESCRIPTION
   -----------

   Memory block IO driver.

*/

#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <apex.h>
#include <config.h>

#if defined (CONFIG_ATAG)
# include <atag.h>
#endif

#include <mach/memory.h>

#include <debug_ll.h>

//#define TALK

#if defined (TALK)
# define PRINTF(f...) printf (f)
#else
# define PRINTF(f...) do {} while (0)
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
  extern char APEX_VMA_PROTECT_END;
  unsigned long* pl;

  PUTC_LL ('k');
  PRINTF ("  marking\n");

	/* Mark */
  for (pl = (unsigned long*) (start + length - CB_BLOCK
			      + (&APEX_VMA_PROTECT_END - &APEX_VMA_START));
       1;
       pl -= CB_BLOCK/sizeof (*pl)) {
//    PRINTF ("   %p\n", pl);
    *pl = (unsigned long) pl;

				/* Prevents integer wrapping at zero */
    if ((((unsigned long) pl) & ~(CB_BLOCK - 1)) <= start)
      break;
  }

  PUTC_LL ('i');
  PRINTF ("  identifying\n");

	/* Identify */
  for (pl = (unsigned long*) (start 
			      + (&APEX_VMA_PROTECT_END - &APEX_VMA_START));
       pl < (unsigned long*) (start + length)
	 && i < sizeof (regions)/sizeof (struct mem_region);
       pl += CB_BLOCK/sizeof (*pl)) {
//    PRINTF ("   %p\n", pl);
    //    if (testram ((u32) pl) != 0)
    //      continue;
    if (*pl == (unsigned long) pl) {
      if (regions[i].length == 0)
	regions[i].start
	  = (unsigned long) pl - (&APEX_VMA_PROTECT_END - &APEX_VMA_START);
      regions[i].length += CB_BLOCK;
    }
    else
      if (regions[i].length)
	++i;
  }
  return i;
}

static void memory_init (void)
{
  int i;

  PUTC_LL ('M');

  i = 0;
#if defined (RAM_BANK0_START)
  PUTC_LL ('0');
  PRINTF ("Scanning bank 0 %x %x\n", 
	  RAM_BANK0_START, RAM_BANK0_LENGTH);
  i = memory_scan (i, RAM_BANK0_START, RAM_BANK0_LENGTH);
  PRINTF ("  %d blocks\n", i);
#endif

#if defined (RAM_BANK1_START)
  PUTC_LL ('1');
  PRINTF ("Scanning bank 1 %x %x\n", 
	  RAM_BANK1_START, RAM_BANK1_LENGTH);
  i = memory_scan (i, RAM_BANK1_START, RAM_BANK1_LENGTH);
#endif

#if defined (RAM_BANK2_START)
  PUTC_LL ('2');
  i = memory_scan (i, RAM_BANK2_START, RAM_BANK2_LENGTH);
#endif

#if defined (RAM_BANK3_START)
  PUTC_LL ('3');
  i = memory_scan (i, RAM_BANK3_START, RAM_BANK3_LENGTH);
#endif

  PUTC_LL ('m');
}

#if !defined (CONFIG_SMALL)
static void memory_report (void)
{
  int i;

  printf ("  memory:");

  for (i = 0; i < sizeof (regions)/sizeof (struct mem_region); ++i)
    if (regions[i].length) {
      if (i)
	printf ("         ");
      printf (" 0x%lx 0x%08x (%d MiB)\n", 
	      regions[i].start, regions[i].length, 
	      regions[i].length/(1024*1024));
    }
}
#endif

static int memory_open (struct descriptor_d* d)
{
  /* Should do a bounds check */
  return 0;			/* OK */
}

static ssize_t memory_read (struct descriptor_d* d, void* pv, size_t cb)
{
  if (d->index + cb > d->length)
    cb = d->length - d->index;

#if 0
  /* *** Larger version of the memory read function which serves to
     properly read from memories (e.g. broken CF interfaces) where the
     chip select must toggle for every access.  This is a lowest
     common denominator, and will tend to be pretty slow. */
  {
    int i = cb;
    unsigned char* pbSrc = (unsigned char*) (d->start + d->index);
    while (i--)
      *(unsigned char*) pv = *pbSrc++, ++pv;
  }
#else
  memcpy (pv, (void*) (d->start + d->index), cb);
#endif
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


static __driver_1 struct driver_d memory_driver = {
  .name = "memory",
  .description = "generic RAM driver",
  .open = memory_open,
  .close = close_helper,
  .read = memory_read,
  .write = memory_write,
  .seek = seek_helper,
};

static __service_4 struct service_d memory_service = {
  .init = memory_init,
#if !defined (CONFIG_SMALL)
  .report = memory_report,
#endif
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
			
//    printf (" mem 0x%08x # 0x%08x\n", p->u.mem.start, p->u.mem.size);
    p = tag_next (p);
  }

  return p;
}

static __atag_1 struct atag_d _atag_memory = { atag_memory };


#endif
