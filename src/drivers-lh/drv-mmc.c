/* drv-mmc.c
     $Id$

   written by Marc Singer
   19 Oct 2005

   Copyright (C) 2005 Marc Singer

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

*/

#include <config.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <apex.h>
#include <command.h>
#include <mach/hardware.h>

extern struct driver_d* console;
extern char* strcat (char*, const char*);

#define TALK 2

#if defined (TALK)
#define PRINTF(f...)		printf (f)
#define ENTRY(l)		printf ("%s\n", __FUNCTION__)
#define DBG(l,f...)		do { if (TALK >= l) printf (f); } while (0)
#else
#define PRINTF(f...)		do {} while (0)
#define ENTRY(l)		do {} while (0)
#define DBG(l,f...)		do {} while (0)
#endif

  /* MMC codes cribbed from the Linux MMC driver */

/* Standard MMC commands (3.1)           type  argument     response */
   /* class 1 */
#define	MMC_GO_IDLE_STATE         0   /* bc                          */
#define MMC_SEND_OP_COND          1   /* bcr  [31:0] OCR         R3  */
#define MMC_ALL_SEND_CID          2   /* bcr                     R2  */
#define MMC_SET_RELATIVE_ADDR     3   /* ac   [31:16] RCA        R1  */
#define MMC_SET_DSR               4   /* bc   [31:16] RCA            */
#define MMC_SELECT_CARD           7   /* ac   [31:16] RCA        R1  */
#define MMC_SEND_CSD              9   /* ac   [31:16] RCA        R2  */
#define MMC_SEND_CID             10   /* ac   [31:16] RCA        R2  */
#define MMC_READ_DAT_UNTIL_STOP  11   /* adtc [31:0] dadr        R1  */
#define MMC_STOP_TRANSMISSION    12   /* ac                      R1b */
#define MMC_SEND_STATUS	         13   /* ac   [31:16] RCA        R1  */
#define MMC_GO_INACTIVE_STATE    15   /* ac   [31:16] RCA            */

  /* class 2 */
#define MMC_SET_BLOCKLEN         16   /* ac   [31:0] block len   R1  */
#define MMC_READ_SINGLE_BLOCK    17   /* adtc [31:0] data addr   R1  */
#define MMC_READ_MULTIPLE_BLOCK  18   /* adtc [31:0] data addr   R1  */

  /* class 3 */
#define MMC_WRITE_DAT_UNTIL_STOP 20   /* adtc [31:0] data addr   R1  */

  /* class 4 */
#define MMC_SET_BLOCK_COUNT      23   /* adtc [31:0] data addr   R1  */
#define MMC_WRITE_BLOCK          24   /* adtc [31:0] data addr   R1  */
#define MMC_WRITE_MULTIPLE_BLOCK 25   /* adtc                    R1  */
#define MMC_PROGRAM_CID          26   /* adtc                    R1  */
#define MMC_PROGRAM_CSD          27   /* adtc                    R1  */

  /* class 6 */
#define MMC_SET_WRITE_PROT       28   /* ac   [31:0] data addr   R1b */
#define MMC_CLR_WRITE_PROT       29   /* ac   [31:0] data addr   R1b */
#define MMC_SEND_WRITE_PROT      30   /* adtc [31:0] wpdata addr R1  */

  /* class 5 */
#define MMC_ERASE_GROUP_START    35   /* ac   [31:0] data addr   R1  */
#define MMC_ERASE_GROUP_END      36   /* ac   [31:0] data addr   R1  */
#define MMC_ERASE                37   /* ac                      R1b */

  /* class 9 */
#define MMC_FAST_IO              39   /* ac   <Complex>          R4  */
#define MMC_GO_IRQ_STATE         40   /* bcr                     R5  */

  /* class 7 */
#define MMC_LOCK_UNLOCK          42   /* adtc                    R1b */

  /* class 8 */
