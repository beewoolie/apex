/* drv-nor-cfi.c
     $Id$

   written by Marc Singer
   13 Nov 2004

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

   Generic NOR flash IO driver

   o This driver has been hacked to only support a 32 bit NOR
     meta-device consisting of a pair of 16 bit flash chips.  It would
     be good to have a single driver that uses compile-time macros to
     control how it accesses the flash array.  We want to stay away
     from truly dynamic code in order to conserve memory.

     *** buffered write code has not been fixed to handle the 32 bit
     *** array width.

   o Much work has gone into making this driver generic.  It is not
     intended to be able to support multiple organizations at one
     time.  In other words, the generic elements of the driver are
     fixed at compile time.

   o Banking support.  We take a few liberties with the banking
     support in that we assume some things.  Full generality comes at
     a cost and, at this point, I am not willing to pay the full cost.
     The driver can handle two chips, but not two completely different
     chips.  Esentially, it can handle a pair of identical chips or
     one chip with two banks.  It linearizes multiple banks for the
     convenience of writing large, contiguous blocks of data.
     Improved flexbility would come from always translating from the
     region structure.  What probably needs to happen is the region
     structure needs to include the physical offset of the region.
     Handling disparate chips, however, will probably never be done as
     the chips may require different control constants.  Phooey.

*/

#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <apex.h>
#include <config.h>
#include <spinner.h>
#include <asm/reg.h>

#include <mach/nor-cfi.h>

//#define TALK
//#define NOISY

#if defined TALK
# define PRINTF(v...)	printf (v)
#else
# define PRINTF(v...)	do {} while (0)
#endif

#define ENTRY(l) PRINTF ("%s\n", __FUNCTION__)

//#define NO_WRITE		/* Disable writes, for debugging */

#if NOR_WIDTH == 32
# define REGA		__REG		/* Array I/O macro */
# if NOR_CHIP_MULTIPLIER == 1
#  define REGC	        __REG
# else
#  define REGC	        __REG16
# endif
#elif NOR_WIDTH == 16
# define REGA		__REG16		/* Array I/O macro */
# define REGC		__REG16		/* Single chip I/O macro */
#endif

#if NOR_CHIP_MULTIPLIER == 2
# define CMD(v)		((v) | ((v)<<16))
# define STAT(v)	((v) | ((v)<<16))
# define QRY(v)		((v) | ((v)<<16))
#else
# define CMD(v)		(v)
# define STAT(v)	(v)
# define QRY(v)		(v)
#endif

#if NOR_WIDTH == 16
/* *** This has only be tested to work with a single chip and a bus
   width of 16. */
# if ! defined (CONFIG_SMALL)
#  define USE_BUFFERED_WRITE	/* Use write buffer for faster operation */
# endif
#endif

#define WIDTH_SHIFT	(NOR_WIDTH>>4)	/* Bit shift for device width */

/* CFI compliant parameters */

#define ReadArray	0xff
#define ReadID		0x90
#define ReadQuery	0x98
#define ReadStatus	0x70
#define ClearStatus	0x50
#define Erase		0x20
#define EraseConfirm	0xd0
#define Program		0x40
#define ProgramBuffered	0xe8
#define ProgramConfirm	0xd0
#define Suspend		0xb0
#define Resume		0xd0
#define STSConfig	0xb8
#define Configure	0x60
#define LockSet		0x01
#define LockClear	0xd0
#define ProgramOTP	0xc0

#define Ready		(1<<7)
#define EraseSuspended	(1<<6)
#define EraseError	(1<<5)
#define ProgramError	(1<<4)
#define VPEN_Low	(1<<3)
#define ProgramSuspended (1<<2)
#define DeviceProtected	(1<<1)

struct nor_region {
  int size;
  int count;
  unsigned long start;		/* Relative offsets from 0 of the start of */
  unsigned long next;		/*   this region and the start of the next */
};

#define C_REGIONS_MAX	16  /* Reasonable storage for erase region info */

