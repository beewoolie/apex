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

*/

#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <apex.h>
#include <config.h>
#include "lh7a40x.h"
#include <spinner.h>
#define TALK

#define NOR_0_PHYS	(0x00000000)
#define NOR_0_LENGTH	(16*1024*1024)
#define WIDTH		(32)		/* Device width in bits */
#define WIDTH_SHIFT	(WIDTH>>4)	/* Bit shift for device width */

#define CMD(v) ((v) | ((v)<<16))
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

#define CPLD_FLASH	(0x71000000)
#define FPEN		(1<<0)
#define FST1		(1<<1)
#define FST2		(1<<2)

#define STAT(v) ((v) | ((v)<<16))
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
  { 0x89, 0x17, 16*1024*1024, 256*1024 }, /* i28F640J3A */
};

const static struct nor_chip* chip;

static unsigned long phys_from_index (unsigned long index)
{
  return index + NOR_0_PHYS;
}

static void vpen_enable (void)
{
  __REG16 (CPLD_FLASH) |= FPEN;
}

static void vpen_disable (void)
{
  __REG16 (CPLD_FLASH) &= ~FPEN;
}

static unsigned long nor_read_one (unsigned long index)
{
  return __REG (index);
}
#define READ_ONE(i) nor_read_one (i)

static void nor_write_one (unsigned long index, unsigned long v)
{
  __REG (index) = v;
}
#define WRITE_ONE(i,v) nor_write_one ((i), (v))

#define CLEAR_STATUS(i) nor_write_one(i, CMD(ClearStatus))

static unsigned long nor_status (unsigned long index)
{
  unsigned long status; 
  unsigned long time = timer_read ();
  do {
    status = READ_ONE (index);
  } while (   (status & STAT(Ready)) != 0
           && timer_delta (time, timer_read ()) < 6*1000);
  return status;
}

static unsigned long nor_unlock_page (unsigned long index)
{
  WRITE_ONE (index, CMD(Configure));
  WRITE_ONE (index, CMD(LockClear));
  return nor_status (index);
}

static void nor_init (void)
{
  unsigned char manufacturer;
  unsigned char device;

  vpen_disable ();

  __REG (NOR_0_PHYS) = CMD(ReadArray);
  __REG (NOR_0_PHYS) = CMD(ReadID);

  if (   __REG (NOR_0_PHYS + (0x10 << WIDTH_SHIFT)) != ('Q'|('Q'<<16))
      || __REG (NOR_0_PHYS + (0x11 << WIDTH_SHIFT)) != ('R'|('R'<<16))
      || __REG (NOR_0_PHYS + (0x12 << WIDTH_SHIFT)) != ('Y'|('Y'<<16)))
    return;

  manufacturer = __REG (NOR_0_PHYS + (0x00 << WIDTH_SHIFT)) & 0xff;
  device       = __REG (NOR_0_PHYS + (0x01 << WIDTH_SHIFT)) & 0xff;

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

  __REG (NOR_0_PHYS) = CMD(ClearStatus);
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
    WRITE_ONE (index, CMD(ReadArray));
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

  return;			/* *** this code won't work */

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  while (cb) {
    unsigned long index = d->start + d->index;
    int page = index/chip->erase_size;
    unsigned long data;
    unsigned long status;
    int step = 4;

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

    vpen_enable ();
    if (page != pageLast) {
      status = nor_unlock_page (index);
      if (status & (ProgramError | VPEN_Low | DeviceProtected))
	goto fail;
      pageLast = page;
    }
    WRITE_ONE (index, CMD(Program));
    WRITE_ONE (index, data);
    status = nor_status (index);
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
  }

  return cbWrote;
}

static void nor_erase (struct descriptor_d* d, size_t cb)
{
  int pageLast = -1;

  /* *** This code didn't erase correctly. */
  return;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  SPINNER_STEP;

  while (cb > 0) {
    unsigned long index = d->start + d->index;
    int page = index/chip->erase_size;
    unsigned long available
      = chip->erase_size - (index & (chip->erase_size - 1));
    unsigned long status; 

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
    WRITE_ONE (index, CMD(Erase));
    WRITE_ONE (index, CMD(EraseConfirm));
    status = nor_status (index);
    vpen_disable ();

    SPINNER_STEP;

    if (status & (STAT(EraseError) | STAT(VPEN_Low) | STAT(DeviceProtected))) {
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
  .name = "nor-7a40x",
  .description = "NOR flash driver",
  .open = nor_open,
  .close = close_helper,
  .read = nor_read,
  .write = nor_write,
  .erase = nor_erase,
  .seek = seek_helper,
};

static __service_6 struct service_d lh7a40x_nor_service = {
  .init = nor_init,
};
