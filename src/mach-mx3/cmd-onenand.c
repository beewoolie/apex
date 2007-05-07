/* cmd-onenand.c

   written by Marc Singer
   30 Mar 2007

   Copyright (C) 2007 Marc Singer

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

   OneNAND driver and test routines.

   o Partial Writes.  We must infer from the OneNAND documentation
     that rewriting portions of a page require a read-modify-write
     cycle.  We don't really assume that we can rewrite a page from
     APEX, so the HW implementation details are somewhat irrelevent.

*/

#include <config.h>
#include <apex.h>
#include <command.h>
#include <service.h>
#include <linux/string.h>
#include <error.h>
#include <linux/kernel.h>
#include <driver.h>
#include <service.h>
#include <spinner.h>

#include "hardware.h"

#define MULTIPLIER	2
//#define MULTIPLER	4	/* Helps debug broken NFF */

#define NAND_BASE		(0xa0000000)
#define NAND_BOOTRAM		(NAND_BASE + 0x0000*MULTIPLIER)
#define NAND_DATARAM0		(NAND_BASE + 0x0200*MULTIPLIER)
#define NAND_DATARAM1		(NAND_BASE + 0x0800*MULTIPLIER)

#define PAGES_PER_BLOCK		(64)

#define REG(x)			__REG16 (NAND_BASE + (x)*MULTIPLIER)

#define NAND_OP			REG(0x0000)
#define NAND_MAN_ID		REG(0xf000)
#define NAND_DEV_ID		REG(0xf001)
#define NAND_VER_ID		REG(0xf002)
#define NAND_DATA_SIZE		REG(0xf003)
#define NAND_BOOT_SIZE		REG(0xf004)
#define NAND_BUFF_CNT		REG(0xf005)
#define NAND_TECH_INFO		REG(0xf006)
#define NAND_SA_1		REG(0xf100)
#define NAND_SA_2		REG(0xf101)
#define NAND_SA_3		REG(0xf102)
#define NAND_SA_4		REG(0xf103)
#define NAND_SA_5		REG(0xf104)
#define NAND_SA_6		REG(0xf105)
#define NAND_SA_7		REG(0xf106)
#define NAND_SA_8		REG(0xf107)
#define NAND_SB			REG(0xf200)
#define NAND_CMD		REG(0xf220)
#define NAND_CONFIG_1		REG(0xf221)
#define NAND_CONFIG_2		REG(0xf222)
#define NAND_STATUS		REG(0xf240)
#define NAND_INTR		REG(0xf241)
#define NAND_SBA		REG(0xf24c)
#define NAND_WP_STATUS		REG(0xf24e)

#define NAND_CMD_LOAD		0x00
#define NAND_CMD_LOAD_SPARE	0x13
#define NAND_CMD_PROGRAM	0x80
#define NAND_CMD_PROGRAM_SPARE	0x1a
#define NAND_CMD_COPY_BACK	0x1b
#define NAND_CMD_RESET_CORE	0xf0
#define NAND_CMD_RESET_ALL	0xf3
#define NAND_CMD_UNLOCK		0x23
#define NAND_CMD_UNLOCKALL	0x27
#define NAND_CMD_BLOCKERASE	0x94


#define NAND_STATUS_TO		(1<<0)
#define NAND_STATUS_PLANE2CURR	(1<<1)
#define NAND_STATUS_PLANE2PREV	(1<<2)
#define NAND_STATUS_PLANE1CURR	(1<<3)
#define NAND_STATUS_PLANE1PREV	(1<<4)
#define NAND_STATUS_OTPBL	(1<<5)
#define NAND_STATUS_OTPL	(1<<6)
#define NAND_STATUS_RSTB	(1<<7)
#define NAND_STATUS_SUSPEND	(1<<9)
#define NAND_STATUS_ERROR	(1<<10)
#define NAND_STATUS_ERASE	(1<<11)
#define NAND_STATUS_PROG	(1<<12)
#define NAND_STATUS_LOAD	(1<<13)
#define NAND_STATUS_LOCK	(1<<14)
#define NAND_STATUS_ONGO	(1<<15)

#define NAND_INTR_READY		(1<<15)

#define DFS_FBA(x)		(((x)&0x3ff) | ((x)&0x400 << 4))
#define DBS(x)			(((x)&1)<<15)
#define FPA_FSA(x)		(x)
#define BSA_BSC(d,s,c)		(((d)?(1<<11):0)|(((s)&0x7)<<8)|((c)&0x3))
#define SBA(x)			(((x)/PAGES_PER_BLOCK)&0x3ff)