struct nor_chip {
  unsigned long total_size;
  int writebuffer_size;		/* Size (bytes) of buffered write buffer */
  int regions;
  struct nor_region region[C_REGIONS_MAX];
};

const static struct nor_chip* chip;
static struct nor_chip chip_probed;

static unsigned long phys_from_index (unsigned long index)
{
#if defined (NOR_1_PHYS)
  return index 
    + ((index < NOR_0_LENGTH) ? NOR_0_PHYS : (NOR_1_PHYS - NOR_0_LENGTH));
#else
  return index + NOR_0_PHYS;
#endif
}

static void vpen_enable (void)
{
#if defined (VPEN_ENABLE)
  VPEN_ENABLE;
#endif
}

static void vpen_disable (void)
{
#if defined (VPEN_DISABLE)
  VPEN_DISABLE;
#endif
}

static unsigned long nor_read_one (unsigned long index)
{
  return REGA (index);
}
#define READ_ONE(i) nor_read_one (i)

static void nor_write_one (unsigned long index, unsigned long v)
{
  REGA (index) = v;
}
#define WRITE_ONE(i,v) nor_write_one ((i), (v))

#define CLEAR_STATUS(i) nor_write_one(i, CMD (ClearStatus))

static unsigned long nor_status (unsigned long index)
{
  unsigned long status; 
  unsigned long time = timer_read ();
  do {
    status = READ_ONE (index);
  } while (   (status & STAT (Ready)) != STAT (Ready)
           && timer_delta (time, timer_read ()) < 6*1000);
  return status;
}


/* nor_region

   returns a pointer to the erase region structure for the given index.

*/

static const struct nor_region* nor_region (unsigned long index)
{
  int i;
  for (i = 0; i < chip->regions; ++i)
    if (index < chip->region[i].next)
      return &chip->region[i];
  return &chip->region[0];		/* A very serious error */
}


/* nor_unlock_page

   will conditionally unlock the page if it is locked.  Conditionally
   performing this operation results in minimal wear.

   We check if either block is locked and unlock both if one is
   locked.

*/

static unsigned long nor_unlock_page (unsigned long index)
{
  index &= ~ (nor_region (index)->size - 1);

  WRITE_ONE (index, CMD (ReadID));
  PRINTF ("nor_unlock_page 0x%lx %lx\n", index, 
	  REGA (index + (0x02 << WIDTH_SHIFT)));
  if ((REGA (index + (0x02 << WIDTH_SHIFT)) & QRY (0x1)) == 0)
    return STAT (Ready);

  WRITE_ONE (index, CMD (Configure));
  WRITE_ONE (index, CMD (LockClear));
  return nor_status (index);
}

static void nor_init_chip (unsigned long phys)
{
  int iRegionFirst = chip_probed.regions;
  int i;
  unsigned long start;

  PRINTF ("%s: probing %lx\n", __FUNCTION__, phys);

  REGA (phys) = CMD (ReadArray);
  REGA (phys) = CMD (ReadQuery);

  if (   REGA (phys + (0x10 << WIDTH_SHIFT)) != QRY('Q')
      || REGA (phys + (0x11 << WIDTH_SHIFT)) != QRY('R')
      || REGA (phys + (0x12 << WIDTH_SHIFT)) != QRY('Y')) {
#if defined (TALK)
    PRINTF ("%s: failed\n", __FUNCTION__);
    dump ((void*) phys, 256, 0);
#endif
    return;
  }

#if defined (TALK)
  PRINTF ("%s: success\n", __FUNCTION__);
  dump ((void*) phys, 256, 0);
#endif

  start = chip_probed.total_size;
  i = chip_probed.regions;

  chip_probed.regions += REGC (phys + (0x2c << WIDTH_SHIFT));
  chip_probed.total_size
    += (1<<REGC (phys + (0x27 << WIDTH_SHIFT)))*NOR_CHIP_MULTIPLIER;

  chip_probed.writebuffer_size
    = (1<<(   REGC (phys + (0x2a << WIDTH_SHIFT))
           | (REGC (phys + (0x2b << WIDTH_SHIFT)) << 8)))
    *NOR_CHIP_MULTIPLIER;

	/* Discover erase regions.  Unfortunately, this has to be done
	   so that the erase and unlock IO is reasonably efficient. */

  for (; i < chip_probed.regions; ++i) {
    int offset = (i - iRegionFirst)*4;
    PRINTF ("  ");
    chip_probed.region[i].size 
      = 256*(   REGC (phys + ((0x2f + offset) << WIDTH_SHIFT))
	     | (REGC (phys + ((0x30 + offset) << WIDTH_SHIFT)) << 8))
      *NOR_CHIP_MULTIPLIER;
    chip_probed.region[i].count
      = 1 + (   REGC (phys + ((0x2d + offset) << WIDTH_SHIFT))
	     | (REGC (phys + ((0x2e + offset) << WIDTH_SHIFT)) << 8));
    PRINTF ("  region %d %d %d\n", i,
	    chip_probed.region[i].size, chip_probed.region[i].count);
    chip_probed.region[i].start = start;
    start += chip_probed.region[i].size*chip_probed.region[i].count;
    chip_probed.region[i].next = start;
  }
}

