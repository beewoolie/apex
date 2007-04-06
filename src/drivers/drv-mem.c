/* drv-mem.c

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


   Probing and our BSS/Data sections
   ---------------------------------

   There have been some problems with the way we probe to find unique
   SDRAM.  The present implementation has only the restrictions that

    1) the last word of the stack must be available during probing
    2) that the CB_BLOCK is larger than the size of the loader footprint
    3) that the loader is aligned to CB_BLOCK.
    4) if loader is longer than a CB_BLOCK, only the first CB_BLOCK of
       data is protected from probes.

*/

#include <config.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <error.h>
#include <command.h>
#include <apex.h>
#if defined (CONFIG_MMU)
# include <asm/cp15.h>
#endif

#if defined (CONFIG_ATAG)
# include <atag.h>
#endif

#include <drv-mem.h>
#include <mach/memory.h>

#include <debug_ll.h>

//#define TALK

#if defined (TALK)
# define PRINTF(f...) printf (f)
#else
# define PRINTF(f...) do {} while (0)
#endif

struct mem_region memory_regions[32];

#if defined (CONFIG_CMD_MEMLIMIT)
static unsigned memlimit;
#endif

#define CB_BLOCK	     (1024*1024)

int memory_scan (struct mem_region* regions, int c,
		 unsigned long start, unsigned long length)
{
  extern char APEX_VMA_START;
  extern char APEX_VMA_PROBE_END;
  unsigned long* pl;
  int i = 0;

//  length = 1024*1024*36;
  PUTC_LL ('k');
  PRINTF ("  marking start %lx length %lx probe %x\n", start, length,
	  &APEX_VMA_PROBE_END - &APEX_VMA_START);

	/* Mark */
  for (pl = (unsigned long*) (start + length - CB_BLOCK
			      + (&APEX_VMA_PROBE_END - &APEX_VMA_START));
       1;
       pl -= CB_BLOCK/sizeof (*pl)) {
    PRINTF ("   %p\n", pl);
    *pl = (unsigned long) pl;

				/* Prevents integer wrapping at zero */
    if ((((unsigned long) pl) & ~(CB_BLOCK - 1)) <= start)
      break;
  }

  PUTC_LL ('i');
  PRINTF ("  identifying\n");

#if defined (CONFIG_MMU)
  CLEANALL_DCACHE;
#endif

	/* Identify */
  for (pl = (unsigned long*) (start
			      + (&APEX_VMA_PROBE_END - &APEX_VMA_START));
       pl < (unsigned long*) (start + length) && i < c;
       pl += CB_BLOCK/sizeof (*pl)) {
    PRINTF ("   %p %08lx\n", pl, *pl);
    //    if (testram ((u32) pl) != 0)
    //      continue;
    if (*pl == (unsigned long) pl) {
      if (regions[i].length == 0)
	regions[i].start
	  = (unsigned long) pl - (&APEX_VMA_PROBE_END - &APEX_VMA_START);
      regions[i].length += CB_BLOCK;
    }
    else
      if (regions[i].length)
	++i;
  }
  if (regions[i].length)
    ++i;
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
  i = memory_scan (memory_regions + i,
		   sizeof (memory_regions)/sizeof (*memory_regions) - i,
		   RAM_BANK0_START, RAM_BANK0_LENGTH);
  PRINTF ("  %d block%s\n", i, i != 1 ? "s" : "");
#endif

#if defined (RAM_BANK1_START)
  PUTC_LL ('1');
  PRINTF ("Scanning bank 1 %x %x\n",
	  RAM_BANK1_START, RAM_BANK1_LENGTH);
  i = memory_scan (memory_regions + i,
		   sizeof (memory_regions)/sizeof (*memory_regions) - i,
		   RAM_BANK1_START, RAM_BANK1_LENGTH);
#endif

#if defined (RAM_BANK2_START)
  PUTC_LL ('2');
  i = memory_scan (memory_regions + i,
		   sizeof (memory_regions)/sizeof (*memory_regions) - i,
		   RAM_BANK2_START, RAM_BANK2_LENGTH);
#endif

#if defined (RAM_BANK3_START)
  PUTC_LL ('3');
  i = memory_scan (memory_regions + i,
		   sizeof (memory_regions)/sizeof (*memory_regions) - i,
		   RAM_BANK3_START, RAM_BANK3_LENGTH);
#endif

  PUTC_LL ('m');
}

#if !defined (CONFIG_SMALL)
static void memory_report (void)
{
  int i;

  printf ("  memory:");

  for (i = 0; i < sizeof (memory_regions)/sizeof (*memory_regions); ++i)
    if (memory_regions[i].length) {
      if (i)
	printf ("         ");
      printf (" 0x%lx 0x%08x (%d MiB)\n",
	      memory_regions[i].start, memory_regions[i].length,
	      memory_regions[i].length/(1024*1024));
    }
#if defined (CONFIG_CMD_MEMLIMIT)
  if (memlimit)
    printf ("          memlimit is %d.%03d MiB, %d (0x%x) bytes\n",
	    memlimit/(1024*1024),
	    (((memlimit/1024)*1000)/1024)%1000,
	    memlimit, memlimit);
#endif

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

  /* *** FIXME: this should optimize for the case where cb == width
     and width is non-zero.  We should force IO to be the requested
     width.  memcpy is unreliable. */

  switch (d->width) {
  case 1:
  /* *** Dumber version of the memory read function which serves to
     properly read from memories (e.g. broken CF interfaces) where the
     chip select must toggle for every access.  This is a lowest
     common denominator, and will tend to be pretty slow. */
    {
      unsigned char* pbSrc = (unsigned char*) (d->start + d->index);
      int i = cb;
      while (i--)
	*(unsigned char*) pv = *pbSrc++, ++pv;
    }
    break;

  default:
  case 2:
  case 4:
    /* memcpy performs an optimal copy, probably at machine word width */
    memcpy (pv, (void*) (d->start + d->index), cb);
    break;
  }

  d->index += cb;

  return cb;
}

