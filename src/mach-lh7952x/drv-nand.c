/* drv-nand.c
     $Id$

   written by Marc Singer
   4 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

   NAND flash block IO driver for LPD 79524.

   Unlike the NOR driver, this one hasn't received the same attention
   for reducing the size of the loader.  It is probably amenable to
   the same optimizations.  However, since it is so much shorter than
   the nor dirver, there isn't the same need to perform the
   optimization.

*/

#include <driver.h>
#include <linux/string.h>
#include <apex.h>
#include <config.h>
#include <spinner.h>

#include "lh79524.h"

#define NAND_PHYS	(0x40800000)
#define NAND_DATA	(NAND_PHYS | 0x00)
#define NAND_CLE	(NAND_PHYS | 0x10)
#define NAND_ALE	(NAND_PHYS | 0x08)

#define Reset		(0xff)
#define ReadID		(0x90)
#define Status		(0x70)
#define Read1		(0x00)	/* Start address in first 256 bytes of page */
#define Read2		(0x01)	/* Start address in second 256 bytes of page */
#define Read3		(0x50)	/* Start address in last 16 bytes, (ECC?) */
#define Erase		(0x60)
#define EraseConfirm	(0xd0)
#define AutoProgram	(0x10)
#define SerialInput	(0x80)

#define Fail		(1<<0)
#define Ready		(1<<6)
#define Writable	(1<<7)

#define CPLD_REG_FLASH	(0x4c800000)
#define RDYnBSY		(1<<2)

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
  while ((__REG8 (CPLD_REG_FLASH) & RDYnBSY) == 0)
    ;
}

static int nand_probe (void)
{
  unsigned char manufacturer;
  unsigned char device;

  __REG8 (NAND_CLE) = Reset;
  wait_on_busy ();
  __REG8 (NAND_CLE) = Status;
  wait_on_busy ();
  if ((__REG8 (NAND_DATA) & Ready) == 0) {
//    printf ("NAND flash not found\r\n");
    return 1;
  }

  __REG8 (NAND_CLE) = ReadID;
  __REG8 (NAND_ALE) = 0;
  wait_on_busy ();

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

  return chip == NULL;		/* Present and initialized */
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

    //    printf ("nand-read %ld page  %d index  %d available\r\n",
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
    unsigned long page  = d->index/512;
    unsigned long index = d->index%512;
    int available = 512 - index;
    int tail;

    if (available > cb)
      available = cb;
    tail = 528 - index - available;
    
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

static void nand_erase (struct descriptor_d* d, size_t cb)
{
  if (!chip)
    return;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  SPINNER_STEP;

  do {
    unsigned long block = (d->start + d->index)/chip->erase_size;
    unsigned long available
      = chip->erase_size - ((d->start + d->index) & (chip->erase_size - 1));

    __REG8 (NAND_CLE) = Erase;
    __REG8 (NAND_ALE) = (block & 0xff);
    __REG8 (NAND_ALE) = ((block >> 8) & 0xff);
    __REG8 (NAND_CLE) = EraseConfirm;

    wait_on_busy ();

    SPINNER_STEP;

    __REG8 (NAND_CLE) = Status;
    if (__REG8 (NAND_DATA) & Fail) {
      printf ("Erase failed at block %ld\r\n", block);
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
}


static __driver_3 struct driver_d nand_driver = {
  .name = "nand-79524",
  .description = "NAND flash driver",
  //  .flags = DRIVER_ | DRIVER_CONSOLE,
  .probe = nand_probe,
  .open = nand_open,
  .close = close_descriptor,
  .read = nand_read,
  .write = nand_write,
  .erase = nand_erase,
  .seek = seek_descriptor,
};