static void nor_init (void)
{
  ENTRY (0);

  vpen_disable ();

  chip_probed.total_size = 0;	/* Should be redundant */
  chip_probed.regions = 0;	/* Should be redundant */

  nor_init_chip (NOR_0_PHYS);
#if defined (NOR_1_PHYS)
  nor_init_chip (NOR_1_PHYS);
#endif

  if (!chip_probed.total_size)
    return;

  chip = &chip_probed;

#if 0
  printf ("\nNOR flash ");

  if (chip) {
    printf (" %ldMiB total", 
	    chip->total_size/(1024*1024));
    printf (", %dB write buffer", 
	    chip->writebuffer_size);
    printf ("\n");
  }
#endif

#if defined (NOISY)

  PRINTF ("\nNOR flash ");

  if (chip) {
    PRINTF (" %ldMiB total", 
	    chip->total_size/(1024*1024));
    PRINTF (", %dB write buffer", 
	    chip->writebuffer_size);
    PRINTF ("\n");
  }

  PRINTF ("command set 0x%x\n",
	  REGC (NOR_0_PHYS + (0x13 << WIDTH_SHIFT)));
  PRINTF ("extended table 0x%x\n",
	  REGC (NOR_0_PHYS + (0x15 << WIDTH_SHIFT)));
  PRINTF ("alternate command set 0x%x\n",
	  REGC (NOR_0_PHYS + (0x17 << WIDTH_SHIFT)));
  PRINTF ("alternate address 0x%x\n",
	  REGC (NOR_0_PHYS + (0x19 << WIDTH_SHIFT)));

  {
    int typical;
    int max;

    typical = REGC (NOR_0_PHYS + (0x1f << WIDTH_SHIFT));
    max	    = REGC (NOR_0_PHYS + (0x23 << WIDTH_SHIFT));
    PRINTF ("single word write %d us (%d us)\n", 
	    1<<typical, (1<<typical)*max);
    typical = REGC (NOR_0_PHYS + (0x20 << WIDTH_SHIFT));
    max	    = REGC (NOR_0_PHYS + (0x24 << WIDTH_SHIFT));
    PRINTF ("write-buffer write %d us (%d us)\n", 
	    1<<typical, (1<<typical)*max);
    typical = REGC (NOR_0_PHYS + (0x21 << WIDTH_SHIFT));
    max	    = REGC (NOR_0_PHYS + (0x25 << WIDTH_SHIFT));
    PRINTF ("block erase %d ms (%d ms)\n", 
	    1<<typical, (1<<typical)*max);
    
    typical = REGC (NOR_0_PHYS + (0x22 << WIDTH_SHIFT));
    max     = REGC (NOR_0_PHYS + (0x26 << WIDTH_SHIFT));
    if (typical) 
      PRINTF ("chip erase %d us (%d us)\n", 
	      1<<typical, (1<<typical)*max);
  }

  PRINTF ("device size 0x%x\n",
	  REGC (NOR_0_PHYS + (0x27 << WIDTH_SHIFT)));
  PRINTF ("flash interface 0x%x\n",
	  REGC (NOR_0_PHYS + (0x28 << WIDTH_SHIFT))
	  | (REGC (NOR_0_PHYS + (0x28 << WIDTH_SHIFT)) << 8));
  PRINTF ("write buffer size %d bytes\n",
	  1<<(REGC (NOR_0_PHYS + (0x2a << WIDTH_SHIFT))
	      | (REGC (NOR_0_PHYS + (0x2b << WIDTH_SHIFT)) << 8)));
  PRINTF ("erase block region count %d\n",
	  REGC (NOR_0_PHYS + (0x2c << WIDTH_SHIFT)));
  PRINTF ("erase block info 0x%x\n",
	  REGC (NOR_0_PHYS + (0x2d << WIDTH_SHIFT))
	  | (REGC (NOR_0_PHYS + (0x2e << WIDTH_SHIFT)) << 8)
	  | (REGC (NOR_0_PHYS + (0x2f << WIDTH_SHIFT)) << 16)
	  | (REGC (NOR_0_PHYS + (0x30 << WIDTH_SHIFT)) << 24));
#endif

  REGA (NOR_0_PHYS) = CMD (ClearStatus);
}