#define MMC_APP_CMD              55   /* ac   [31:16] RCA        R1  */
#define MMC_GEN_CMD              56   /* adtc [0] RD/WR          R1b */

/* SD commands                           type  argument     response */
  /* class 8 */
/* This is basically the same command as for MMC with some quirks. */
#define SD_SEND_RELATIVE_ADDR     3   /* ac                      R6  */

  /* Application commands */
#define SD_APP_SET_BUS_WIDTH      6   /* ac   [1:0] bus width    R1  */
#define SD_APP_OP_COND           41   /* bcr  [31:0] OCR         R3  */
#define SD_APP_SEND_SCR          51   /* adtc                    R1  */

/*
  MMC status in R1
  Type
  	e : error bit
	s : status bit
	r : detected and set for the actual command response
	x : detected and set during command execution. the host must poll
            the card by sending status command in order to read these bits.
  Clear condition
  	a : according to the card state
	b : always related to the previous command. Reception of
            a valid command will clear it (with a delay of one command)
	c : clear by read
 */

#define R1_OUT_OF_RANGE		(1 << 31)	/* er, c */
#define R1_ADDRESS_ERROR	(1 << 30)	/* erx, c */
#define R1_BLOCK_LEN_ERROR	(1 << 29)	/* er, c */
#define R1_ERASE_SEQ_ERROR      (1 << 28)	/* er, c */
#define R1_ERASE_PARAM		(1 << 27)	/* ex, c */
#define R1_WP_VIOLATION		(1 << 26)	/* erx, c */
#define R1_CARD_IS_LOCKED	(1 << 25)	/* sx, a */
#define R1_LOCK_UNLOCK_FAILED	(1 << 24)	/* erx, c */
#define R1_COM_CRC_ERROR	(1 << 23)	/* er, b */
#define R1_ILLEGAL_COMMAND	(1 << 22)	/* er, b */
#define R1_CARD_ECC_FAILED	(1 << 21)	/* ex, c */
#define R1_CC_ERROR		(1 << 20)	/* erx, c */
#define R1_ERROR		(1 << 19)	/* erx, c */
#define R1_UNDERRUN		(1 << 18)	/* ex, c */
#define R1_OVERRUN		(1 << 17)	/* ex, c */
#define R1_CID_CSD_OVERWRITE	(1 << 16)	/* erx, c, CID/CSD overwrite */
#define R1_WP_ERASE_SKIP	(1 << 15)	/* sx, c */
#define R1_CARD_ECC_DISABLED	(1 << 14)	/* sx, a */
#define R1_ERASE_RESET		(1 << 13)	/* sr, c */
#define R1_STATUS(x)            (x & 0xFFFFE000)
#define R1_CURRENT_STATE(x)    	((x & 0x00001E00) >> 9)	/* sx, b (4 bits) */
#define R1_READY_FOR_DATA	(1 << 8)	/* sx, a */
#define R1_APP_CMD		(1 << 5)	/* sr, c */

/* These are unpacked versions of the actual responses */

struct _mmc_csd {
	u8  csd_structure;
	u8  spec_vers;
	u8  taac;
	u8  nsac;
	u8  tran_speed;
	u16 ccc;
	u8  read_bl_len;
	u8  read_bl_partial;
	u8  write_blk_misalign;
	u8  read_blk_misalign;
	u8  dsr_imp;
	u16 c_size;
	u8  vdd_r_curr_min;
	u8  vdd_r_curr_max;
	u8  vdd_w_curr_min;
	u8  vdd_w_curr_max;
	u8  c_size_mult;
	union {
		struct { /* MMC system specification version 3.1 */
			u8  erase_grp_size;
			u8  erase_grp_mult;
		} v31;
		struct { /* MMC system specification version 2.2 */
			u8  sector_size;
			u8  erase_grp_size;
		} v22;
	} erase;
	u8  wp_grp_size;
	u8  wp_grp_enable;
	u8  default_ecc;
	u8  r2w_factor;
	u8  write_bl_len;
	u8  write_bl_partial;
	u8  file_format_grp;
	u8  copy;
	u8  perm_write_protect;
	u8  tmp_write_protect;
	u8  file_format;
	u8  ecc;
};

