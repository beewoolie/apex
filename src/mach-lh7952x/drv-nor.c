/* drv-nor.c
     $Id$

   written by Marc Singer
   4 Nov 2004

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

   *** Look to the LH7A40X NOR flash driver for some work about
   *** generifiying the driver.  The goal is to make it general at
   *** compile time and not (much) at run time.

   NOR flash IO driver for LPD 7952x.  The is, in the sum, a CFI
   compliant driver.  Nothing has been done to make it a general
   solution since it is more important that the driver be small than
   there be much code sharing.  After all, the code isn't very large
   and it is unlikely to require revision.

   It's important that the driver leave the memory array in ReadArray
   mode so that the memory driver can be used to read from the device.

   Multiple banks means that we are somewhat shaky in some modes.  For
   example, it would be handy to be lazy about clearing status and
   setting the array in read mode.  Unfortunately, the multiple banks
   makes this difficult to detect optimally.  It might be OK to
   perform the ReadArray and ClearStatus commands always for both
   banks.

   Most of the small procedures at the top of this module exist to
   shrink the overall size of this driver.  It is difficult to know
   exactly which changes will help because of side effects and
   constant caching within the bodies of each function.

   The buffered write is much faster and it is about 260 bytes longer.
   It would probably be worth the trouble of optimizing the code size.

*/

#include <apex.h>
#include <config.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include "hardware.h"
#include <spinner.h>

//#define NOISY
//#define TALK
//#define EXTENDED

#if defined TALK
# define PRINTF(v...)	printf (v)
#else
# define PRINTF(v...)	do {} while (0)
#endif

#define USE_BUFFERED_WRITE	/* Use write buffer for faster operation */
//#define NO_WRITE		/* Disable writes, for debugging */


#define WIDTH_SHIFT	(WIDTH>>4)	/* Bit shift for device width */

#define ReadArray	(0xff)
#define ReadID		(0x90)
#define ReadQuery	(0x98)
#define ReadStatus	(0x70)
#define ClearStatus	(0x50)
#define Erase		(0x20)
#define EraseConfirm	(0xd0)
#define Program		(0x40)
#define ProgramBuffered	(0xe8)
#define ProgramConfirm	(0xd0)
#define Suspend		(0xb0)
#define Resume		(0xd0)
#define STSConfig	(0xb8)
#define Configure	(0x60)
#define LockSet		(0x01)
#define LockClear	(0xd0)
#define ProgramOTP	(0xc0)

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
  unsigned long total_size;	/* Size (bytes) of flash device */
  int erase_size;		/* Size (bytes) of each erase block */
  int erase_count;		/* Number of erase blocks */
  int writebuffer_size;		/* Size (bytes) of buffered write buffer */
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
  __REG16 (CPLD_FLASH) |=  CPLD_FLASH_FL_VPEN;
}

static void vpen_disable (void)
{
  __REG16 (CPLD_FLASH) &= ~CPLD_FLASH_FL_VPEN;
}

static unsigned short nor_read_one (unsigned long index)
{
  return __REG16 (index);
}
#define READ_ONE(i) nor_read_one (i)

static void nor_write_one (unsigned long index, unsigned short v)
{
  __REG16 (index) = v;
}
#define WRITE_ONE(i,v) nor_write_one ((i), (v))

#define CLEAR_STATUS(i) nor_write_one(i, ClearStatus)

static unsigned short nor_status (unsigned long index)
{
  unsigned short status; 
  unsigned long time = timer_read ();
  do {
    status = READ_ONE (index);
  } while (   (status & Ready) != Ready
           && timer_delta (time, timer_read ()) < 6*1000);
  return status;
}

static unsigned short nor_unlock_page (unsigned long index)
{
  WRITE_ONE (index, Configure);
  WRITE_ONE (index, LockClear);
  return nor_status (index);
}