static int nor_open (struct descriptor_d* d)
{
  if (!chip)
    return -1;

  /* perform bounds check */

  return 0;
}

static ssize_t nor_read (struct descriptor_d* d, void* pv, size_t cb)
{
  ssize_t cbRead = 0;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  while (cb) {
    unsigned long index = d->start + d->index;
    int available = cb;
    if (index < NOR_0_LENGTH && index + available > NOR_0_LENGTH)
      available = NOR_0_LENGTH - index;
    index = phys_from_index (index);

    d->index += available;
    cb -= available;
    cbRead += available;

    //    printf ("nor: 0x%p 0x%08lx %d\n", pv, index, available);
    WRITE_ONE (index, CMD (ReadArray));
    memcpy (pv, (void*) index, available);

    pv += available;
  }    
  
  return cbRead;
}


#if defined (USE_BUFFERED_WRITE)

/* nor_write

   performs a buffered write to the flash device.  The unbuffered
   write, below, is adequate but this version is much faster.  Like
   the single short write method below, we fuss a bit with the
   alignment so that we emit the data efficiently.  Moreover, we make
   an attempt to align the write buffer to a write buffer aligned
   boundary.

*/

static ssize_t nor_write (struct descriptor_d* d, const void* pv, size_t cb)
{
  size_t cbWrote = 0;
  unsigned long pageLast = -1;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  while (cb > 0) {
    unsigned long index = d->start + d->index;
    unsigned long page = index & ~ (nor_region (index)->size - 1);
    unsigned short status;
    int available
      = chip->writebuffer_size - (index & (chip->writebuffer_size - 1));

    if (available > cb)
      available = cb;

    index = phys_from_index (index);

    PRINTF ("nor write: 0x%p 0x%08lx %d\n", pv, index, available);

    vpen_enable ();

    if (page != pageLast) {
      status = nor_unlock_page (index);
      if (status & (ProgramError | VPEN_Low | DeviceProtected))
	goto fail;
      pageLast = page;
    }

#if defined (NO_WRITE)
    printf ("  available %d  cb %d\n", available, cb);
    printf ("0x%lx <= 0x%x\n", index & ~(NOR_WIDTH/8 - 1), ProgramBuffered);
#else
    WRITE_ONE (index & ~(NOR_WIDTH/8 - 1), ProgramBuffered);
#endif
    status = nor_status (index & ~(NOR_WIDTH/8 - 1));
    if (!(status & Ready)) {
      PRINTF ("nor_write failed program start 0x%lx (0x%x)\n", 
	      index & ~(NOR_WIDTH/8 - 1), status);
      goto fail;
    }
    
    {
      int av = available + (index & (NOR_WIDTH/8 - 1));
#if defined (NO_WRITE)
      printf ("0x%lx <= 0x%02x\n", index & ~(NOR_WIDTH/8 - 1),
	      /* *** FIXME for 32 bit  */
	      av - av/2 - 1);
#else
      WRITE_ONE (index & ~(NOR_WIDTH/8 - 1), 
	      /* *** FIXME for 32 bit  */
		 av - av/2 - 1);
#endif
    }

    if (available == chip->writebuffer_size && ((unsigned long) pv & 1) == 0) {
      int i;
      for (i = 0; i < available; i += 2)
#if defined (NO_WRITE)
	printf ("0x%lx := 0x%04x\n", index + i, ((unsigned short*)pv)[i/2]);
#else
	WRITE_ONE (index + i, ((unsigned short*)pv)[i/2]);
#endif
    }
    else {
      int i;
      char rgb[chip->writebuffer_size];
      rgb[0] = 0xff;						/* First */
      rgb[((available + (index & 1) + 1)&~1) - 1] = 0xff;	/* Last */
//      printf ("  last %ld\n", ((available + (index & 1) + 1)&~1) - 1);
      memcpy (rgb + (index & (NOR_WIDTH/8 - 1)), pv, available);
      for (i = 0; i < available + (index & 1); i += 2)
#if defined (NO_WRITE)
	printf ("0x%lx #= 0x%04x\n", 
		(index & ~(NOR_WIDTH/8 - 1)) + i, ((unsigned short*)rgb)[i/2]);
#else
	WRITE_ONE ((index & ~(NOR_WIDTH/8 - 1)) + i,
		   ((unsigned short*)rgb)[i/2]);
#endif
    }

#if defined (NO_WRITE)
    printf ("0x%lx <= 0x%x\n", index & ~(NOR_WIDTH/8 - 1), ProgramConfirm);
#else
    WRITE_ONE (index & ~(NOR_WIDTH/8 - 1), ProgramConfirm);
#endif
    SPINNER_STEP;
    status = nor_status (index);

    vpen_disable ();

    SPINNER_STEP;

    if (status & (ProgramError | VPEN_Low | DeviceProtected)) {
    fail:
      printf ("Program failed at 0x%p (%x)\n", (void*) index, status);
      CLEAR_STATUS (index);
      return cbWrote;
    }

    d->index += available;
    pv += available;
    cb -= available;
    cbWrote += available;
    //    printf ("  cb %x  cbWrote 0x%x  pv %p\n", cb, cbWrote, pv);
  }

  return cbWrote;
}

