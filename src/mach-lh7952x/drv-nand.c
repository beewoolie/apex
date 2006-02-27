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

   There are two modes that this driver can operate in.  The default,
   !CONFIG_NAND_LPD adheres to the SHARP design for using A22 and A23
   to control the nOE and nWE signals.  This mode ought to work
   regardless of whether or not the system has an LPD CPLD chip.  The
   CONFIG_NAND_LPD mode is more efficient because the CPLD controls
   the NAND nCE signal and the A22 and A23 address lines can be used
   as address lines.  The SHARP design may impose restrictions on
   systems designed to require these higher order address lines.

*/

#include <config.h>
#include <apex.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <spinner.h>

#include "hardware.h"
#include "nand.h"

struct nand_chip {
  unsigned char device;
  unsigned long total_size;
  int erase_size;
  //  int address_size;	/* *** FIXME: Better than Boot code */
};

const static struct nand_chip chips[] = {
  { 0x75, 32*1024*1024, 16*1024 },
};

const static struct nand_chip* chip;

static void wait_on_busy (void)
{
#if defined CONFIG_NAND_LPD
  while ((CPLD_FLASH & CPLD_FLASH_RDYnBSY) == 0)
    ;
#else
  do {
    NAND_CLE = Status;
  } while ((NAND_DATA & Ready) == 0);
#endif
}


static void nand_enable (void)
{
#if !defined (CONFIG_NAND_LPD)
  IOCON_MUXCTL14 |=  (1<<8);
  GPIO_MN_PHYS   &= ~(1<<0);
  IOCON_MUXCTL7  &= ~(0xf<<12);
  IOCON_MUXCTL7  |=  (0xa<<12); /* Boot ROM uses 0xf */
//  IOCON_MUXCTL7  |=  (0xf<<12);
  IOCON_RESCTL7  &= ~(0xf<<12);
  IOCON_RESCTL7  |=  (0xa<<12);
#endif
}

static void nand_disable (void)
{
#if !defined (CONFIG_NAND_LPD)
  IOCON_MUXCTL14 &= ~(3<<8);
  GPIO_MN_PHYS |=   1<<0;
  IOCON_MUXCTL7  &= ~(0xf<<12);
  IOCON_MUXCTL7  |=  (0x5<<12);
  IOCON_RESCTL7  &= ~(0xf<<12);
  IOCON_RESCTL7  |=  (0x5<<12);
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

  nand_enable ();

  NAND_CLE = Reset;
  wait_on_busy ();

  NAND_CLE = Status;
  wait_on_busy ();
  if ((NAND_DATA & Ready) == 0)
    goto exit;

  NAND_CLE = ReadID;
  NAND_ALE = 0;

  manufacturer = NAND_DATA;
  device       = NAND_DATA;

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
    printf (" %ldMiB total, %dKiB erase\n",
	    chip->total_size/(1024*1024), chip->erase_size/1024);
  else
    printf (" unknown 0x%x/0x%x\n", manufacturer, device);
#endif

 exit:
  nand_disable ();
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

  nand_enable ();

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  while (cb) {
    unsigned long page  = (d->start + d->index)/512;
    int index = (d->start + d->index)%512;
    int available = 512 - index;

    if (available > cb)
      available = cb;

    d->index += available;
    cb -= available;
    cbRead += available;

    NAND_CLE = Reset;
    wait_on_busy ();
    NAND_CLE = (index < 256) ? Read1 : Read2;
    NAND_ALE = (index & 0xff);
    NAND_ALE = ( page        & 0xff);
    NAND_ALE = ((page >>  8) & 0xff);
    wait_on_busy ();
#if !defined (CONFIG_NAND_LPD)
		/* Switch back to read mode */
    NAND_CLE = (index < 256) ? Read1 : Read2;
#endif
    while (available--)		/* May optimize with assembler...later */
      *((char*) pv++) = NAND_DATA;
  }

  nand_disable ();

  return cbRead;
}

static ssize_t nand_write (struct descriptor_d* d, const void* pv, size_t cb)
{
  int cbWrote = 0;

  if (!chip)
    return cbWrote;

  nand_enable ();

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

    NAND_CLE = SerialInput;
    NAND_ALE = 0;	/* Always start at page beginning */
    NAND_ALE = ( page        & 0xff);
    NAND_ALE = ((page >>  8) & 0xff);

    while (index--)	   /* Skip to the portion we want to change */
      NAND_DATA = 0xff;

    d->index += available;
    cb -= available;
    cbWrote += available;

    while (available--)
      NAND_DATA = *((char*) pv++);

    while (tail--)	   /* Fill to end of block */
      NAND_DATA = 0xff;

    NAND_CLE = AutoProgram;

    wait_on_busy ();

    SPINNER_STEP;

    NAND_CLE = Status;
    if (NAND_DATA & Fail) {
      printf ("Write failed at page %ld\n", page);
      goto exit;
    }
  }

 exit:
  nand_disable ();

  return cbWrote;
}

static void nand_erase (struct descriptor_d* d, size_t cb)
{
  if (!chip)
    return;

  nand_enable ();

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  SPINNER_STEP;

  do {
    unsigned long page = (d->start + d->index)/512;
    unsigned long available
      = chip->erase_size - ((d->start + d->index) & (chip->erase_size - 1));

    NAND_CLE = Erase;
    NAND_ALE = ( page & 0xff);
    NAND_ALE = ((page >> 8) & 0xff);
    NAND_CLE = EraseConfirm;

    wait_on_busy ();

    SPINNER_STEP;

    NAND_CLE = Status;
    if (NAND_DATA & Fail) {
      printf ("Erase failed at page %ld\n", page);
      goto exit;
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

 exit:
  nand_disable ();
}

static __driver_3 struct driver_d nand_driver = {
  .name = "nand-lpd79524",
  .description = "NAND flash driver",
  .flags = DRIVER_WRITEPROGRESS(6),
  .open = nand_open,
  .close = close_helper,
  .read = nand_read,
  .write = nand_write,
  .erase = nand_erase,
  .seek = seek_helper,
};

static __service_6 struct service_d lpd79524_nand_service = {
  .init = nand_init,
};
