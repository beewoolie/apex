/* drv-nor.c
     $Id$

   written by Marc Singer
   4 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

   NOR flash block IO driver for LPD 79524.  The is, in the sum, a CFI
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

*/

#include <driver.h>
#include <linux/string.h>
#include <apex.h>
#include <config.h>
#include "lh79524.h"

//#define TALK

#define NOR_0_PHYS	(0x44000000)
#define NOR_0_LENGTH	(8*1024*1024)
#define NOR_1_PHYS	(0x45000000)
#define NOR_1_LENGTH	(8*1024*1024)
#define WIDTH		(16)	/* Device width in bits */
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
#define Suspend		(0xb0)
#define Resume		(0xd0)
#define STSConfig	(0xb8)
#define Configure	(0x60)
#define LockSet		(0x01)
#define LockClear	(0xd0)
#define ProgramOTP	(0xc0)

#define CPLD_REG_FLASH	(0x4c800000)
#define RDYnBSY		(1<<2)
#define FL_VPEN		(1<<0)

#define Ready		(1<<7)
#define EraseSuspended	(1<<6)
#define EraseError	(1<<5)
#define ProgramError	(1<<4)
#define VPEN_Low	(1<<3)
#define ProgramSuspended (1<<2)
#define DeviceProtected	(1<<1)

struct nor_chip {
  unsigned char manufacturer;
  unsigned char device;
  unsigned long total_size;
  int erase_size;
};

const static struct nor_chip chips[] = {
  { 0xb0, 0x18, 16*1024*1024, 128*1024 }, /* LH28F128SPHTD */
};

const static struct nor_chip* chip;

static unsigned long phys_from_index (unsigned long index)
{
  return index 
    + ((index < NOR_0_LENGTH) ? NOR_0_PHYS : (NOR_1_PHYS - NOR_0_LENGTH));
}

static void vpen_enable (void)
{
  __REG16 (CPLD_REG_FLASH) |= FL_VPEN;
}

static void vpen_disable (void)
{
  __REG16 (CPLD_REG_FLASH) &= ~FL_VPEN;
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
  } while (   (status & Ready) == 0
           && timer_delta (time, timer_read ()) < 6*1000);
  return status;
}

static unsigned short nor_unlock_page (unsigned long index)
{
  WRITE_ONE (index, Configure);
  WRITE_ONE (index, LockClear);
  return nor_status (index);
}

static int nor_probe (void)
{
  unsigned char manufacturer;
  unsigned char device;

  vpen_disable ();

  __REG16 (NOR_0_PHYS) = ReadArray;
  __REG16 (NOR_0_PHYS) = ReadID;

  if (   __REG16 (NOR_0_PHYS + (0x10 << WIDTH_SHIFT)) != 'Q'
      || __REG16 (NOR_0_PHYS + (0x11 << WIDTH_SHIFT)) != 'R'
      || __REG16 (NOR_0_PHYS + (0x12 << WIDTH_SHIFT)) != 'Y')
    return 0;

  manufacturer = __REG16 (NOR_0_PHYS + (0x00 << WIDTH_SHIFT));
  device       = __REG16 (NOR_0_PHYS + (0x01 << WIDTH_SHIFT));

  for (chip = &chips[0]; 
       chip < chips + sizeof (chips)/sizeof (struct nor_chip);
       ++chip)
    if (chip->manufacturer == manufacturer && chip->device == device)
      break;
  if (chip >= chips + sizeof (chips)/sizeof (chips[0]))
      chip = NULL;

#if defined (TALK)

  printf ("\r\nNOR flash ");

  if (chip)
    printf (" %ldMiB total, %dKiB erase\r\n", 
	    chip->total_size/(1024*1024), chip->erase_size/1024);
  else
    printf (" unknown 0x%x/0x%x\r\n", manufacturer, device);

#endif

  __REG16 (NOR_0_PHYS) = ClearStatus;
  __REG16 (NOR_1_PHYS) = ClearStatus;

  return chip == NULL;		/* Present and initialized */
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
    WRITE_ONE (index, ReadArray);
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

   The LH28F128 can perform buffered writes.  This routine only writes
   individual bytes because the logic is simpler and adequate.

   We ought to clear status as the top of the routine.  Unfortunately,
   with multiple banks, we would have to either detect bank switches,
   or always clear status on both banks.

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
    unsigned short data;
    unsigned short status;
    int step = 2;

    index = phys_from_index (index);

    if (index & 1) {
      step = 1;
      index &= ~1;
      WRITE_ONE (d->index, ReadArray);
      data = READ_ONE (index);
      ((unsigned char*)&data)[1] = *(const unsigned char*)pv;
    }
    else if (cb == 1) {
      step = 1;
      WRITE_ONE (d->index, ReadArray);
      data = READ_ONE (index);
      ((unsigned char*)&data)[0] = *(const unsigned char*)pv;
    }
    else
      data = *(const unsigned short*) pv;

    vpen_enable ();
    if (page != pageLast) {
      status = nor_unlock_page (index);
      if (status & (ProgramError | VPEN_Low | DeviceProtected))
	goto fail;
      pageLast = page;
    }
    WRITE_ONE (index, Program);
    WRITE_ONE (index, data);
    status = nor_status (index);
    vpen_disable ();

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
  }

  return cbWrote;
}

static void nor_erase (struct descriptor_d* d, size_t cb)
{
  int pageLast = -1;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  while (cb > 0) {
    unsigned long index = d->start + d->index;
    int page = index/chip->erase_size;
    unsigned long available
      = chip->erase_size - (index & (chip->erase_size - 1));
    unsigned short status; 

    index = phys_from_index (index);

    if (available > cb)
      available = cb;

    index &= ~(chip->erase_size - 1);

    vpen_enable ();
    if (page != pageLast) {
      status = nor_unlock_page (index);
      if (status & (ProgramError | VPEN_Low | DeviceProtected))
	goto fail;
      pageLast = page;
    }
    WRITE_ONE (index, Erase);
    WRITE_ONE (index, EraseConfirm);
    status = nor_status (index);
    vpen_disable ();

    if (status & (EraseError | VPEN_Low | DeviceProtected)) {
    fail:
      printf ("Erase failed at 0x%p (%x)\r\n", (void*) index, status);
      CLEAR_STATUS (index);
      return;
    }

    cb -= available;
    d->index += cb;
  }
}

static __driver_3 struct driver_d nor_driver = {
  .name = "nor-79524",
  .description = "NOR flash driver",
  .probe = nor_probe,
  .open = nor_open,
  .close = close_descriptor,
  .read = nor_read,
  .write = nor_write,
  .erase = nor_erase,
  .seek = seek_descriptor,
};