#else

/* nor_write

   does a little bit of fussing in order to handle writing single
   bytes and odd addresses.  This implementation is coded for a 16 bit
   device.  It is probably OK for 32 bit devices, too, since most of
   those are really pairs of 16 bit devices.

   Most chips can perform buffered writes.  This routine only writes
   one cell at a time because the logic is simpler and adequate.

   For saving on code size, this version is a win.

*/

static ssize_t nor_write (struct descriptor_d* d, const void* pv, size_t cb)
{
  int cbWrote = 0;
  unsigned long pageLast = -1;

  while (cb) {
    unsigned long index = d->start + d->index;
    unsigned long page = index & ~ (nor_region (index)->size - 1);
    unsigned long data = 0;
    unsigned long status;
    int step = NOR_WIDTH/8;

    index = phys_from_index (index);

    if ((index & (NOR_WIDTH/8 - 1)) || cb < step) {
      step = (NOR_WIDTH/8) - (index & (NOR_WIDTH/8 - 1)); /* Max at index */
      if (step > cb)
	step = cb;
      data = ~0;
      memcpy ((unsigned char*) &data + (index & (NOR_WIDTH/8 - 1)), pv, step);
      index &= ~(NOR_WIDTH/8 - 1);
    }
    else
      memcpy (&data, pv, step);

    vpen_enable ();
    if (page != pageLast) {
      status = nor_unlock_page (index);
      if (status & STAT (ProgramError | VPEN_Low | DeviceProtected))
	goto fail;
      pageLast = page;
    }
#if defined (NO_WRITE)
    printf ("nor_write 0x%0*lx index 0x%lx  page 0x%lx  step %d  cb 0x%x\n",
	    NOR_WIDTH/4, data, index, page, step, cb);
    status = STAT (Ready);
#else
    WRITE_ONE (index, CMD (Program));
    WRITE_ONE (index, data);
    status = nor_status (index);
#endif
    vpen_disable ();

    SPINNER_STEP;

    if (status & STAT (ProgramError | VPEN_Low | DeviceProtected)) {
    fail:
      printf ("Program failed at 0x%p (%lx)\n", (void*) index, status);
      CLEAR_STATUS (index);
      return cbWrote;
    }

    d->index += step;
    pv += step;
    cb -= step;
    cbWrote += step;
  }

  return cbWrote;
}