static ssize_t memory_write (struct descriptor_d* d, const void* pv, size_t cb)
{
  if (d->index + cb > d->length)
    cb = d->length - d->index;

  /* Make sure when we write bytes and shorts that we write exactly
     those at the requested address. */
  switch (cb) {
  case 1:
    *(char*) (d->start + d->index) = *(char*) pv;
    break;
  case 2:
    *(unsigned short*) (d->start + d->index) = *(unsigned short*) pv;
    break;
  case 4:
    *(unsigned long*) (d->start + d->index) = *(unsigned long*) pv;
    break;
  default:
    memcpy ((void*) (d->start + d->index), pv, cb);
    break;
  }

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


#if defined (CONFIG_CMD_MEMLIMIT)

static int cmd_memlimit (int argc, const char** argv)
{
  char* pchEnd;
  unsigned long l;

  if (argc == 1) {
    printf ("memlimit is %d.%03d MiB, %d (0x%x) bytes\n",
	    memlimit/(1024*1024),
	    (((memlimit/1024)*1000)/1024)%1000,
	    memlimit, memlimit);
    return 0;
  }
  if (argc != 2)
    return ERROR_PARAM;

  l = simple_strtoul (argv[1], &pchEnd, 0);
  if (*pchEnd == 'k' || *pchEnd == 'K')
    l *= 1024;
  if (*pchEnd == 'm' || *pchEnd == 'm')
    l *= 1024*1024;

  l += 4*1024 - 1;
  l &= ~(4*1024 - 1);
  memlimit = l;

  return 0;
}

static __command struct command_d c_memlimit = {
  .command = "memlimit",
  .description = "limit memory passed to Linux",
  .func = cmd_memlimit,
  COMMAND_HELP(
"memlimit"
" [SIZE]\n"
"  Limit amount of memory passed to the Linux kernel.\n"
"  SIZE is a count of bytes and my be suffixed with 'm' or megabytes.\n"
"  Pass a SIZE of zero to clear the limit.  If there is no parameter,\n"
"  the command will display the current limit.  The limit is always rounded\n"
"  to an even number of 4KiB pages.\n\n"
"  e.g.  memlimit 8m\n"
  )
};

#endif

#if defined (CONFIG_CMD_MEMSCAN)

static int cmd_memscan (int argc, const char** argv)
{
  struct descriptor_d d;
  int result;
  int i;
  int c;
  struct mem_region regions[8];
  int update = 0;

  /* Parse arguments */
  while (argc > 1 && *argv[1] == '-') {
    switch (argv[1][1]) {
    case 'u':
      update = 1;
      break;
    default:
      return ERROR_PARAM;
      break;
    }
    --argc;
    ++argv;
  }

  if (argc != 2)
    return ERROR_PARAM;

  result = parse_descriptor (argv[1], &d);
  if (result) {
    printf ("Unable to open descriptor %s\n", argv[1]);
    return result;
  }

  if (strcmp (d.driver_name, "memory"))
    ERROR_RETURN (ERROR_UNSUPPORTED, "descriptor must refer to memory");

  memset (regions, 0, sizeof (regions));
  c = memory_scan (regions, sizeof (regions)/sizeof (*regions),
		   d.start, d.length);

  for (i = 0; i < c; ++i)
    if (regions[i].length)
      printf (" 0x%lx 0x%08x (%d MiB)\n",
	      regions[i].start, regions[i].length,
	      regions[i].length/(1024*1024));

#if defined (CONFIG_MMU)
  CLEANALL_DCACHE;		/* Actually, a secondary feature */
#endif

  if (update) {
    memset (memory_regions, sizeof (memory_regions), 0);
    memcpy (memory_regions, regions, sizeof (*memory_regions)*c);
  }

  return 0;
}

static __command struct command_d c_memscan = {
  .command = "memscan",
  .description = "scan a region of memory for aliasing",
  .func = cmd_memscan,
  COMMAND_HELP(
"memscan [-u] REGION\n"
"  Scans for valid memory.\n"
"  This command is used to rescan memory, identifying aliased regions\n"
"  in order to report the contiguous blocks.  With the -u switch,\n"
"  the command will update the memory map given to the kernel when\n"
"  it boots.\n"
"  e.g.  memscan -u 0xc0000000+512m\n"
  )
};

#endif

#if defined (CONFIG_ATAG)

static struct tag* atag_memory (struct tag* p)
{
  int i;
  unsigned long start;
  unsigned long length;
#if defined (CONFIG_CMD_MEMLIMIT)
  unsigned long limit = memlimit;
#endif

  for (i = 0; i < sizeof (memory_regions)/sizeof (*memory_regions); ++i) {
    if (!memory_regions[i].length)
      break;

    p->hdr.tag = ATAG_MEM;
    p->hdr.size = tag_size (tag_mem32);

    start = memory_regions[i].start;
    length = memory_regions[i].length;

#if defined (CONFIG_CMD_MEMLIMIT)
    if (memlimit) {
      if (!limit)
	break;
      if (length > limit)
	length = limit;
      limit -= length;
    }
#endif

    p->u.mem.start = start;
    p->u.mem.size  = length;

# if !defined (CONFIG_SMALL)
       printf ("ATAG_MEM: start 0x%08x  size 0x%08x\n",
	       p->u.mem.start, p->u.mem.size);
# endif

    p = tag_next (p);
  }

  return p;
}

static __atag_1 struct atag_d _atag_memory = { atag_memory };


#endif