#define IS_ERROR		(NAND_STATUS & NAND_STATUS_ERROR)
#define IS_BUSY			(!(NAND_INTR & NAND_INTR_READY))

/*
 * Standard NAND flash commands
 */
//#define NAND_CMD_READ0          0
//#define NAND_CMD_READ1          1
//#define NAND_CMD_PAGEPROG       0x10
//#define NAND_CMD_READOOB        0x50
//#define NAND_CMD_ERASE1         0x60
//#define NAND_CMD_STATUS         0x70
//#define NAND_CMD_STATUS_MULTI   0x71
//#define NAND_CMD_SEQIN          0x80
//#define NAND_CMD_READID         0x90
//#define NAND_CMD_ERASE2         0xd0
//#define NAND_CMD_RESET          0xff

/* Extended commands for large page devices */
//#define NAND_CMD_READSTART      0x30
//#define NAND_CMD_CACHEDPROG     0x15


struct onenand_chip {
  unsigned short id[3];		/* Manufacturer, device, version */
  int technology;
  int page_size;
  int erase_size;
  int boot_size;
  int cBuffers;
  int unlocked;			/* Blocks have been unlocked */
};

struct onenand_chip chip;

static char* describe_status (int status)
{
  static char sz[80];
  snprintf (sz, sizeof (sz), "(0x%x)", status);

#define _(b,s) if (status & (b))\
	snprintf (sz + strlen (sz), sizeof (sz) - strlen (sz), " %s", s)

  _(NAND_STATUS_TO, "TO");
  _(NAND_STATUS_PLANE2CURR, "PLANE2CURR");
  _(NAND_STATUS_PLANE2PREV, "PLANE2PREV");
  _(NAND_STATUS_PLANE1CURR, "PLANE1CURR");
  _(NAND_STATUS_PLANE1PREV, "PLANE1PREV");
  _(NAND_STATUS_OTPBL, "OTPBL");
  _(NAND_STATUS_OTPL, "OTPL");
  _(NAND_STATUS_RSTB, "RSTB");
  _(NAND_STATUS_SUSPEND, "SUSPEND");
  _(NAND_STATUS_ERROR, "ERROR");
  _(NAND_STATUS_ERASE, "ERASE");
  _(NAND_STATUS_PROG, "PROG");
  _(NAND_STATUS_LOAD, "LOAD");
  _(NAND_STATUS_LOCK, "LOCK");
  _(NAND_STATUS_ONGO, "ONGO");
#undef _

  return sz;
}

static void onenand_unlock (void)
{
  if (chip.unlocked)
    return;

  NAND_SA_1 = DFS_FBA (0);
  NAND_SA_2 = DBS (0);
  NAND_SBA  = SBA (0);
  NAND_INTR = 0;
  NAND_CMD = NAND_CMD_UNLOCKALL;
  while (IS_BUSY)
    ;
  chip.unlocked = 1;
}

