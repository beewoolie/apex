/* drv-nor.c
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

   NOR flash IO driver for LPD 7A400.  

   *** This driver, along with the other two, should be more
   *** self-discovering.  We don't want to waste code doing needless
   *** searching, but it would be helpful to use the device's internal
   *** data structures to discover how the device is layed out.

   - This driver has been hacked to only support a 32 bit NOR
     meta-device consisting of a pair of 16 bit flash chips.  It would
     be good to have a single driver that uses compile-time macros to
     control how it accesses the flash array.  We want to stay away
     from truly dynamic code in order to conserve memory.

*/

#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <apex.h>
#include <config.h>
#include "hardware.h"
#include <spinner.h>

//#define TALK
//#define NOISY

#if defined TALK
# define PRINTF(v...)	printf (v)
#else
# define PRINTF(v...)	do {} while (0)
#endif

//#define USE_BUFFERED_WRITE	/* Use write buffer for faster operation */
//#define NO_WRITE		/* Disable writes, for debugging */

/* Here are the parameters that ought to be changed to align the
   driver with the expected flash layout. */

#define NOR_0_PHYS	(0x00000000)
#define NOR_0_LENGTH	(16*1024*1024)
#define WIDTH		(32)		/* Device width in bits */
#define WIDTH_SHIFT	(WIDTH>>4)	/* Bit shift for device width */
#define CHIP_MULTIPLIER	(2)		/* Number of chips at REGA */
#define REGC		__REG16		/* Single chip I/O macro */
#define REGA		__REG		/* Array I/O macro */

#define CMD(v)		((v) | ((v)<<16))
#define STAT(v)		((v) | ((v)<<16))
#define QRY(v)		((v) | ((v)<<16))

/* *** How do we hook the VPEN manipulation code? */

/* Here are the parameters that ought to remain constant for CFI
   compliant flash. */

#define ReadArray	0xff
#define ReadID		0x90
#define ReadQuery	0x98
#define ReadStatus	0x70
#define ClearStatus	0x50
#define Erase		0x20
#define EraseConfirm	0xd0
#define Program		0x40
#define ProgramBuffered	0xe8
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

struct nor_chip {
  //  unsigned char manufacturer;
  //  unsigned char device;
  unsigned long total_size;
  int erase_size;
  int erase_count;		/* Number of erase blocks */
  int writebuffer_size;		/* Size (bytes) of buffered write buffer */
};

const static struct nor_chip* chip;
static struct nor_chip chip_probed;

static unsigned long phys_from_index (unsigned long index)
{
  return index + NOR_0_PHYS;
}

static void vpen_enable (void)
{
  __REG16 (CPLD_FLASH) |= CPLD_FLASH_FL_VPEN;
}

static void vpen_disable (void)
{
  __REG16 (CPLD_FLASH) &= ~CPLD_FLASH_FL_VPEN;
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
  } while (   (status & STAT(Ready)) != STAT(Ready)
           && timer_delta (time, timer_read ()) < 6*1000);
  return status;
}

/* nor_unlock_page

   will conditionally unlock the page if it is locked.  Conditionally
   performing this operation results in minimal wear.

   We check if either block is locked and unlock both if one is
   locked.

*/

static unsigned long nor_unlock_page (unsigned long index)
{
  index &= ~chip->erase_size;	/* Base of page */

  WRITE_ONE (index, CMD (ReadID));
  if ((REGA (index + (0x02 << WIDTH_SHIFT)) & QRY (0x1)) == 0)
    return STAT (Ready);

  WRITE_ONE (index, CMD (Configure));
  WRITE_ONE (index, CMD (LockClear));
  return nor_status (index);
}

