/* drv-nand.c
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

   NAND flash IO driver for LPD 79524.

   Unlike the NOR driver, this one hasn't received the same attention
   for reducing code size.  It is probably amenable to the same
   optimizations.  However, since it already much smaller than the NOR
   driver, there isn't the same incentive to optimize.

*/

#include <config.h>
#include <apex.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <spinner.h>

#include "lh79524.h"
#include "nand.h"

struct nand_chip {
  unsigned char device;
  unsigned long total_size;
  int erase_size;
};

const static struct nand_chip chips[] = {
  { 0x75, 32*1024*1024, 16*1024 },
};

const static struct nand_chip* chip;

static void wait_on_busy (void)
{
#if defined CONFIG_NAND_LPD
  while ((__REG8 (CPLD_REG_FLASH) & RDYnBSY) == 0)
    ;
#else
  do {
    __REG8 (NAND_CLE) = Status;
  } while ((__REG8 (NAND_DATA) & Ready) == 0);
#endif
}

/* nand_init

   probes the NAND flash device.

   Note that the status check redundantly sends the Status command
   when we are not using the CONFIG_NAND_LPD mode.  It's left in for
   now.

*/

static void nand_init (void)
{
  unsigned char manufacturer;
  unsigned char device;

  printf ("nand init\r\n");

  __REG8 (NAND_CLE) = Reset;
  wait_on_busy ();

  __REG8 (NAND_CLE) = Status;
  wait_on_busy ();
  if ((__REG8 (NAND_DATA) & Ready) == 0) {
    return;
  }

  __REG8 (NAND_CLE) = ReadID;
  __REG8 (NAND_ALE) = 0;
  //  wait_on_busy ();

  manufacturer = __REG8 (NAND_DATA);
  device       = __REG8 (NAND_DATA);

  for (chip = &chips[0]; 
       chip < chips + sizeof(chips)/sizeof (struct nand_chip);
       ++chip)
    if (chip->device == device)
      break;
  if (chip >= chips + sizeof(chips)/sizeof (chips[0]))
      chip = NULL;

#if 0
  printf ("NAND flash ");

  if (chip)
    printf (" %ldMiB total, %dKiB erase\r\n", 
	    chip->total_size/(1024*1024), chip->erase_size/1024);
  else
    printf (" unknown 0x%x/0x%x\r\n", manufacturer, device);
#endif

  return;
}

static int nand_open (struct descriptor_d* d)
{
  if (!chip)
    return -1;

  /* Perform bounds check */

  return 0;
}

static ssize_t nand_read (struct descriptor_d* d, void* pv, size_t cb)
{
  ssize_t cbRead = 0;

  if (!chip)
    return cbRead;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  while (cb) {
    unsigned long page  = (d->start + d->index)/512;
    int index = (d->start + d->index)%512;
    int available = 512 - index;

    if (available > cb)
      available = cb;

//    printf ("nand-read  page %ld  index %d  available %d\r\n",
//	    page, index, available);

    d->index += available;
    cb -= available;
    cbRead += available;

    __REG8 (NAND_CLE) = Reset;
    wait_on_busy ();
    __REG8 (NAND_CLE) = (index < 256) ? Read1 : Read2;
    __REG8 (NAND_ALE) = (index & 0xff);
    __REG8 (NAND_ALE) = ( page        & 0xff);
    __REG8 (NAND_ALE) = ((page >>  8) & 0xff);
    wait_on_busy ();
#if !defined (CONFIG_NAND_LPD)
    __REG8 (NAND_CLE) = (index < 256) ? Read1 : Read2;
#endif
    while (available--)		/* May optimize with assembler...later */
      *((char*) pv++) = __REG8 (NAND_DATA);
  }    
  
  return cbRead;
}