static void nor_init (void)
{
  vpen_disable ();

  __REG16 (NOR_0_PHYS) = ReadArray;
  __REG16 (NOR_0_PHYS) = ReadID;

  if (   __REG16 (NOR_0_PHYS + (0x10 << WIDTH_SHIFT)) != 'Q'
      || __REG16 (NOR_0_PHYS + (0x11 << WIDTH_SHIFT)) != 'R'
      || __REG16 (NOR_0_PHYS + (0x12 << WIDTH_SHIFT)) != 'Y')
    return;

  chip_probed.total_size 
    = (1<<__REG16 (NOR_0_PHYS + (0x27 << WIDTH_SHIFT)))
#if defined (NOR_1_PHYS)
    /* *** FIXME: this isn't a valid hack.  The trouble is that we
       don't have a reliable way to detect multiple chips.  I could do
       it with some sort of trick with setting the status and looking
       to see if the second bank changes too.  I could look to see how
       the kernel does it. */
    *2
#endif
    ;
  chip_probed.writebuffer_size
    = 1<<(   __REG16 (NOR_0_PHYS + (0x2a << WIDTH_SHIFT))
	  | (__REG16 (NOR_0_PHYS + (0x2b << WIDTH_SHIFT)) << 8));
  chip_probed.erase_size
    = 256*(   __REG16 (NOR_0_PHYS + (0x2f << WIDTH_SHIFT))
	   | (__REG16 (NOR_0_PHYS + (0x30 << WIDTH_SHIFT)) << 8));
  chip_probed.erase_count
    = 1 + (__REG16 (NOR_0_PHYS + (0x2d << WIDTH_SHIFT))
	   | (__REG16 (NOR_0_PHYS + (0x2e << WIDTH_SHIFT)) << 8));
  chip = &chip_probed;


#if defined (NOISY)

  PRINTF ("\r\nNOR flash ");

  if (chip)
    PRINTF (" %ldMiB total, %dKiB erase\r\n", 
	    chip->total_size/(1024*1024), chip->erase_size/1024);

  PRINTF ("command set 0x%x\r\n",
	  __REG16 (NOR_0_PHYS + (0x13 << WIDTH_SHIFT)));
  PRINTF ("extended table 0x%x\r\n",
	  __REG16 (NOR_0_PHYS + (0x15 << WIDTH_SHIFT)));
  PRINTF ("alternate command set 0x%x\r\n",
	  __REG16 (NOR_0_PHYS + (0x17 << WIDTH_SHIFT)));
  PRINTF ("alternate address 0x%x\r\n",
	  __REG16 (NOR_0_PHYS + (0x19 << WIDTH_SHIFT)));

  {
    int typical;
    int max;

    typical = __REG16 (NOR_0_PHYS + (0x1f << WIDTH_SHIFT));
    max	    = __REG16 (NOR_0_PHYS + (0x23 << WIDTH_SHIFT));
    PRINTF ("single word write %d us (%d us)\r\n", 
	    1<<typical, (1<<typical)*max);
    typical = __REG16 (NOR_0_PHYS + (0x20 << WIDTH_SHIFT));
    max	    = __REG16 (NOR_0_PHYS + (0x24 << WIDTH_SHIFT));
    PRINTF ("write-buffer write %d us (%d us)\r\n", 
	    1<<typical, (1<<typical)*max);
    typical = __REG16 (NOR_0_PHYS + (0x21 << WIDTH_SHIFT));
    max	    = __REG16 (NOR_0_PHYS + (0x25 << WIDTH_SHIFT));
    PRINTF ("block erase %d ms (%d ms)\r\n", 
	    1<<typical, (1<<typical)*max);
    
    typical = __REG16 (NOR_0_PHYS + (0x22 << WIDTH_SHIFT));
    max     = __REG16 (NOR_0_PHYS + (0x26 << WIDTH_SHIFT));
    if (typical) 
      PRINTF ("chip erase %d us (%d us)\r\n", 
	      1<<typical, (1<<typical)*max);
  }

  PRINTF ("device size 0x%x\r\n",
	  __REG16 (NOR_0_PHYS + (0x27 << WIDTH_SHIFT)));
  PRINTF ("flash interface 0x%x\r\n",
	  __REG16 (NOR_0_PHYS + (0x28 << WIDTH_SHIFT))
	  | (__REG16 (NOR_0_PHYS + (0x28 << WIDTH_SHIFT)) << 8));
  PRINTF ("write buffer size %d bytes\r\n",
	  1<<(__REG16 (NOR_0_PHYS + (0x2a << WIDTH_SHIFT))
	      | (__REG16 (NOR_0_PHYS + (0x2b << WIDTH_SHIFT)) << 8)));
  PRINTF ("erase block region count %d\r\n",
	  __REG16 (NOR_0_PHYS + (0x2c << WIDTH_SHIFT)));
  PRINTF ("erase block info 0x%x\r\n",
	  __REG16 (NOR_0_PHYS + (0x2d << WIDTH_SHIFT))
	  | (__REG16 (NOR_0_PHYS + (0x2e << WIDTH_SHIFT)) << 8)
	  | (__REG16 (NOR_0_PHYS + (0x2f << WIDTH_SHIFT)) << 16)
	  | (__REG16 (NOR_0_PHYS + (0x30 << WIDTH_SHIFT)) << 24));
#endif

#if defined (EXTENDED)

 {
   int ext = __REG16 (NOR_0_PHYS + (0x15 << WIDTH_SHIFT));
   printf ("extended 0x%x\r\n", ext);

   if (   __REG16 (NOR_0_PHYS + (ext << WIDTH_SHIFT)) == 'P'
       && __REG16 (NOR_0_PHYS + ((ext + 1) << WIDTH_SHIFT)) == 'R'
       && __REG16 (NOR_0_PHYS + ((ext + 2) << WIDTH_SHIFT)) == 'I') {
    
     printf ("features 0x%x\r\n",
	     __REG16 (NOR_0_PHYS + ((ext + 5) << WIDTH_SHIFT))
	     | (__REG16 (NOR_0_PHYS + ((ext + 6) << WIDTH_SHIFT)) << 8));
   }
 }

#endif


  __REG16 (NOR_0_PHYS) = ClearStatus;
#if defined (NOR_1_PHYS)
  __REG16 (NOR_1_PHYS) = ClearStatus;
#endif
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

    PRINTF ("nor read: 0x%p 0x%08lx %d\r\n", pv, index, available);
    WRITE_ONE (index, ReadArray);
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
  int pageLast = -1;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  while (cb > 0) {
    unsigned long index = d->start + d->index;
    int page = d->index/chip->erase_size;
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
    printf ("  available %d  cb %d\r\n", available, cb);
    printf ("0x%lx <= 0x%x\r\n", index & ~1, ProgramBuffered);
#else
    WRITE_ONE (index & ~1, ProgramBuffered);
#endif
    status = nor_status (index & ~1);
    if (!(status & Ready)) {
      PRINTF ("nor_write failed program start 0x%lx (0x%x)\r\n", 
	      index & ~1, status);
      goto fail;
    }
    
    {
      int av = available + (index & 1);
#if defined (NO_WRITE)
      printf ("0x%lx <= 0x%02x\r\n", index & ~1, av - av/2 - 1);
#else
      WRITE_ONE (index & ~1, av - av/2 - 1);
#endif
    }

    if (available == chip->writebuffer_size && ((unsigned long) pv & 1) == 0) {
      int i;
      for (i = 0; i < available; i += 2)
#if defined (NO_WRITE)
	printf ("0x%lx := 0x%04x\r\n", index + i, ((unsigned short*)pv)[i/2]);
#else
	WRITE_ONE (index + i, ((unsigned short*)pv)[i/2]);
#endif
    }
    else {
      int i;
      char rgb[chip->writebuffer_size];
      rgb[0] = 0xff;						/* First */
      rgb[((available + (index & 1) + 1)&~1) - 1] = 0xff;	/* Last */
//      printf ("  last %ld\r\n", ((available + (index & 1) + 1)&~1) - 1);
      memcpy (rgb + (index & 1), pv, available);
      for (i = 0; i < available + (index & 1); i += 2)
#if defined (NO_WRITE)
	printf ("0x%lx #= 0x%04x\r\n", 
		(index & ~1) + i, ((unsigned short*)rgb)[i/2]);
#else
	WRITE_ONE ((index & ~1) + i, ((unsigned short*)rgb)[i/2]);
#endif
    }

#if defined (NO_WRITE)
    printf ("0x%lx <= 0x%x\r\n", index & ~1, ProgramConfirm);
#else
    WRITE_ONE (index & ~1, ProgramConfirm);
#endif
    SPINNER_STEP;
    status = nor_status (index);

    vpen_disable ();

    SPINNER_STEP;

    if (status & (ProgramError | VPEN_Low | DeviceProtected)) {
    fail:
      printf ("Program failed at 0x%p (%x)\r\n", (void*) index, status);
      CLEAR_STATUS (index);
      return cbWrote;
    }

    d->index += available;
    pv += available;
    cb -= available;
    cbWrote += available;
    //    printf ("  cb %x  cbWrote 0x%x  pv %p\r\n", cb, cbWrote, pv);
  }

  return cbWrote;
}

