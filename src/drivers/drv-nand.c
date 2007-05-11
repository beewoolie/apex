/* drv-nand.c

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

   NAND flash driver.

   Unlike the NOR driver, this one hasn't received the same attention
   for reducing code size.  It is probably amenable to the same
   optimizations.  However, since it already much smaller than the NOR
   driver, there isn't the same incentive to optimize.

   Status
   ------

   (1<<0) Program/erase error
   (1<<6) Ready
   (1<<7) !R/O

*/

#include <config.h>
#include <apex.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <spinner.h>
#include <error.h>
#include <command.h>

#include <drv-nand-base.h>
#include "mach/drv-nand.h"

#define TALK

static void wait_on_busy (void);

#if defined (CONFIG_DRIVER_NAND_TYPE_ST)

inline void nand_read_setup (unsigned long page, int index)
{
  NAND_CLE = NAND_ReadSetup;
  NAND_ALE = index & 0xff;
  NAND_ALE = (index >> 8) & 0xff;
  NAND_ALE = page & 0xff;
  NAND_ALE = (page >>  8) & 0xff;
//  NAND_ALE = (page >> 16) & 0xff;
  NAND_CLE = NAND_Read;
  wait_on_busy ();
  NAND_CLE = NAND_Read;
}

#endif

#if defined (CONFIG_DRIVER_NAND_TYPE_TOSHIBA)

inline void nand_read_setup (unsigned long page, int index)
{
  NAND_CLE = (index < 256) ? NAND_Read1 : NAND_Read2;
  NAND_ALE = (index & 0xff);
  NAND_ALE = ( page        & 0xff);
  NAND_ALE = ((page >>  8) & 0xff);
#if NAND_ADDRESSES > 2
  NAND_ALE = ((page >> 16) & 0xff);
#endif
  wait_on_busy ();

		/* Switch back to read mode */
  NAND_CLE = (index < 256) ? NAND_Read1 : NAND_Read2;
}

#endif

#if !defined (NAND_ENABLE)
# define NAND_ENABLE
#endif
#if !defined (NAND_WP_ENABLE)
# define NAND_WP_ENABLE
#endif
#if !defined (NAND_WP_DISABLE)
# define NAND_WP_DISABLE
#endif

#if defined (CONFIG_NAND_ADDRESS_BYTES)
# define NAND_ADDRESSES		CONFIG_DRIVER_NAND_ADDRES_BYTES
#else
# define NAND_ADDRESSES		(2)
#endif

struct nand_chip {
  int id_mask;			/* Bits indicating which ID bytes to match */
  unsigned short id[4];
  unsigned long total_size;
  int erase_size;
  int page_size;
  //  int address_size;	/* *** FIXME: Better than Boot code */
};

const static struct nand_chip chips[] = {
  { (1<<1),
    { 0x98, 0x75 },		/* Toshiba - 256 MiB*/
    32*1024*1024, 16*1024, 512 },
  { (1<<0) | (1<<1) | (1<<2) | (1<<3),
    { 0x20, 0xf1, 0x80, 0x15},	/* ST - 1 GiB */
    128*1024*1024, 128*1024, 2048 },
};

const static struct nand_chip* chip;


/* wait_on_busy

   checks the status of the device and returns when it is no longer
   busy.  This call *requires* that the nCE line is already pulled
   low.

*/

static void wait_on_busy (void)
{
  /* *** FIXME: need to time-out and detect absence of the device */
  while (NAND_ISBUSY)
    ;
}


/* nand_init

   probes the NAND flash device.

   Note that the status check redundantly sends the Status command
   when we are not using the CONFIG_NAND_LPD mode.  It's left in for
   now.

*/