#define MMC_VDD_145_150	0x00000001	/* VDD voltage 1.45 - 1.50 */
#define MMC_VDD_150_155	0x00000002	/* VDD voltage 1.50 - 1.55 */
#define MMC_VDD_155_160	0x00000004	/* VDD voltage 1.55 - 1.60 */
#define MMC_VDD_160_165	0x00000008	/* VDD voltage 1.60 - 1.65 */
#define MMC_VDD_165_170	0x00000010	/* VDD voltage 1.65 - 1.70 */
#define MMC_VDD_17_18	0x00000020	/* VDD voltage 1.7 - 1.8 */
#define MMC_VDD_18_19	0x00000040	/* VDD voltage 1.8 - 1.9 */
#define MMC_VDD_19_20	0x00000080	/* VDD voltage 1.9 - 2.0 */
#define MMC_VDD_20_21	0x00000100	/* VDD voltage 2.0 ~ 2.1 */
#define MMC_VDD_21_22	0x00000200	/* VDD voltage 2.1 ~ 2.2 */
#define MMC_VDD_22_23	0x00000400	/* VDD voltage 2.2 ~ 2.3 */
#define MMC_VDD_23_24	0x00000800	/* VDD voltage 2.3 ~ 2.4 */
#define MMC_VDD_24_25	0x00001000	/* VDD voltage 2.4 ~ 2.5 */
#define MMC_VDD_25_26	0x00002000	/* VDD voltage 2.5 ~ 2.6 */
#define MMC_VDD_26_27	0x00004000	/* VDD voltage 2.6 ~ 2.7 */
#define MMC_VDD_27_28	0x00008000	/* VDD voltage 2.7 ~ 2.8 */
#define MMC_VDD_28_29	0x00010000	/* VDD voltage 2.8 ~ 2.9 */
#define MMC_VDD_29_30	0x00020000	/* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31	0x00040000	/* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_31_32	0x00080000	/* VDD voltage 3.1 ~ 3.2 */
#define MMC_VDD_32_33	0x00100000	/* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34	0x00200000	/* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_34_35	0x00400000	/* VDD voltage 3.4 ~ 3.5 */
#define MMC_VDD_35_36	0x00800000	/* VDD voltage 3.5 ~ 3.6 */
#define MMC_CARD_BUSY	0x80000000	/* Card Power up status bit */

/*
 * Card Command Classes (CCC)
 */
#define CCC_BASIC		(1<<0)	/* (0) Basic protocol functions */
					/* (CMD0,1,2,3,4,7,9,10,12,13,15) */
#define CCC_STREAM_READ		(1<<1)	/* (1) Stream read commands */
					/* (CMD11) */
#define CCC_BLOCK_READ		(1<<2)	/* (2) Block read commands */
					/* (CMD16,17,18) */
#define CCC_STREAM_WRITE	(1<<3)	/* (3) Stream write commands */
					/* (CMD20) */
#define CCC_BLOCK_WRITE		(1<<4)	/* (4) Block write commands */
					/* (CMD16,24,25,26,27) */
#define CCC_ERASE		(1<<5)	/* (5) Ability to erase blocks */
					/* (CMD32,33,34,35,36,37,38,39) */
#define CCC_WRITE_PROT		(1<<6)	/* (6) Able to write protect blocks */
					/* (CMD28,29,30) */
#define CCC_LOCK_CARD		(1<<7)	/* (7) Able to lock down card */
					/* (CMD16,CMD42) */
#define CCC_APP_SPEC		(1<<8)	/* (8) Application specific */
					/* (CMD55,56,57,ACMD*) */
