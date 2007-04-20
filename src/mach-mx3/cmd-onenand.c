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

   OneNAND test code.

*/

#include <config.h>
#include <apex.h>
#include <command.h>
#include <service.h>
#include <linux/string.h>
#include <error.h>
#include <linux/kernel.h>

#include "hardware.h"

#define MULTIPLIER	2
//#define MULTIPLER	4	/* Helps debug broken NFF */

#define NAND_BASE	(0xa0000000)
#define NAND_BOOTRAM	(NAND_BASE + 0x0000*MULTIPLIER)
#define NAND_DATARAM0	(NAND_BASE + 0x0200*MULTIPLIER)
#define NAND_DATARAM1	(NAND_BASE + 0x0800*MULTIPLIER)

#define REG(x)		__REG16 (NAND_BASE + (x)*MULTIPLIER)

#define NAND_OP		REG(0x0000)
#define NAND_MAN_ID	REG(0xf000)
#define NAND_DEV_ID	REG(0xf001)
#define NAND_VER_ID	REG(0xf002)
#define NAND_DATA_SIZE	REG(0xf003)
#define NAND_BOOT_SIZE	REG(0xf004)
#define NAND_BUFF_CNT	REG(0xf005)
#define NAND_TECH_INFO	REG(0xf006)
#define NAND_SA_1	REG(0xf100)
#define NAND_SA_2	REG(0xf101)
#define NAND_SA_3	REG(0xf102)
#define NAND_SA_4	REG(0xf103)
#define NAND_SA_5	REG(0xf104)
#define NAND_SA_6	REG(0xf105)
#define NAND_SA_7	REG(0xf106)
#define NAND_SA_8	REG(0xf107)
#define NAND_SB		REG(0xf200)
#define NAND_CMD	REG(0xf220)
#define NAND_CONFIG_1	REG(0xf221)
#define NAND_CONFIG_2	REG(0xf222)
#define NAND_STATUS	REG(0xf240)
#define NAND_INTR	REG(0xf241)
#define NAND_SBA	REG(0xf24c)
#define NAND_WP_STATUS	REG(0xf24e)

#define NAND_CMD_LOAD		0x00
#define NAND_CMD_LOAD_SPARE	0x13
#define NAND_CMD_PROGRAM	0x80
#define NAND_CMD_PROGRAM_SPARE	0x1a
#define NAND_CMD_COPY_BACK	0x1b
#define NAND_CMD_RESET_CORE	0xf0
#define NAND_CMD_RESET_ALL	0xf3
#define NAND_CMD_UNLOCK		0x23

#define NAND_STATUS_ERROR	(1<<10)

#define NAND_INTR_READY		(1<<15)

#define DFS_FBA(x)		(((x)&0x3ff) | ((x)&0x400 << 4))
#define DBS(x)			(((x)&1)<<15)
#define FPA_FSA(x)		(x)
#define BSA_BSC(d,s,c)		(((d)?(1<<11):0)|(((s)&0x7)<<8)|((c)&0x3))
#define SBA(x)			(((x)/64)&0x3ff)

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
    NAND_SA_1 = DFS_FBA (page/64);
    NAND_SA_2 = DBS (0);
    NAND_SA_8 = FPA_FSA ((page%64)*4);
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
    NAND_SA_1 = DFS_FBA (page/64);
    NAND_SA_2 = DBS (0);
    NAND_SA_8 = FPA_FSA ((page%64)*4);
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
    NAND_SA_1 = DFS_FBA (page/64);
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