static void nand_init (void)
{
  unsigned short id[4];
//  unsigned short manufacturer;
//  unsigned short device;
//  unsigned short third;
//  unsigned short fourth;

  NAND_ENABLE;			/* Optional setup for NAND flash */

  NAND_CS_ENABLE;

  NAND_CLE = NAND_Reset;
  wait_on_busy ();

  NAND_CLE = NAND_Status;
  wait_on_busy ();
  if ((NAND_DATA & NAND_Ready) == 0)
    goto exit;

  NAND_CLE = NAND_ReadID;
  NAND_ALE = 0;

  id[0] = NAND_DATA;
  id[1] = NAND_DATA;
  id[2] = NAND_DATA;
  id[3] = NAND_DATA;

  for (chip = &chips[0];
       chip < chips + sizeof(chips)/sizeof (struct nand_chip);
       ++chip)
#define _M(i)\
	(!(chip->id_mask & (1<<(i))) || id[i] == chip->id[i])
    if (_M (0) && _M (1) && _M (1) && _M (1))
      break;
#undef _M
  if (chip >= chips + sizeof(chips)/sizeof (chips[0]))
      chip = NULL;

#if defined (TALK)
  printf ("\n* NAND flash");

  if (chip)
    printf (" %ld MiB total, %d KiB erase, %d B page"
	    " (0x%02x/0x%02x/0x%02x/0x%02x)\n",
	    chip->total_size/(1024*1024), chip->erase_size/1024,
	    chip->page_size,
	    id[0], id[1], id[2], id[3]);
  else
    printf (" unknown 0x%02x/0x%02x/0x%02x/0x%02x\n",
	    id[0], id[1], id[2], id[3]);
#endif

 exit:
  NAND_CS_DISABLE;
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

  NAND_CS_ENABLE;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  while (cb) {
    unsigned long page  = (d->start + d->index)/chip->page_size;
    int index = (d->start + d->index)%chip->page_size;
    int available = chip->page_size - index;

    if (available > cb)
      available = cb;

    d->index += available;
    cb -= available;
    cbRead += available;

    NAND_CLE = NAND_Reset;
    wait_on_busy ();
    nand_read_setup (page, index);

    while (available--)		/* May optimize with assembler...later */
      *((char*) pv++) = NAND_DATA;
  }

  NAND_CS_DISABLE;

  return cbRead;
}

static ssize_t nand_write (struct descriptor_d* d, const void* pv, size_t cb)
{
  int cbWrote = 0;

  if (!chip)
    return cbWrote;

  NAND_CS_ENABLE;
  NAND_WP_DISABLE;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  SPINNER_STEP;

  while (cb) {
    unsigned long page  = (d->start + d->index)/chip->page_size;
    unsigned long index = (d->start + d->index)%chip->page_size;
    int available = chip->page_size - index;
    int tail;

    if (available > cb)
      available = cb;
    /* A previous version of this code wrote to the end of the page,
       including the auxiliary region.  This is no longer the case.
       Now, we only write to the end of the data area.  */
//    tail = 528 - index - available;
    tail = chip->page_size - index - available;

	/* Reset and read to perform I/O on the data region  */
    NAND_CLE = NAND_Reset;
    wait_on_busy ();
#if defined (NAND_Read1) && defined (NAND_SerialInput)
    NAND_CLE = NAND_Read1;
    NAND_ALE = 0;
    NAND_ALE = ( page        & 0xff);
    NAND_ALE = ((page >>  8) & 0xff);
#if NAND_ADDRESSES > 2
    NAND_ALE = ((page >> 16) & 0xff);
#endif
    wait_on_busy ();

	/* Perform write */
    NAND_CLE = NAND_SerialInput;
    NAND_ALE = 0;	/* Always start at page beginning */
    NAND_ALE = ( page        & 0xff);
    NAND_ALE = ((page >>  8) & 0xff);
#if NAND_ADDRESSES > 2
    NAND_ALE = ((page >> 16) & 0xff);
#endif

    while (index--)	   /* Skip to the portion we want to change */
      NAND_DATA = 0xff;

#endif

    d->index += available;
    cb -= available;
    cbWrote += available;

#if defined (NAND_AutoProgram)
    while (available--)
      NAND_DATA = *((char*) pv++);

    while (tail--)	   /* Fill to end of block */
      NAND_DATA = 0xff;

    NAND_CLE = NAND_AutoProgram;

    wait_on_busy ();
#endif

    SPINNER_STEP;

    NAND_CLE = NAND_Status;
    if (NAND_DATA & NAND_Fail) {
      printf ("Write failed at page %ld\n", page);
      goto exit;
    }
  }

 exit:
  NAND_WP_ENABLE;
  NAND_CS_DISABLE;

  return cbWrote;
}