#else

/* nor_write

   does a little bit of fussing in order to handle writing single
   bytes and odd addresses.  This implementation is coded for a 16 bit
   device.  It is probably OK for 32 bit devices, too, since most of
   those are really pairs of 16 bit devices.

   The LH28F128 can perform buffered writes.  This routine only writes
   individual bytes because the logic is simpler and adequate.

   We ought to clear status as the top of the routine.  Unfortunately,
   with multiple banks, we would have to either detect bank switches,
   or always clear status on both banks.

*/

static ssize_t nor_write (struct descriptor_d* d, const void* pv, size_t cb)
{
  size_t cbWrote = 0;
  int pageLast = -1;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  while (cb) {
    unsigned long index = d->start + d->index;
    int page = d->index/chip->erase_size;
    unsigned short data;
    unsigned short status;
    int step = 2;

    index = phys_from_index (index);

    if (index & 1) {
      step = 1;
      index &= ~1;
      data = ~0;
      ((unsigned char*)&data)[1] = *(const unsigned char*)pv;
    }
    else if (cb == 1) {
      step = 1;
      data = ~0;
      ((unsigned char*)&data)[0] = *(const unsigned char*)pv;
    }
    else
      memcpy (&data, pv, 2);

//    printf ("nor_write index 0x%lx  page 0x%x  step %d  cb 0x%x\r\n",
//	    index, page, step, cb);

    vpen_enable ();
    if (page != pageLast) {
      status = nor_unlock_page (index);
      if (status & (ProgramError | VPEN_Low | DeviceProtected))
	goto fail;
      pageLast = page;
    }
#if defined (NO_WRITE)
    status = Ready;
#else
    WRITE_ONE (index, Program);
    WRITE_ONE (index, data);
    status = nor_status (index);
#endif
    vpen_disable ();

    SPINNER_STEP;

    if (status & (ProgramError | VPEN_Low | DeviceProtected)) {
    fail:
      printf ("Program failed at 0x%p (%x)\r\n", (void*) index, status);
      CLEAR_STATUS (index);
      return cbWrote;
    }

    d->index += step;
    pv += step;
    cb -= step;
    cbWrote += step;
//    printf ("  cb %x  cbWrote 0x%x\r\n", cb, cbWrote);
  }

  return cbWrote;
}