#define CCC_IO_MODE		(1<<9)	/* (9) I/O mode */
					/* (CMD5,39,40,52,53) */
#define CCC_SWITCH		(1<<10)	/* (10) High speed switch */
					/* (CMD6,34,35,36,37,50) */
					/* (11) Reserved */
					/* (CMD?) */

/*
 * CSD field definitions
 */

#define CSD_STRUCT_VER_1_0  0           /* Valid for system specification 1.0 - 1.2 */
#define CSD_STRUCT_VER_1_1  1           /* Valid for system specification 1.4 - 2.2 */
#define CSD_STRUCT_VER_1_2  2           /* Valid for system specification 3.1       */

#define CSD_SPEC_VER_0      0           /* Implements system specification 1.0 - 1.2 */
#define CSD_SPEC_VER_1      1           /* Implements system specification 1.4 */
#define CSD_SPEC_VER_2      2           /* Implements system specification 2.0 - 2.2 */
#define CSD_SPEC_VER_3      3           /* Implements system specification 3.1 */



#define CLOCK_DETECT		(300*1024)	 /* Clock rate during detect */
#define CLOCK_DATA		(20*1024*1024)	 /* Clock rate during I/O */

#define MMC_CLKC_START_CLK	(1<<1)
#define MMC_CLKC_STOP_CLK	(1<<0)

#define MMC_STATUS_ENDRESP	(1<<13)
#define MMC_STATUS_DONE		(1<<12)
#define MMC_STATUS_TRANDONE	(1<<11)
#define MMC_STATUS_CLK_DIS	(1<<8)
#define MMC_STATUS_FIFO_FULL	(1<<7)
#define MMC_STATUS_FIFO_EMPTY	(1<<6)
#define MMC_STATUS_CRC		(1<<5)
#define MMC_STATUS_CRCREAD	(1<<3)
#define MMC_STATUS_CRCWRITE	(1<<2)
#define MMC_STATUS_TORES	(1<<1)
#define MMC_STATUS_TOREAD	(1<<0)

#define MMC_PREDIV_APB_RD_EN	(1<<5)
#define MMC_PREDIV_MMC_EN	(1<<4)
#define MMC_PREDIV_MMC_PREDIV_SHIFT	(0)
#define MMC_PREDIV_MMC_PREDIV_MASK	(0xf)

#define MMC_CMDCON_ABORT	(1<<13)
#define MMC_CMDCON_SET_READ_WRITE (1<<12)
#define MMC_CMDCON_MULTI_BLK4_INTEN (1<<11)
#define MMC_CMDCON_READ_WAIT_EN	(1<<10)
#define MMC_CMDCON_SDIO_EN	(1<<9)
#define MMC_CMDCON_BIG_ENDIAN	(1<<8)
#define MMC_CMDCON_WIDE		(1<<7)
#define MMC_CMDCON_INITIALIZE	(1<<6)
#define MMC_CMDCON_BUSY		(1<<5)
#define MMC_CMDCON_STREAM	(1<<4)
#define MMC_CMDCON_WRITE	(1<<3)
#define MMC_CMDCON_DATA_EN	(1<<2)
#define MMC_CMDCON_RESPONSE_FORMAT_SHIFT (0)
#define MMC_CMDCON_RESPONSE_FORMAT_MASK (0x3)

#define MMC_CMDCON_RESPONSE_NONE	(0 << MMC_CMDCON_RESPONSE_FORMAT_SHIFT)
#define MMC_CMDCON_RESPONSE_R1		(1 << MMC_CMDCON_RESPONSE_FORMAT_SHIFT)
#define MMC_CMDCON_RESPONSE_R2		(2 << MMC_CMDCON_RESPONSE_FORMAT_SHIFT)
#define MMC_CMDCON_RESPONSE_R3		(3 << MMC_CMDCON_RESPONSE_FORMAT_SHIFT)