static void nand_erase (struct descriptor_d* d, size_t cb)
{
  if (!chip)
    return;

  NAND_CS_ENABLE;
  NAND_WP_DISABLE;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  SPINNER_STEP;

  do {
    unsigned long page = (d->start + d->index)/chip->page_size;
    unsigned long available
      = chip->erase_size - ((d->start + d->index) & (chip->erase_size - 1));

    NAND_CLE = NAND_Erase;
    NAND_ALE = ( page & 0xff);
    NAND_ALE = ((page >> 8) & 0xff);
#if NAND_ADDRESSES > 2
    NAND_ALE = ((page >> 16) & 0xff);
#endif
    NAND_CLE = NAND_EraseConfirm;

    wait_on_busy ();

    SPINNER_STEP;

    NAND_CLE = NAND_Status;
    if (NAND_DATA & NAND_Fail) {
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
  NAND_WP_ENABLE;
  NAND_CS_DISABLE;
}

#if !defined (CONFIG_SMALL)

static void nand_report (void)
{
  unsigned char status;

#if defined (USE_DETECT_ENDIAN_MISMATCH)
  if (endian_error) {
    printf ("  nor:    *** Endian Mismatch ***\n");
    return;
  }
#endif

  if (!chip)
    return;

  NAND_CS_ENABLE;
  NAND_CLE = NAND_Status;
  status = NAND_DATA;
  NAND_CS_DISABLE;

  printf ("  nand:   %ld MiB total, %d KiB erase, %d B page %s%s%s\n",
	  chip->total_size/(1024*1024), chip->erase_size/1024,
	  chip->page_size,
	  (status & NAND_Fail) ? " FAIL" : "",
	  (status & NAND_Ready) ? " RDY" : "",
	  (status & NAND_Writable) ? " R/W" : " R/O"
	  );
}

#endif

static __driver_3 struct driver_d nand_driver = {
  .name = "nand",
  .description = "NAND flash driver",
  .flags = DRIVER_WRITEPROGRESS(6),
  .open = nand_open,
  .close = close_helper,
  .read = nand_read,
  .write = nand_write,
  .erase = nand_erase,
  .seek = seek_helper,
};

static __service_6 struct service_d nand_service = {
  .init = nand_init,
#if !defined (CONFIG_SMALL)
  .report = nand_report,
#endif
};

#if defined (CONFIG_CMD_NAND)

int cmd_nand (int argc, const char** argv)
{
  if (!chip)
    return 0;

  if (strcmp (argv[1], "mark") == 0) {
    unsigned long cb = 32*1024*1024;
    int c;
    int last = -1;

    for (c = 0; c < cb; c += chip->page_size) {
      char rgb[chip->page_size];
      char sz[20];
      int result;
      struct descriptor_d d;

      {
	int v = c/16384;
	if (v != last)
	  printf ("  %04x    \r", v);
	last = v;
      }

      {
	int i;
	for (i = 0; i < chip->page_size; i += 4)
	  *(unsigned long*)&rgb[i] = i + c;
      }

      sprintf (sz, "nand:0x%x+%d", c, chip->page_size);
      if (   (result = parse_descriptor (sz, &d))
	     || (result = open_descriptor (&d))) {
	printf ("Unable to open target %s\n", sz);
	return ERROR_FAILURE;
      }

      nand_write (&d, rgb, chip->page_size);
      close_descriptor (&d);
    }
  }

  if (strcmp (argv[1], "scan") == 0) {
    int index;
    int cBad = 0;

    NAND_CS_ENABLE;
    for (index = 0; index < chip->total_size; index += chip->erase_size) {
      int page = index/chip->page_size;
      NAND_CLE = NAND_Reset;
      wait_on_busy ();
      NAND_CLE = NAND_Read3;
      NAND_ALE = (index & 0xff);
      NAND_ALE = ( page        & 0xff);
      NAND_ALE = ((page >>  8) & 0xff);
#if NAND_ADDRESSES > 2
      NAND_ALE = ((page >> 16) & 0xff);
#endif
      wait_on_busy ();
      NAND_CLE = NAND_Read3;
      {
	char rgb[16];
	int i;
	int f = 0;
	for (i = 0; i < sizeof (rgb); ++i) {
	  rgb[i] = NAND_DATA;
	  if (rgb[i] != 0xff)
	    f = 1;
	}
	if (f) {
	  dumpw (rgb, 16, index, 1);
	  ++cBad;
	}
      }
    }
    NAND_CS_DISABLE;
    printf ("%d of %ld blocks are bad\n",
	    cBad, chip->total_size/chip->erase_size);
  }

  return 0;
}

static __command struct command_d c_nand = {
  .command = "nand",
  .func = cmd_nand,
  COMMAND_DESCRIPTION ("nand test function")
  COMMAND_HELP (
"nand [SUBCOMMAND [PARAMETER]]\n"
" mark       - Write to every block an identifying pattern\n"
" scan       - Scan for bad blocks\n"
"\n")
};

#endif