#endif

static void nor_erase (struct descriptor_d* d, size_t cb)
{
  //  int pageLast = -1;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  SPINNER_STEP;

  while (cb > 0) {
    unsigned long index = d->start + d->index;
    //    int page = d->index/chip->erase_size;
    unsigned long available
      = chip->erase_size - (index & (chip->erase_size - 1));
    unsigned short status; 

    //    printf ("nor_erase preindex 0x%lx\r\n", index);
    index = phys_from_index (index);

    if (available > cb)
      available = cb;

    index &= ~(chip->erase_size - 1);

    PRINTF ("nor_erase index 0x%lx  available 0x%lx\r\n",
	    index, available);

    vpen_enable ();
    //    if (page != pageLast) {
      status = nor_unlock_page (index);
      if (status & (ProgramError | VPEN_Low | DeviceProtected)) {
	PRINTF ("nor_erase failed at 0x%lx (0x%x)\r\n", index, status);
	goto fail;
      }
      //      pageLast = page;
      //    }
#if defined (NO_WRITE)
    status = Ready;
#else
    WRITE_ONE (index, Erase);
    WRITE_ONE (index, EraseConfirm);
    status = nor_status (index);
#endif
    vpen_disable ();

    SPINNER_STEP;

    if (status & (EraseError | VPEN_Low | DeviceProtected)) {
    fail:
      printf ("Erase failed at 0x%p (%x)\r\n", (void*) index, status);
      CLEAR_STATUS (index);
      return;
    }

    cb -= available;
    d->index += available;
  }
}

static __driver_3 struct driver_d nor_driver = {
  .name = "nor-lpd7952x",
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

static __service_6 struct service_d lpd7952x_nor_service = {
  .init = nor_init,
};