#define MMC_RATE_IO_V		(0)
#define MMC_RATE_ID_V		(6)
#define MMC_PREDIV_V		(4)
//#define MMC_PREDIV_V		(8)
#define MMC_RES_TO_V		(0x7f)
#define MMC_READ_TO_V		(64)

#define MMC_WAIT		udelay(1)


struct mmc_info {
  char response[17];		/* Most recent response */
};

struct mmc_info mmc;

static const char* report_status (unsigned long l)
{
  static char sz[256];

  sprintf (sz, "[%04lx", l);
  if (l & MMC_STATUS_ENDRESP)
    strcat (sz, " ENDRESP");
  if (l & MMC_STATUS_DONE)
    strcat (sz, " DONE");
  if (l & MMC_STATUS_TRANDONE)
    strcat (sz, " TRANDONE");
  if (l & MMC_STATUS_CLK_DIS)
    strcat (sz, " CLK_DIS");
  if (l & MMC_STATUS_FIFO_FULL)
    strcat (sz, " FIFO_FULL");
  if (l & MMC_STATUS_FIFO_EMPTY)
    strcat (sz, " FIFO_EMPTY");
  if (l & MMC_STATUS_CRC)
    strcat (sz, " CRC");
  if (l & MMC_STATUS_CRCREAD)
    strcat (sz, " CRCREAD");
  if (l & MMC_STATUS_CRCWRITE)
    strcat (sz, " CRCWRITE");
  if (l & MMC_STATUS_TORES)
    strcat (sz, " TORES");
  if (l & MMC_STATUS_TOREAD)
    strcat (sz, " TOREAD");
  strcat (sz, "]");
  return sz;
}

static void start_clock (void)
{
  ENTRY ();
  if (!(MMC_STATUS & MMC_STATUS_CLK_DIS))
    return;

  MMC_CLKC = MMC_CLKC_START_CLK;
  MMC_WAIT;

  /* *** FIXME: may be helpful to implement a timeout check */
  while (MMC_STATUS & MMC_STATUS_CLK_DIS)
    MMC_WAIT;
}

static void stop_clock (void)
{
  ENTRY ();

  if (MMC_STATUS & MMC_STATUS_CLK_DIS)
    return;

  udelay (100);
  MMC_CLKC = MMC_CLKC_STOP_CLK;
  MMC_WAIT;

  /* *** FIXME: may be helpful to implement a timeout check */
  while (!(MMC_STATUS & MMC_STATUS_CLK_DIS))
    MMC_WAIT;
}

static void clear_fifo (void)
{
  unsigned long prediv = MMC_PREDIV;

  MMC_PREDIV |= MMC_PREDIV_APB_RD_EN;
  MMC_WAIT;
  while (!(MMC_STATUS & MMC_STATUS_FIFO_EMPTY)) {
    MMC_DATA_FIFO;
    MMC_WAIT;
  }

  MMC_PREDIV = prediv;
  MMC_WAIT;
}

static void clear_status (void)
{
  MMC_EOI = (1<<5)|(1<<2)|(1<<1)|(1<<0);
  MMC_WAIT;
}

/* pull_response

   retrieves a command response.  The length is the length of the
   expected response, in bits.

*/

static void pull_response (int length)
{
  int i;
  unsigned short result;

  length /= 8;

  printf ("response ");
  for (i = 0; i < length;) {
    unsigned short s = MMC_RES_FIFO;
    if (i == 0)
      result = s;
    mmc.response[i++] = s & 0xff;
    printf (" %02x", s & 0xff);
    if (i < length) {
      mmc.response[i++] = (s >> 8) & 0xff;
      printf (" %02x", (s >> 8) & 0xff);
    }
  }
  printf ("\n");
}