#endif

static void nor_erase (struct descriptor_d* d, size_t cb)
{
  if (d->index + cb > d->length)
    cb = d->length - d->index;

  SPINNER_STEP;

  while (cb > 0) {
    unsigned long index = d->start + d->index;
    const struct nor_region* region = nor_region (index);
    unsigned long available = region->size - (index & (region->size - 1));
    unsigned long status; 

    index = phys_from_index (index);

    if (available > cb)
      available = cb;

    index &= ~(region->size - 1);

    vpen_enable ();
    status = nor_unlock_page (index);
#if defined (NO_WRITE)
    printf ("  unlock status %lx\n", status);
#endif
    if (status & STAT (ProgramError | VPEN_Low | DeviceProtected))
      goto fail;
#if defined (NO_WRITE)
    WRITE_ONE (index, CMD (ReadArray));
    printf ("fake erasing 0x%04x\n", REGA(index));
    printf ("nor_erase index 0x%lx  available 0x%lx\n",
	    index, available);
    status = STAT (Ready);
#else
    WRITE_ONE (index, CMD (Erase));
    WRITE_ONE (index, CMD (EraseConfirm));
    status = nor_status (index);
#endif
    vpen_disable ();

    SPINNER_STEP;

    if (status & STAT (EraseError | VPEN_Low | DeviceProtected)) {
    fail:
      printf ("Erase failed at 0x%p (%lx)\n", (void*) index, status);
      CLEAR_STATUS (index);
      return;
    }

    cb -= available;
    d->index += available;
  }
}


/* nor_release

   has the sole mission to put the nor flash into read-array mode.
   This is necessary to guarantee that the reset command will be able
   to execute from flash.  Without this, it it possible that the reset
   command will pass control to a NOR flash array where the data read
   is the array status.

*/

static void nor_release (void)
{
  REGA (NOR_0_PHYS) = CMD (ReadArray);
#if defined (NOR_1_PHYS)
  REGA (NOR_1_PHYS) = CMD (ReadArray);
#endif
}


#if !defined (CONFIG_SMALL)

static void nor_report (void)
{
  int i;

  if (!chip)
    return;

  printf ("  nor:    %ldMiB total  %dB write buffer\n",
	  chip->total_size/(1024*1024), chip->writebuffer_size);
  for (i = 0; i < chip->regions; ++i)
    printf ("          region %d: %3d block%c of %6d (0x%05x) bytes\n",
	    i,
	    chip->region[i].count, 
	    (chip->region[i].count > 1) ? 's' : ' ',
	    chip->region[i].size, chip->region[i].size);
}

#endif

static __driver_3 struct driver_d nor_driver = {
  .name = "nor-cfi",
  .description = "CFI NOR flash driver",
#if defined (USE_BUFFERED_WRITE)
  .flags = DRIVER_WRITEPROGRESS(5),
#else
  .flags = DRIVER_WRITEPROGRESS(2),
#endif
  .open = nor_open,
  .close = close_helper,
  .read = nor_read,
  .write = nor_write,
  .erase = nor_erase,
  .seek = seek_helper,
};

static __service_6 struct service_d cfi_nor_service = {
  .init = nor_init,
  .release = nor_release,
#if !defined (CONFIG_SMALL)
  .report = nor_report,
#endif
};