static int cmd_onenand (int argc, const char** argv)
{
  printf ("cmd_onenand\n");

  if (argc == 1) {
    printf ("  manu 0x%x  dev 0x%x  ver 0x%x  data 0x%x"
	    "  boot 0x%x  cnt 0x%x\n",
	    NAND_MAN_ID, NAND_DEV_ID, NAND_VER_ID,
	    NAND_DATA_SIZE, NAND_BOOT_SIZE, NAND_BUFF_CNT);
    printf ("  sa 1 0x%04x  2 0x%04x  3 0x%04x  4 0x%04x\n"
	    "     5 0x%04x  6 0x%04x  7 0x%04x  8 0x%04x\n",
	    NAND_SA_1, NAND_SA_2, NAND_SA_3, NAND_SA_4,
	    NAND_SA_5, NAND_SA_6, NAND_SA_7, NAND_SA_8);
    printf ("  sb 0x%04x\n", NAND_SB);
    printf ("  status 0x%04x  int 0x%04x  cfg1 0x%04x\n",
	    NAND_STATUS, NAND_INTR, NAND_CONFIG_1);
    printf ("  wp 0x%04x\n", NAND_WP_STATUS);
    return 0;
  }

  if (strcmp (argv[1], "select") == 0) {
    int chip;
    if (argc < 3)
      ERROR_RETURN (ERROR_PARAM, "select requires chip number");
    chip = simple_strtoul (argv[2], NULL, 0);
    NAND_SA_2 = DBS (chip);
  }

  if (strcmp (argv[1], "reset") == 0) {
    NAND_OP = NAND_CMD_RESET_CORE;
  }

  if (strcmp (argv[1], "load") == 0) {
    int page;
    if (argc < 3)
      ERROR_RETURN (ERROR_PARAM, "load requires page number");
    page = simple_strtoul (argv[2], NULL, 0);
    NAND_SA_1 = DFS_FBA (page/PAGES_PER_BLOCK);
    NAND_SA_2 = DBS (0);
    NAND_SA_8 = FPA_FSA ((page%PAGES_PER_BLOCK)*4);
    NAND_SB = BSA_BSC (1,0,4);
    NAND_INTR = 0;
    NAND_CMD = NAND_CMD_LOAD;
    printf ("NAND_INTR 0x%x  NAND_STATUS 0x%x\n", NAND_INTR, NAND_STATUS);
    while (IS_BUSY)
      ;
    if (IS_ERROR)
      printf ("error\n");
  }

  if (strcmp (argv[1], "store") == 0) {
    int page;
    if (argc < 3)
      ERROR_RETURN (ERROR_PARAM, "load requires page number");
    page = simple_strtoul (argv[2], NULL, 0);
    NAND_SA_1 = DFS_FBA (page/PAGES_PER_BLOCK);
    NAND_SA_2 = DBS (0);
    NAND_SA_8 = FPA_FSA ((page%PAGES_PER_BLOCK)*4);
    NAND_SB = BSA_BSC (1,0,4);
    NAND_INTR = 0;
    NAND_CMD = NAND_CMD_PROGRAM;
    while (IS_BUSY)
      ;
    if (IS_ERROR)
      printf ("error\n");
  }

  if (strcmp (argv[1], "unlock") == 0) {
    int page;
    if (argc < 3)
      ERROR_RETURN (ERROR_PARAM, "unlock requires page number");
    page = simple_strtoul (argv[2], NULL, 0);
    NAND_SA_1 = DFS_FBA (page/PAGES_PER_BLOCK);
    NAND_SA_2 = DBS (0);
    NAND_SBA  = SBA (page);
    NAND_INTR = 0;
    NAND_CMD = NAND_CMD_UNLOCK;
    while (IS_BUSY)
      ;
    if (IS_ERROR)
      printf ("error\n");
  }

  if (strcmp (argv[1], "burst") == 0) {
    NAND_CONFIG_1 |= 0x8000;
  }

  if (strcmp (argv[1], "sync") == 0) {
    __REG (0xb8002000)
      |= (1<<20)		/* SYNC */
      |  (2<<28)		/* BCD, /3 */
      |  (1<<7)			/* EW */
      ;
  }

  return 0;
}

static __command struct command_d c_onenand = {
  .command = "onenand",
  .func = (command_func_t) cmd_onenand,
  COMMAND_DESCRIPTION ("OneNAND test")
  COMMAND_HELP(
"onenand\n"
"  OneNAND test.\n"
"  select CHIP     - Select chip 0 or 1 within the OneNAND part\n"
"  reset           - Resets the OneNAND chip controller\n"
"  load PAGE       - Loads a page from OneNAND to Data0\n"
"  store PAGE      - Writes page to OneNAND from Data0\n"
"  burst           - Enable burst read mode\n"
  )
};


/* onenand_init

   probes the OneNAND flash device.

*/

static void onenand_init (void)
{
  chip.id[0] = NAND_MAN_ID;
  chip.id[1] = NAND_DEV_ID;
  chip.id[2] = NAND_VER_ID;
  chip.technology = NAND_TECH_INFO;
  chip.page_size = NAND_DATA_SIZE;
  chip.erase_size = chip.page_size*PAGES_PER_BLOCK;
  chip.boot_size = NAND_BOOT_SIZE;
  chip.cBuffers = NAND_BUFF_CNT;
}

static int onenand_open (struct descriptor_d* d)
{
  if (!chip.id[0])
    return -1;

  /* Perform bounds check */

  return 0;
}

static ssize_t onenand_read (struct descriptor_d* d, void* pv, size_t cb)
{
  ssize_t cbRead = 0;

  if (!chip.id[0])
    return cbRead;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  while (cb) {
    unsigned long page  = (d->start + d->index)/chip.page_size;
    int index = (d->start + d->index)%chip.page_size;
    int available = chip.page_size - index;

    if (available > cb)
      available = cb;

    d->index += available;
    cb -= available;
    cbRead += available;

    NAND_SA_1 = DFS_FBA (page/PAGES_PER_BLOCK);
    NAND_SA_2 = DBS (0);
    NAND_SA_8 = FPA_FSA ((page % PAGES_PER_BLOCK)*4);
    NAND_SB   = BSA_BSC (1, 0, 4);
    NAND_INTR = 0;
    NAND_CMD = NAND_CMD_LOAD;

    while (IS_BUSY)
      ;
    //    wait_on_busy ();

    memcpy (pv, (const char*) NAND_DATARAM0 + index, available);
    pv += available;
// while (available--)		/* May optimize with assembler...later */
//      *((char*) pv++) = NAND_DATA;
  }

  return cbRead;
}