static unsigned short wait_for_completion (void)
{
  unsigned short status_last = 0;
  unsigned short status;

  udelay (1);
  do {
    MMC_WAIT;
    status = MMC_STATUS;
    MMC_WAIT;
    if (status != status_last)
      printf (" %x", status);
    status_last = status;
  } while ((status
	    & (  MMC_STATUS_ENDRESP
	       | MMC_STATUS_DONE
	       | MMC_STATUS_TRANDONE
//	       | MMC_STATUS_TORES
//	       | MMC_STATUS_TOREAD
	       | MMC_STATUS_CRC
	       | MMC_STATUS_CRCREAD
		 ))
	   == 0);
  printf ("wait: %s %lx\n", report_status (status), MMC_INT_STATUS);
  { int i; for (i = 0; i < 229743; i++); }
  stop_clock ();

  return status_last;
}

static void mmc_probe (void)
{
  unsigned short s;
  int tries;

  ENTRY ();

  stop_clock ();
  DBG (2, "CMDCON 0x%lx\n", MMC_CMDCON);

//  start_clock ();
//  udelay (1000);
//  MMC_CMDCON &= ~MMC_CMDCON_INITIALIZE;

//  MMC_CMDCON |= MMC_CMDCON_DATA_EN | MMC_CMDCON_BIG_ENDIAN;
#if 1
  MASK_AND_SET (MMC_CMDCON, MMC_CMDCON_RESPONSE_FORMAT_MASK,
		MMC_CMDCON_RESPONSE_NONE);
  MMC_WAIT;

  MMC_CMD = MMC_GO_IDLE_STATE;
  MMC_WAIT;
  MMC_ARGUMENT = 0;
  MMC_WAIT;
//  MMC_CMDCON = cmdcon;
  MMC_RATE = MMC_RATE_ID_V;
  MMC_WAIT;

  clear_fifo ();
  clear_status ();
  printf ("attempting to idle MMC (%s %lx)\n", 
	  report_status (MMC_STATUS), MMC_INT_STATUS);
  start_clock ();
  wait_for_completion ();
//  MMC_CMDCON &= ~MMC_CMDCON_INITIALIZE;
#endif

#if 1
  tries = 5;
 redo_op_cond:
  stop_clock ();

  //  MMC_CMDCON &= ~MMC_CMDCON_INITIALIZE;
  MMC_CMD = MMC_SEND_OP_COND;
  MMC_WAIT;
  MMC_ARGUMENT = 0 ; // 0x00ffc000;
  MMC_WAIT;
  MMC_RATE = MMC_RATE_ID_V;
  MMC_WAIT;
  MASK_AND_SET (MMC_CMDCON, MMC_CMDCON_RESPONSE_FORMAT_MASK,
		MMC_CMDCON_RESPONSE_R3);
  MMC_WAIT;
  
  clear_fifo ();
  clear_status ();
  printf ("attempting to op_cond MMC (%s %lx)\n", 
	  report_status (MMC_STATUS), MMC_INT_STATUS);
  start_clock ();
  wait_for_completion ();

  pull_response (48);

  if ((mmc.response[1] & 0x80) == 0 && tries--) {
    { int i; for (i = 0; i < 229743; i++); }
    udelay (1000);
    goto redo_op_cond;
  }

#endif  

  tries = 3;

 redo:
  stop_clock ();
  MMC_CMD = MMC_ALL_SEND_CID;
  MMC_WAIT;
  MMC_ARGUMENT = 0;
  MMC_WAIT;
  MMC_RATE = MMC_RATE_ID_V;
  MMC_WAIT;
  MASK_AND_SET (MMC_CMDCON, MMC_CMDCON_RESPONSE_FORMAT_MASK,
		MMC_CMDCON_RESPONSE_R2);
  MMC_WAIT;
  
  DBG (2, "CMDCON 0x%lx\n", MMC_CMDCON);

  clear_fifo ();
  clear_status ();
  printf ("attempting to all_send_cid MMC (%s %lx)\n", 
	  report_status (MMC_STATUS), MMC_INT_STATUS);
  start_clock ();
  s = wait_for_completion ();

  if ((s & MMC_STATUS_TORES) && tries--) {
    udelay (1000);
    goto redo;
  }

  if (!(s & MMC_STATUS_TORES))
    pull_response (136);
  
//  stop_clock ();
}

