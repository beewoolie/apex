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

   *** FIXME: must unlock before erasing/writing.  There might be a
   *** function to lock after writing, too.

*/

#include <driver.h>
#include <linux/string.h>
#include <apex.h>
#include <config.h>
#include "lh79524.h"

#define TALK

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
#define Lock		(0x60)
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

static unsigned long phys_from_ib (unsigned long ib)
{
  return ib + ((ib < NOR_0_LENGTH) ? NOR_0_PHYS : (NOR_1_PHYS - NOR_0_LENGTH));
}

static void vpen_enable (void)
{
  __REG16 (CPLD_REG_FLASH) |= FL_VPEN;
}

static void vpen_disable (void)
{
  __REG16 (CPLD_REG_FLASH) &= ~FL_VPEN;
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
       chip < chips + sizeof(chips)/sizeof (struct nor_chip);
       ++chip)
    if (chip->manufacturer == manufacturer && chip->device == device)
      break;
  if (chip >= chips + sizeof(chips)/sizeof (chips[0]))
      chip = NULL;

#if defined (TALK)

  printf ("\r\nNOR flash ");

  if (chip)
    printf (" %ldMiB total, %dKiB erase\r\n", 
	    chip->total_size/(1024*1024), chip->erase_size/1024);
  else
    printf (" unknown 0x%x/0x%x\r\n", manufacturer, device);

#endif

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

  if (!chip)
    return cbRead;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  while (cb) {
    unsigned long index = d->start + d->index;
    int available = cb;
    if (index < NOR_0_LENGTH && index + available > NOR_0_LENGTH)
      available = NOR_0_LENGTH - index;
    index = phys_from_ib (index);

    d->index += available;
    cb -= available;
    cbRead += available;

    //    printf ("nor: 0x%p 0x%08lx %d\n", pv, index, available);
    __REG16(index) = 0xff;
    memcpy (pv, (void*) index, available);

    pv += available;
  }    
  
  return cbRead;
}

#if 0

static ssize_t nor_write (unsigned long fh, const void* pv, size_t cb)
{
  int cbWrote = 0;

  if (!chip)
    return cbWrote;

  if (descriptors[fh].ib + cb > descriptors[fh].cb)
    cb = descriptors[fh].cb - descriptors[fh].ib;

  while (cb) {
    unsigned long page  = descriptors[fh].ib/512;
    unsigned long index = descriptors[fh].ib%512;
    int available = 512 - index;
    int tail;

    if (available > cb)
      available = cb;
    tail = 528 - index - available;
    
    __REG8 (NOR_CLE) = SerialInput;
    __REG8 (NOR_ALE) = 0;	/* Always start at page beginning */
    __REG8 (NOR_ALE) = ( page        & 0xff);
    __REG8 (NOR_ALE) = ((page >>  8) & 0xff);

    while (index--)	   /* Skip to the portion we want to change */
      __REG8 (NOR_DATA) = 0xff;

    descriptors[fh].ib += available;
    cb -= available;
    cbWrote += available;

    while (available--)
      __REG8 (NOR_DATA) = *((char*) pv++);

    while (tail--)	   /* Fill to end of block */
      __REG8 (NOR_DATA) = 0xff;

    __REG8 (NOR_CLE) = AutoProgram;

    wait_on_busy ();

    __REG8 (NOR_CLE) = Status;
    if (__REG8 (NOR_DATA) & Fail) {
      printf ("Write failed at page %ld\r\n", page);
      return cbWrote;
    }
  }    

  return cbWrote;
}

#endif

static void nor_erase (struct descriptor_d* d, size_t cb)
{
  if (!chip)
    return;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  while (cb > 0) {
    unsigned long index
      = phys_from_ib (d->start + d->index);
    unsigned long available
      = ((index + chip->erase_size)&~(chip->erase_size - 1)) - index;
    unsigned short status; 
    if (available > cb)
      available = cb;

    index &= ~(chip->erase_size - 1);

    vpen_enable ();
    __REG16 (index) = Erase;
    __REG16 (index) = EraseConfirm;

    do {
      status = __REG16 (index);
    } while ((status & Ready) == 0);
    
    vpen_disable ();

    if (status & EraseError) {
      printf ("Erase failed at 0x%p (%x)\r\n", (void*) index, status);
      __REG16 (index) = ClearStatus;
      return;
    }

    cb -= available;
    d->index += cb;

    __REG16 (index) = ReadArray;
  }
}

static __driver_3 struct driver_d nor_driver = {
  .name = "nor-79524",
  .description = "NOR flash driver",
  .probe = nor_probe,
  .open = nor_open,
  .close = close_descriptor,
  .read = nor_read,
  //  .write = nor_write,
  .erase = nor_erase,
  .seek = seek_descriptor,
};