static void nor_init (void)
{
  vpen_disable ();

  REGA (NOR_0_PHYS) = CMD (ReadArray);
  REGA (NOR_0_PHYS) = CMD (ReadID);

  if (   REGA (NOR_0_PHYS + (0x10 << WIDTH_SHIFT)) != QRY('Q')
      || REGA (NOR_0_PHYS + (0x11 << WIDTH_SHIFT)) != QRY('R')
      || REGA (NOR_0_PHYS + (0x12 << WIDTH_SHIFT)) != QRY('Y'))
    return;

  chip_probed.total_size 
    = (1<<REGC (NOR_0_PHYS + (0x27 << WIDTH_SHIFT)))
    *CHIP_MULTIPLIER;

  chip_probed.writebuffer_size
    = (1<<(   REGC (NOR_0_PHYS + (0x2a << WIDTH_SHIFT))
           | (REGC (NOR_0_PHYS + (0x2b << WIDTH_SHIFT)) << 8)))
    *CHIP_MULTIPLIER;
  chip_probed.erase_size
    = 256*(   REGC (NOR_0_PHYS + (0x2f << WIDTH_SHIFT))
	   | (REGC (NOR_0_PHYS + (0x30 << WIDTH_SHIFT)) << 8))
    *CHIP_MULTIPLIER;
  chip_probed.erase_count
    = 1 + (REGC (NOR_0_PHYS + (0x2d << WIDTH_SHIFT))
	   | (REGC (NOR_0_PHYS + (0x2e << WIDTH_SHIFT)) << 8));
  chip = &chip_probed;

#if defined (NOISY)

  PRINTF ("\r\nNOR flash ");

  if (chip) {
    PRINTF (" %ldMiB total, %dKiB erase", 
	    chip->total_size/(1024*1024), chip->erase_size/1024);
    PRINTF (", %dB write buffer, %d erase blocks", 
	    chip->writebuffer_size, chip->erase_count);
    PRINTF ("\r\n");
  }

  PRINTF ("command set 0x%x\r\n",
	  REGC (NOR_0_PHYS + (0x13 << WIDTH_SHIFT)));
  PRINTF ("extended table 0x%x\r\n",
	  REGC (NOR_0_PHYS + (0x15 << WIDTH_SHIFT)));
  PRINTF ("alternate command set 0x%x\r\n",
	  REGC (NOR_0_PHYS + (0x17 << WIDTH_SHIFT)));
  PRINTF ("alternate address 0x%x\r\n",
	  REGC (NOR_0_PHYS + (0x19 << WIDTH_SHIFT)));

  {
    int typical;
    int max;

    typical = REGC (NOR_0_PHYS + (0x1f << WIDTH_SHIFT));
    max	    = REGC (NOR_0_PHYS + (0x23 << WIDTH_SHIFT));
    PRINTF ("single word write %d us (%d us)\r\n", 
	    1<<typical, (1<<typical)*max);
    typical = REGC (NOR_0_PHYS + (0x20 << WIDTH_SHIFT));
    max	    = REGC (NOR_0_PHYS + (0x24 << WIDTH_SHIFT));
    PRINTF ("write-buffer write %d us (%d us)\r\n", 
	    1<<typical, (1<<typical)*max);
    typical = REGC (NOR_0_PHYS + (0x21 << WIDTH_SHIFT));
    max	    = REGC (NOR_0_PHYS + (0x25 << WIDTH_SHIFT));
    PRINTF ("block erase %d ms (%d ms)\r\n", 
	    1<<typical, (1<<typical)*max);
    
    typical = REGC (NOR_0_PHYS + (0x22 << WIDTH_SHIFT));
    max     = REGC (NOR_0_PHYS + (0x26 << WIDTH_SHIFT));
    if (typical) 
      PRINTF ("chip erase %d us (%d us)\r\n", 
	      1<<typical, (1<<typical)*max);
  }

  PRINTF ("device size 0x%x\r\n",
	  REGC (NOR_0_PHYS + (0x27 << WIDTH_SHIFT)));
  PRINTF ("flash interface 0x%x\r\n",
	  REGC (NOR_0_PHYS + (0x28 << WIDTH_SHIFT))
	  | (REGC (NOR_0_PHYS + (0x28 << WIDTH_SHIFT)) << 8));
  PRINTF ("write buffer size %d bytes\r\n",
	  1<<(REGC (NOR_0_PHYS + (0x2a << WIDTH_SHIFT))
	      | (REGC (NOR_0_PHYS + (0x2b << WIDTH_SHIFT)) << 8)));
  PRINTF ("erase block region count %d\r\n",
	  REGC (NOR_0_PHYS + (0x2c << WIDTH_SHIFT)));
  PRINTF ("erase block info 0x%x\r\n",
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


/* nor_write

   does a little bit of fussing in order to handle writing single
   bytes and odd addresses.  This implementation is coded for a 16 bit
   device.  It is probably OK for 32 bit devices, too, since most of
   those are really pairs of 16 bit devices.

   Most chips can perform buffered writes.  This routine only writes
   one cell at a time because the logic is simpler and adequate.

*/

static ssize_t nor_write (struct descriptor_d* d, const void* pv, size_t cb)
{
  int cbWrote = 0;
  int pageLast = -1;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  while (cb) {
    unsigned long index = d->start + d->index;
    int page = index/chip->erase_size;
    unsigned long data;
    unsigned long status;
    int step = 4;

    index = phys_from_index (index);

    if ((index & (WIDTH/8 - 1)) || cb < step) {
      step = (WIDTH/8) - (index & (WIDTH/8 - 1)); /* Max at this index */
      if (step > cb)
	step = cb;
      data = ~0;
      memcpy ((unsigned char*) &data + (index & (WIDTH/8 - 1)), pv, step);
      index &= ~(WIDTH/8 - 1);
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
    PRINTF ("nor_write 0x%08lx index 0x%lx  page 0x%x  step %d  cb 0x%x\r\n",
	    data, index, page, step, cb);
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
      printf ("Program failed at 0x%p (%lx)\r\n", (void*) index, status);
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

static void nor_erase (struct descriptor_d* d, size_t cb)
{
  if (d->index + cb > d->length)
    cb = d->length - d->index;

  SPINNER_STEP;

  while (cb > 0) {
    unsigned long index = d->start + d->index;
    unsigned long available
      = chip->erase_size - (index & (chip->erase_size - 1));
    unsigned long status; 

    index = phys_from_index (index);

    if (available > cb)
      available = cb;

    index &= ~(chip->erase_size - 1);

    vpen_enable ();
    status = nor_unlock_page (index);
    if (status & STAT (ProgramError | VPEN_Low | DeviceProtected))
      goto fail;
#if defined (NO_WRITE)
    PRINTF ("nor_erase index 0x%lx  available 0x%lx\r\n",
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
      printf ("Erase failed at 0x%p (%lx)\r\n", (void*) index, status);
      CLEAR_STATUS (index);
      return;
    }

    cb -= available;
    d->index += cb;
  }
}

static __driver_3 struct driver_d nor_driver = {
  .name = "nor-lpd7a40x",
  .description = "NOR flash driver",
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

static __service_6 struct service_d lpd7a40x_nor_service = {
  .init = nor_init,
};