static ssize_t onenand_write (struct descriptor_d* d,
			      const void* pv, size_t cb)
{
  int cbWrote = 0;

  if (!chip.id[0])
    return cbWrote;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  SPINNER_STEP;

  onenand_unlock ();

  while (cb) {
    unsigned long page  = (d->start + d->index)/chip.page_size;
    unsigned long index = (d->start + d->index)%chip.page_size;
    int available = chip.page_size - index;

    if (available > cb)
      available = cb;

		/* Prepare OneNAND data buffer for partial write */
    if (available != chip.page_size) {
      NAND_SA_1 = DFS_FBA (page/PAGES_PER_BLOCK);
      NAND_SA_2 = DBS (0);
      NAND_SA_8 = FPA_FSA ((page%PAGES_PER_BLOCK)*4);
      NAND_SB = BSA_BSC (1,0,4);
      NAND_INTR = 0;
      NAND_CMD = NAND_CMD_LOAD;
      while (IS_BUSY)
	;
    }

    memcpy ((char*) NAND_DATARAM0 + index, pv, available);

    NAND_SA_1 = DFS_FBA (page/PAGES_PER_BLOCK);
    NAND_SA_2 = DBS (0);
    NAND_SA_8 = FPA_FSA ((page%PAGES_PER_BLOCK)*4);
    NAND_SB = BSA_BSC (1,0,4);
    NAND_INTR = 0;
    NAND_CMD = NAND_CMD_PROGRAM;
    while (IS_BUSY)
      ;

    SPINNER_STEP;

    if (NAND_STATUS & NAND_STATUS_ERROR) {
      printf ("Write failed at page %ld %s\n", page,
	      describe_status (NAND_STATUS));
      goto exit;
    }

    d->index += available;
    cb -= available;
    cbWrote += available;
    pv += available;

  }

 exit:

  return cbWrote;
}

static void onenand_erase (struct descriptor_d* d, size_t cb)
{
  if (!chip.id[0])
    return;

  onenand_unlock ();

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  SPINNER_STEP;

  do {
    unsigned long page = (d->start + d->index)/chip.page_size;
    unsigned long available
      = chip.erase_size - ((d->start + d->index) & (chip.erase_size - 1));

    NAND_SA_1 = DFS_FBA (page/PAGES_PER_BLOCK);
    NAND_SA_2 = DBS (0);
//    NAND_SA_8 = FPA_FSA ((page%PAGES_PER_BLOCK)*4);
//    NAND_SB = BSA_BSC (1,0,4);
    NAND_INTR = 0;
    NAND_CMD = NAND_CMD_BLOCKERASE;
    while (IS_BUSY)
      ;

    SPINNER_STEP;

    if (NAND_STATUS & NAND_STATUS_ERROR) {
      printf ("Erase failed at page %ld %s\n", page,
	      describe_status (NAND_STATUS));
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
  ;
}

#if !defined (CONFIG_SMALL)

static void onenand_report (void)
{
  //  unsigned char status;

  if (!chip.id[0])
    return;

  printf (" onenand: %d MiB total (%s), %d KiB erase, %d B page\n",
	  (1<<(((chip.id[1] >> 4) & 0xf)))*128/8,
	  (chip.id[1] & (1<<3)) ? "DDP" : "Single",
	  chip.erase_size/1024, chip.page_size);

#if 0
	  chip->total_size/(1024*1024), chip->erase_size/1024,
	  chip->page_size,
	  (status & NAND_Fail) ? " FAIL" : "",
	  (status & NAND_Ready) ? " RDY" : "",
	  (status & NAND_Writable) ? " R/W" : " R/O"
	  );
#endif
}

#endif

static __driver_3 struct driver_d onenand_driver = {
  .name = "nand-onenand",
  .description = "OneNAND flash driver",
  .flags = DRIVER_WRITEPROGRESS(6),
  .open = onenand_open,
  .close = close_helper,
  .read = onenand_read,
  .write = onenand_write,
  .erase = onenand_erase,
  .seek = seek_helper,
};

static __service_6 struct service_d onenand_service = {
  .init = onenand_init,
#if !defined (CONFIG_SMALL)
  .report = onenand_report,
#endif
};