static ssize_t nand_write (struct descriptor_d* d, const void* pv, size_t cb)
{
  int cbWrote = 0;

  if (!chip)
    return cbWrote;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  SPINNER_STEP;

  while (cb) {
    unsigned long page  = (d->start + d->index)/512;
    unsigned long index = (d->start + d->index)%512;
    int available = 512 - index;
    int tail;

    if (available > cb)
      available = cb;
    tail = 528 - index - available;
    
//  printf ("nand_write (%ld) page %ld  index %ld  available %d  tail %d\r\n",
//   cb, page, index, available, tail);

    __REG8 (NAND_CLE) = SerialInput;
    __REG8 (NAND_ALE) = 0;	/* Always start at page beginning */
    __REG8 (NAND_ALE) = ( page        & 0xff);
    __REG8 (NAND_ALE) = ((page >>  8) & 0xff);

    while (index--)	   /* Skip to the portion we want to change */
      __REG8 (NAND_DATA) = 0xff;

    d->index += available;
    cb -= available;
    cbWrote += available;

    while (available--)
      __REG8 (NAND_DATA) = *((char*) pv++);

    while (tail--)	   /* Fill to end of block */
      __REG8 (NAND_DATA) = 0xff;

    __REG8 (NAND_CLE) = AutoProgram;

    wait_on_busy ();

    SPINNER_STEP;

    __REG8 (NAND_CLE) = Status;
    if (__REG8 (NAND_DATA) & Fail) {
      printf ("Write failed at page %ld\r\n", page);
      return cbWrote;
    }
  }    

  return cbWrote;
}

#if 0
#define LH79524_GPIO_NAND_CE  ((volatile u32*)0xFFFD9000)
#define NAND_DISABLE_CE(nand) do { *LH79524_GPIO_NAND_CE |= GPIO_PMDR_PM0;} while(0)
#define NAND_ENABLE_CE(nand) do { *LH79524_GPIO_NAND_CE &= ~GPIO_PMDR_PM0;} while(0)

#define WRITE_NAND_COMMAND(d, adr) do{ *(volatile u8 *)((unsigned long)adr | NAND_EN_BIT | _BIT(4)) = (u8)(d); } while(0)
#define WRITE_NAND_ADDRESS(d, adr) do{ *(volatile u8 *)((unsigned long)adr | NAND_EN_BIT | _BIT(3)) = (u8)(d); } while(0)
#define WRITE_NAND(d, adr) do{ *(volatile u8 *)((unsigned long)adr | NAND_EN_BIT) = (u8)d; } while(0)
#define READ_NAND(adr) ((volatile unsigned char)(*(volatile u8 *)((unsigned long)adr | _BIT(23))))
#endif


static void nand_erase (struct descriptor_d* d, size_t cb)
{
  if (!chip)
    return;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  SPINNER_STEP;

#if 0
  __REG (GPIO_MN_PHYS | 0x08) &= ~(1<<0); /* PM0 output */
  __REG (GPIO_MN_PHYS | 0x00) &= ~(1<<0); /* PM0 low */

  __REG (IOCON_PHYS | IOCON_MUXCTL14) &= ~(3<<8);
  __REG (IOCON_PHYS | IOCON_MUXCTL14) |=  (1<<8);
#endif

  do {
    unsigned long page = (d->start + d->index)/512;
    unsigned long available
      = chip->erase_size - ((d->start + d->index) & (chip->erase_size - 1));

//    printf ("nand erase %ld, %ld\r\n", page, available);

    __REG8 (NAND_CLE) = Erase;
    __REG8 (NAND_ALE) = ( page & 0xff);
    __REG8 (NAND_ALE) = ((page >> 8) & 0xff);
    __REG8 (NAND_CLE) = EraseConfirm;

    wait_on_busy ();

    SPINNER_STEP;

    __REG8 (NAND_CLE) = Status;
    if (__REG8 (NAND_DATA) & Fail) {
      printf ("Erase failed at page %ld\r\n", page);
//      goto fail;
      return;
    }

    if (available < cb) {
      cb -= available;
      d->index += available;
    }
    else {
      cb = 0;
      d->index = d->length;
    }
  } while (cb > 0);

#if 0
 fail:
  __REG (GPIO_MN_PHYS | 0x00) |= (1<<0); /* PM0 high */
  __REG (IOCON_PHYS | IOCON_MUXCTL14) &= ~(3<<8);
#endif
}

static __driver_3 struct driver_d nand_driver = {
  .name = "nand-79524",
  .description = "NAND flash driver",
  //  .flags = DRIVER_ | DRIVER_CONSOLE,
  .open = nand_open,
  .close = close_helper,
  .read = nand_read,
  .write = nand_write,
  .erase = nand_erase,
  .seek = seek_helper,
};

static __service_2 struct service_d lh79524_nand_service = {
  .init = nand_init,
};