static void mmc_init (void)
{
  ENTRY ();

  GPIO_PFDD |= (1<<6);		/* Enable card detect interrupt pin */

  return;

  if (MMC_PREDIV & MMC_PREDIV_MMC_EN) {
    stop_clock ();
    MMC_PREDIV = 0;
    MMC_WAIT;
    udelay (1);
  }

  DBG (2, "%s: enabling MMC\n", __FUNCTION__);

  MMC_PREDIV = MMC_PREDIV_MMC_EN | MMC_PREDIV_V;
  MMC_WAIT;
  MMC_RATE = MMC_RATE_ID_V;
  MMC_WAIT;
  MMC_CMDCON = MMC_CMDCON_INITIALIZE; /* Init card */
  MMC_WAIT;
  MMC_RES_TO = MMC_RES_TO_V;
  MMC_WAIT;
  MMC_INT_MASK = 0x3f;
  MMC_WAIT;
  udelay (1);
  //  udelay (1);
  //  stop_clock ();
//  MASK_AND_SET (MMC_PREDIV, MMC_PREDIV_MMC_PREDIV_MASK, MMC_PREDIV_V);
//  MMC_READ_TO = MMC_READ_TO_V;

//  start_clock ();
//  udelay (1000);
//  MMC_CMDCON &= ~MMC_CMDCON_INITIALIZE;

  mmc_probe ();

}

static void mmc_report (void)
{
  printf ("  mmc:    %s\n", (GPIO_PFD & (1<<6)) ? "no card" : "card present");
}

static void mmc_setup (void)
{
  if (MMC_PREDIV & MMC_PREDIV_MMC_EN) {
    stop_clock ();
    MMC_PREDIV = 0;
    MMC_WAIT;
    udelay (1);
  }

  DBG (2, "%s: enabling MMC\n", __FUNCTION__);

  MMC_PREDIV = MMC_PREDIV_MMC_EN | MMC_PREDIV_V;
  MMC_WAIT;
  MMC_RATE = MMC_RATE_ID_V;
  MMC_WAIT;
  MMC_CMDCON = MMC_CMDCON_INITIALIZE; /* Init card */
  MMC_WAIT;
  MMC_RES_TO = MMC_RES_TO_V;
  MMC_WAIT;
  MMC_INT_MASK = 0x3f;
  MMC_WAIT;
  udelay (1);
  //  udelay (1);
  //  stop_clock ();
//  MASK_AND_SET (MMC_PREDIV, MMC_PREDIV_MMC_PREDIV_MASK, MMC_PREDIV_V);
//  MMC_READ_TO = MMC_READ_TO_V;

//  start_clock ();
//  udelay (1000);
//  MMC_CMDCON &= ~MMC_CMDCON_INITIALIZE;
}


static __driver_5 struct driver_d mmc_driver = {
  .name = "mmc-lh7a404",
  .description = "MMC/SD card driver",
  //  .open = memory_open,
  //  .close = close_helper,
  //  .read = memory_read,
  //  .write = memory_write,
  //  .seek = seek_helper,
};

static __service_6 struct service_d mmc_service = {
  .init = mmc_init,
#if !defined (CONFIG_SMALL)
  .report = mmc_report,
#endif
};


static int cmd_mmc (int argc, const char** argv)
{
  printf ("%s: card %s\n", __FUNCTION__,
	  (GPIO_PFD & (1<<6)) ? "not inserted" : "inserted");

  mmc_setup ();

  mmc_probe ();

  return 0;
}

static __command struct command_d c_mmc = {
  .command = "mmc",
  .description = "test MMC controller",
  .func = cmd_mmc,
};

