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
#define WIDTH		(1)	/* Bit shift for device width */

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

struct nor_descriptor {
  size_t ibStart;
  size_t cb;
  size_t ib;
};

struct nor_chip {
  unsigned char manufacturer;
  unsigned char device;
  unsigned long total_size;
  int erase_size;
};

const static struct nor_chip chips[] = {
  { 0xb0, 0x18, 16*1024*1024, 128*1024 },
};

const static struct nor_chip* chip;

static struct nor_descriptor descriptors[2];

static unsigned long phys_from_ib (unsigned long ib)
{
  return ib + ((ib < NOR_0_LENGTH) ? NOR_0_PHYS : (NOR_1_PHYS - NOR_0_LENGTH));
}

static int nor_probe (void)
{
  unsigned char manufacturer;
  unsigned char device;

  __REG16 (CPLD_REG_FLASH) &= ~FL_VPEN;

  __REG16 (NOR_0_PHYS) = ReadArray;
  __REG16 (NOR_0_PHYS) = ReadID;

  if (   __REG16 (NOR_0_PHYS + (0x10 << WIDTH)) != 'Q'
      || __REG16 (NOR_0_PHYS + (0x11 << WIDTH)) != 'R'
      || __REG16 (NOR_0_PHYS + (0x12 << WIDTH)) != 'Y')
    return 0;

  manufacturer = __REG16 (NOR_0_PHYS + (0x00 << WIDTH));
  device       = __REG16 (NOR_0_PHYS + (0x01 << WIDTH));

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


static unsigned long nor_open (struct open_d* d)
{
  int fh;

  if (!chip)
    return -1;

  for (fh = 0; fh < sizeof (descriptors)/sizeof(struct nor_descriptor); ++fh)
    if (descriptors[fh].cb == 0)
      break;

  if (fh >= sizeof (descriptors)/sizeof(struct nor_descriptor))
    return -1;

  descriptors[fh].ibStart = d->start;
  descriptors[fh].cb = d->length;
  descriptors[fh].ib = 0;
      
  /* *** FIXME: bounding of the cb is a good idea here */

  return fh;
}

static void nor_close (unsigned long fh)
{
  descriptors[fh].cb = 0;
} 

static ssize_t nor_read (unsigned long fh, void* pv, size_t cb)
{
  ssize_t cbRead = 0;

  if (!chip)
    return cbRead;

  if (descriptors[fh].ib + cb > descriptors[fh].cb)
    cb = descriptors[fh].cb - descriptors[fh].ib;

  while (cb) {
    unsigned long index = descriptors[fh].ibStart + descriptors[fh].ib;
    int available = cb;
    if (index < NOR_0_LENGTH && index + available > NOR_0_LENGTH)
      available = NOR_0_LENGTH - index;
    index = phys_from_ib (index);

    descriptors[fh].ib += available;
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

static void nor_erase (unsigned long fh, size_t cb)
{
  if (!chip)
    return;

  if (cb > descriptors[fh].ib + descriptors[fh].cb)
    cb = descriptors[fh].cb + descriptors[fh].ib;

  while (cb > 0) {
    unsigned long index
      = phys_from_ib (descriptors[fh].ibStart + descriptors[fh].ib);
    unsigned long available
      = ((index + chip->erase_size)&~(chip->erase_size - 1)) - index;
    unsigned short status; 
    if (available > cb)
      available = cb;

    index &= ~(chip->erase_size - 1);

    __REG16 (CPLD_REG_FLASH) |= FL_VPEN;
    __REG16 (index) = Erase;
    __REG16 (index) = EraseConfirm;

    do {
      status = __REG16 (index);
    } while ((status & Ready) == 0);
    
    __REG16 (CPLD_REG_FLASH) &= ~FL_VPEN;

    if (status & EraseError) {
      printf ("Erase failed at 0x%p (%x)\r\n", (void*) index, status);
      __REG16 (index) = ClearStatus;
      return;
    }

    cb -= available;
    descriptors[fh].ib += cb;

    __REG16 (index) = ReadArray;
  }
}

static size_t nor_seek (unsigned long fh, ssize_t ib, int whence)
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

static __driver_3 struct driver_d nor_driver = {
  .name = "nor-79524",
  .description = "NOR flash driver",
  .probe = nor_probe,
  .open = nor_open,
  .close = nor_close,
  .read = nor_read,
  //  .write = nor_write,
  .erase = nor_erase,
  .seek = nor_seek,
};

