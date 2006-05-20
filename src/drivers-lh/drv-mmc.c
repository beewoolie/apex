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

   Clocking
   --------

   The specification limits the clock frequency during identification
   to 400KHz.  During I/O, the limit is 20MHz.

   Responses Byte Ordering
   -----------------------

   On old hardware, rev -01 of the LH7A400, the response FIFO
   half-word is byte swapped.

      15     8 7      0
     +--------+--------+
     | BYTE 0 | BYTE 1 |	// Old hardware, MSB default
     +--------+--------+

   The correct ordering puts the first byte in the low order bits.

      15     8 7      0
     +--------+--------+
     | BYTE 1 | BYTE 0 |	// Correct layout, LSB default
     +--------+--------+


   Response Bytes
   --------------

   The R3 type (and probably R1) emit their responses with a null byte
   at the start.  This means that the first byte of the response
   should be ignored and the real response data starts at byte one.

   *** However, the standalone code has no such thing.  They skip a
   *** single byte, which is the header of the response, when they
   *** convert it to an OCR value.  Finding out why may be
   *** interesting.

   OCR_MAX
   -------

   The controller ought to be able to handle the full breadth of
   voltages for the SD/MMC spec.  However, we have some cards that
   either misreport their OCR, or don't play well.  So, the normal
   default spread of voltages, 0x00ffff80 which is used in some of the
   sample code, is reduced to 0x00ff0000 which seems to work well for
   everyone.  The SD cards tested all use that spread in any case.

*/

#include <config.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <apex.h>
#include <command.h>
#include <mach/hardware.h>
#include <console.h>

//#define USE_BIGENDIAN_RESPONSE /* Old hardware */
#define USE_SD

#if defined (COMPANION)
# define GPIO_WP		PH1
#endif

#if defined (TROUNCER)
# define GPIO_CARD_DETECT	PF6
# define GPIO_WP		PC1
#endif

extern char* strcat (char*, const char*);

#define TALK 3

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
#define MMC_SEND_OP_COND          1   /* bcr  [31:0]  OCR        R3  */
#define MMC_ALL_SEND_CID          2   /* bcr                     R2  */
#define MMC_SET_RELATIVE_ADDR     3   /* ac   [31:16] RCA        R1  */
#define MMC_SET_DSR               4   /* bc   [31:16] RCA            */
#define MMC_SELECT_CARD           7   /* ac   [31:16] RCA        R1  */
#define MMC_SEND_CSD              9   /* ac   [31:16] RCA        R2  */
#define MMC_SEND_CID             10   /* ac   [31:16] RCA        R2  */
#define MMC_READ_DAT_UNTIL_STOP  11   /* adtc [31:0]  dadr       R1  */
#define MMC_STOP_TRANSMISSION    12   /* ac                      R1b */
#define MMC_SEND_STATUS	         13   /* ac   [31:16] RCA        R1  */
#define MMC_GO_INACTIVE_STATE    15   /* ac   [31:16] RCA            */

  /* class 2 */
#define MMC_SET_BLOCKLEN         16   /* ac   [31:0]  block len  R1  */
#define MMC_READ_SINGLE_BLOCK    17   /* adtc [31:0]  data addr  R1  */
#define MMC_READ_MULTIPLE_BLOCK  18   /* adtc [31:0]  data addr  R1  */

  /* class 3 */
#define MMC_WRITE_DAT_UNTIL_STOP 20   /* adtc [31:0]  data addr  R1  */

  /* class 4 */
#define MMC_SET_BLOCK_COUNT      23   /* adtc [31:0]  data addr  R1  */
#define MMC_WRITE_BLOCK          24   /* adtc [31:0]  data addr  R1  */
#define MMC_WRITE_MULTIPLE_BLOCK 25   /* adtc                    R1  */
#define MMC_PROGRAM_CID          26   /* adtc                    R1  */
#define MMC_PROGRAM_CSD          27   /* adtc                    R1  */

  /* class 6 */
#define MMC_SET_WRITE_PROT       28   /* ac   [31:0]  data addr  R1b */
#define MMC_CLR_WRITE_PROT       29   /* ac   [31:0]  data addr  R1b */
#define MMC_SEND_WRITE_PROT      30   /* adtc [31:0]  wpdata addr R1  */

  /* class 5 */
#define MMC_ERASE_GROUP_START    35   /* ac   [31:0]  data addr  R1  */
#define MMC_ERASE_GROUP_END      36   /* ac   [31:0]  data addr  R1  */
#define MMC_ERASE                37   /* ac                      R1b */

  /* class 9 */
#define MMC_FAST_IO              39   /* ac   <Complex>          R4  */
#define MMC_GO_IRQ_STATE         40   /* bcr                     R5  */

  /* class 7 */
#define MMC_LOCK_UNLOCK          42   /* adtc                    R1b */

  /* class 8 */
#define MMC_APP_CMD              55   /* ac   [31:16] RCA        R1  */
#define MMC_GEN_CMD              56   /* adtc [0]     RD/WR      R1b */

/* SD commands                           type  argument     response */
  /* class 8 */
/* This is basically the same command as for MMC with some quirks. */
#define SD_SEND_RELATIVE_ADDR     3   /* ac                      R6  */

  /* Application commands */
#define SD_APP_SET_BUS_WIDTH      6   /* ac   [1:0]   bus width  R1	  */
#define SD_APP_OP_COND           41   /* bcr  [31:0]  OCR        R1 (R4)  */
#define SD_APP_SEND_SCR          51   /* adtc                    R1	  */

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
#define R1_CURRENT_STATE(x)	((x & 0x00001E00) >> 9)	/* sx, b (4 bits) */
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
#define OCR_ALL_READY	0x80000000	/* Card Power up status bit */

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

#define CSD_STRUCT_VER_1_0  0           /* Valid for system spec 1.0 - 1.2 */
#define CSD_STRUCT_VER_1_1  1           /* Valid for system spec 1.4 - 2.2 */
#define CSD_STRUCT_VER_1_2  2           /* Valid for system spec 3.1       */

#define CSD_SPEC_VER_0      0           /* Implements system spec 1.0 - 1.2 */
#define CSD_SPEC_VER_1      1           /* Implements system spec 1.4 */
#define CSD_SPEC_VER_2      2           /* Implements system spec 2.0 - 2.2 */
#define CSD_SPEC_VER_3      3           /* Implements system spec 3.1 */



/* *** These clock values aren't used at the moment.  They should be
   held as upper limits on the rate and determined from HCLK and the
   predivisor. */
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

#define MMC_CMDCON_RESPONSE_NONE (0 << MMC_CMDCON_RESPONSE_FORMAT_SHIFT) //   0
#define MMC_CMDCON_RESPONSE_R1	 (1 << MMC_CMDCON_RESPONSE_FORMAT_SHIFT) //  48
#define MMC_CMDCON_RESPONSE_R2	 (2 << MMC_CMDCON_RESPONSE_FORMAT_SHIFT) // 136
#define MMC_CMDCON_RESPONSE_R3	 (3 << MMC_CMDCON_RESPONSE_FORMAT_SHIFT) //  48

/*  HCLK is usually 99993600 */

#define MMC_RATE_IO_V		(0)			/* 0 -> MCLK/1  */
#define MMC_RATE_ID_V		(6)			/* 6 -> MCLK/64 */
//#define MMC_RATE_ID_V		(5)			/* 5 -> MCLK/32 */
//#define MMC_PREDIV_V		(4)
#define MMC_PREDIV_V		(8)			/* HCLK/N */
#define MMC_RES_TO_V		(0x7f)
//#define MMC_RES_TO_V		(64)
//#define MMC_READ_TO_V		(0x7fff)
#define MMC_READ_TO_V		(0xffff)

inline void mdelay (int c) {
  while (c-- > 0) udelay (1000); }
#define MS_ACQUIRE_DELAY	(10)


#define MMC_OCR_ARG_MAX		(0x00ffff00)

#define CMD_BIT_APP		 (1<<23)
#define CMD_BIT_INIT		 (1<<22)
#define CMD_BIT_BUSY		 (1<<21)
#define CMD_BIT_LS		 (1<<20) /* Low speed, used during acquire */
#define CMD_BIT_DATA		 (1<<19)
#define CMD_BIT_WRITE		 (1<<18)
#define CMD_BIT_STREAM		 (1<<17)
#define CMD_MASK_RESP		 (3<<24)
#define CMD_SHIFT_RESP		 (24)
#define CMD_MASK_CMD		 (0xff)
#define CMD_SHIFT_CMD		 (0)

#define CMD(c,r)		(  ((c) &  CMD_MASK_CMD)\
				 | ((r) << CMD_SHIFT_RESP)\
				 )

#define CMD_IDLE	 CMD(MMC_GO_IDLE_STATE,0) | CMD_BIT_LS	 | CMD_BIT_INIT
#define CMD_SD_OP_COND	 CMD(SD_APP_OP_COND,1)      | CMD_BIT_LS | CMD_BIT_APP
#define CMD_MMC_OP_COND	 CMD(MMC_SEND_OP_COND,3)    | CMD_BIT_LS | CMD_BIT_INIT
#define CMD_ALL_SEND_CID CMD(MMC_ALL_SEND_CID,2)    | CMD_BIT_LS
#define CMD_MMC_SET_RCA	 CMD(MMC_SET_RELATIVE_ADDR,1) | CMD_BIT_LS
#define CMD_SD_SEND_RCA	 CMD(SD_SEND_RELATIVE_ADDR,1) | CMD_BIT_LS
#define CMD_SEND_CSD	 CMD(MMC_SEND_CSD,2)
#define CMD_SELECT_CARD	 CMD(MMC_SELECT_CARD,1)
#define CMD_SET_BLOCKLEN CMD(MMC_SET_BLOCKLEN,1)
#define CMD_READ_SINGLE  CMD(MMC_READ_SINGLE_BLOCK,1) | CMD_BIT_DATA
#define CMD_READ_MULTIPLE CMD(MMC_READ_MULTIPLE_BLOCK,1)

struct mmc_info {
  char response[20];		/* Most recent response */
  char cid[16];			/* CID of acquired card  */
  char csd[16];			/* CSD of acquired card */
  int acquire_time;		/* Count of delays to acquire card */
  int sd;			/* SD card detected */
  int rca;			/* Relative address assigned to card */

  int c_size;
  int c_size_mult;
  int read_bl_len;
  int mult;
  int blocknr;
  int block_len;
  unsigned long device_size;
};

struct mmc_info mmc;

static inline int card_acquired (void) {
  return mmc.cid[0] != 0; }

static void mmc_report (void);

static inline unsigned long response_ocr (void) {
  return (((unsigned long) mmc.response[1]) << 24)
    | (((unsigned long) mmc.response[2]) << 16)
    | (((unsigned long) mmc.response[3]) << 8)
    | (((unsigned long) mmc.response[4]) << 0);
}

static inline unsigned long csd_c_size (void) {
  int v = (mmc.csd[6]<<16) | (mmc.csd[7]<<8) | mmc.csd[8];
  return (v>>6) & 0xfff;
}

static inline unsigned long csd_c_size_mult (void) {
  int v = (mmc.csd[9]<<8) | mmc.csd[10];
  return (v>>7) & 0x7;
}


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
    strcat (sz, " CRCRESP");
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
  if (!(MMC_STATUS & MMC_STATUS_CLK_DIS))
    return;

  MMC_CLKC = MMC_CLKC_START_CLK;

  /* *** FIXME: may be good to implement a timeout check. */

//  while (MMC_STATUS & MMC_STATUS_CLK_DIS)
//    ;
}

static void stop_clock (void)
{
  if (MMC_STATUS & MMC_STATUS_CLK_DIS)
    return;

  MMC_CLKC = MMC_CLKC_STOP_CLK;

  /* *** FIXME: may be helpful to implement a timeout check.
     Interestingly, the Sharp implementation of this function doesn't
     have a timeout. */
  while (!(MMC_STATUS & MMC_STATUS_CLK_DIS))
    udelay (1);
}

static void clear_fifo (void)
{
  unsigned long prediv = MMC_PREDIV;

  MMC_PREDIV |= MMC_PREDIV_APB_RD_EN;
  while (!(MMC_STATUS & MMC_STATUS_FIFO_EMPTY))
    MMC_DATA_FIFO;

  MMC_PREDIV = prediv;
}

static void clear_response_fifo (void)
{
  int i;

  for (i = 16; i--; )
    MMC_RES_FIFO;
}

static void clear_status (void)
{
//  MMC_EOI = (1<<5)|(1<<2)|(1<<1)|(1<<0);
}


/* pull_response

   retrieves a command response.  The length is the length of the
   expected response, in bits.

*/

static void pull_response (int length)
{
  int i;
//  int c = 16;
  int c = (length + 7)/8;

  DBG (3, "%s: %d-%d  ", __FUNCTION__, length, c);

  for (i = 0; i < c; ) {
    unsigned short s = MMC_RES_FIFO;

#if defined (USE_BIGENDIAN_RESPONSE)
    mmc.response[i++] = (s >> 8) & 0xff;
    if (i < c)
      mmc.response[i++] = s & 0xff;
#else
    mmc.response[i++] = s & 0xff;
    if (i < c)
      mmc.response[i++] = (s >> 8) & 0xff;
#endif
    DBG (3, " %02x %02x", mmc.response[i-2], mmc.response[i-1]);
  }

  DBG (3, "\n");
}

static unsigned short wait_for_completion (unsigned long bits)
{
  unsigned short status = 0;
#if defined (TALK) && TALK > 0
  unsigned short status_last = 0;
#endif

  DBG (3, " (%lx) ", bits);
  do {
    udelay (1);
    status = MMC_STATUS;
#if defined (TALK) && TALK > 0
    if (status != status_last)
      DBG (3, " %04x", status);
    status_last = status;
#endif
  } while ((status
	    & ( bits
//  MMC_STATUS_ENDRESP
//	       | MMC_STATUS_DONE
//	       | MMC_STATUS_TRANDONE
//	       | MMC_STATUS_TORES
//	       | MMC_STATUS_TOREAD
//	       | MMC_STATUS_CRC
//	       | MMC_STATUS_CRCREAD
		 ))
	   == 0);
  DBG (3, " => %s %lx\n", report_status (status), MMC_INT_STATUS);
  stop_clock ();

  return status;
}

static void report_command (void)
{
  DBG (2, "cmd 0x%02lx (%03ld) arg 0x%08lx  cmdcon 0x%04lx"
       "  rate 0x%02lx/%04ld\n",
       MMC_CMD, MMC_CMD, MMC_ARGUMENT, MMC_CMDCON, MMC_PREDIV, MMC_RATE);
}


static unsigned short execute_command (unsigned long cmd, unsigned long arg)
{
  int state = (cmd & CMD_BIT_APP) ? 99 : 0;
  unsigned short s = 0;

 top:
  stop_clock ();

  MMC_RATE = (cmd & CMD_BIT_LS) ? MMC_RATE_ID_V: MMC_RATE_IO_V;

  if (s)
    DBG (3, "%s: state %d s 0x%x\n", __FUNCTION__, state, s);

  switch (state) {

  case 0:			/* Execute command */
    MMC_CMD = ((cmd & CMD_MASK_CMD) >> CMD_SHIFT_CMD);
    MMC_ARGUMENT = arg;
    MMC_CMDCON
      = ((cmd & CMD_MASK_RESP) >> CMD_SHIFT_RESP)
      | ((cmd & CMD_BIT_INIT)	? MMC_CMDCON_INITIALIZE : 0)
      | ((cmd & CMD_BIT_BUSY)	? MMC_CMDCON_BUSY	: 0)
      | ((cmd & CMD_BIT_DATA)	? MMC_CMDCON_DATA_EN	: 0)
      | ((cmd & CMD_BIT_WRITE)  ? MMC_CMDCON_WRITE	: 0)
      | ((cmd & CMD_BIT_STREAM)	? MMC_CMDCON_STREAM	: 0)
      ;
    ++state;
    break;

  case 1:
    return s;

  case 99:			/* APP prefix */
    MMC_CMD = MMC_APP_CMD;
    MMC_ARGUMENT = 0;
    MMC_CMDCON = 0
      | MMC_CMDCON_RESPONSE_R1	/* Response is status */
      | ((cmd & CMD_BIT_INIT) ? MMC_CMDCON_INITIALIZE : 0);
    state = 0;
    break;
  }

//  clear_fifo ();
//  clear_response_fifo ();
  clear_status ();
  report_command ();
  start_clock ();
  s = wait_for_completion (MMC_STATUS_ENDRESP);

  /* We return an error if there is a timeout, even if we've fetched a
     response */
  if (s & MMC_STATUS_TORES)
    return s;

  if (s & MMC_STATUS_ENDRESP)
    switch (MMC_CMDCON & 0x3) {
    case 0:
      break;
    case 1:
    case 3:
      pull_response (48);
      break;
    case 2:
      pull_response (136);
      break;
    }

  goto top;
}


/* mmc_acquire

   detects cards on the bus and initializes them for IO.  It can
   detect both SD and MMC card types.

   It will only detect a single card and the first one will be the one
   that is configured with an RCA and will be used by the driver.

*/

static void mmc_acquire (void)
{
  unsigned short s;
  int tries = 0;
  unsigned long ocr = 0;
  unsigned long r;
  int state = 0;
  unsigned long command = 0;
  int sd = 1;

  mmc.acquire_time = 0;
  memset (mmc.cid, 0, sizeof (mmc.cid));

  stop_clock ();

//  MMC_CMDCON |= MMC_CMDCON_SDIO_EN;
//  MMC_CMDCON |= MMC_CMDCON_WIDE;

  s = execute_command (CMD_IDLE, 0);

  while (state < 100) {
    switch (state) {

    case 0:			/* Setup for SD */
      command = CMD_SD_OP_COND;
      tries = 10;		/* *** We're not sure we need to wait
				   for the READY bit to be clear, but
				   it should be in any case. */
      ++state;
      break;

    case 10:			/* Setup for MMC */
      command = CMD_MMC_OP_COND;
      tries = 10;
      sd = 0;
      ++state;
      break;

    case 1:
    case 11:
      s = execute_command (command, 0);
      if (s & MMC_STATUS_TORES)
	state += 8;		/* Mode unavailable */
      else
	++state;
      break;

    case 2:			/* Initial OCR check  */
    case 12:
      ocr = response_ocr ();
      if (ocr & OCR_ALL_READY)
	++state;
      else
	state += 2;
      break;

    case 3:			/* Initial wait for OCR clear */
    case 13:
      while ((ocr & OCR_ALL_READY) && --tries > 0) {
	mdelay (MS_ACQUIRE_DELAY);
	s = execute_command (command, 0);
	ocr = response_ocr ();
      }
      if (ocr & OCR_ALL_READY)
	state += 6;
      else
	++state;
      break;

    case 4:			/* Assign OCR */
    case 14:
      tries = 200;
      ocr &= 0x00ff8000;	/* Mask for the bits we care about */
      do {
	mdelay (MS_ACQUIRE_DELAY);
	mmc.acquire_time += MS_ACQUIRE_DELAY;
	s = execute_command (command, ocr);
	r = response_ocr ();
      } while (!(r & OCR_ALL_READY) && --tries > 0);
      if (r & OCR_ALL_READY)
	++state;
      else
	state += 5;
      break;

    case 5:			/* CID polling */
    case 15:
      mmc.sd = sd;
      s = execute_command (CMD_ALL_SEND_CID, 0);
      memcpy (mmc.cid, mmc.response + 1, 16);
      ++state;
      break;

    case 6:			/* RCA send */
      s = execute_command (CMD_SD_SEND_RCA, 0);
      mmc.rca = (mmc.response[1] << 8) | mmc.response[2];
      ++state;
      break;

    case 16:			/* RCA assignment */
      mmc.rca = 1;
      s = execute_command (CMD_MMC_SET_RCA, mmc.rca << 16);
      ++state;
      break;

    case 7:
    case 17:
      s = execute_command (CMD_SEND_CSD, mmc.rca << 16);
      memcpy (mmc.csd, mmc.response + 1, 16);
      state = 100;
      break;

    case 9:
      ++state;			/* Continue with MMC */
      break;

    case 19:
    case 20:
      state = 999;
      break;			/* No cards */
    }
  }

  if (card_acquired ()) {
    mmc.c_size = csd_c_size ();
    mmc.c_size_mult = csd_c_size_mult ();
    mmc.read_bl_len = mmc.csd[5] & 0xf;
    mmc.mult = 1<<(mmc.c_size_mult + 2);
    mmc.block_len = 1<<mmc.read_bl_len;
    mmc.blocknr = (mmc.c_size + 1)*mmc.mult;
    mmc.device_size = mmc.blocknr*mmc.block_len;
    mmc_report ();
  }
}

static void mmc_init (void)
{
//  ENTRY ();

#if defined (MACH_TROUNCER)
  GPIO_PFDD |= (1<<6);		/* Enable card detect interrupt pin */
  return;
#endif

  MMC_PREDIV = 0;
#if defined (ARCH_LH7A400)
  MMC_SPI = 0;
  MMC_BUF_PART_FULL = 0;
#endif

  DBG (2, "%s: enabling MMC\n", __FUNCTION__);

  MMC_PREDIV = MMC_PREDIV_MMC_EN | MMC_PREDIV_APB_RD_EN | MMC_PREDIV_V;
  MMC_RATE   = MMC_RATE_ID_V;
  MMC_RES_TO = MMC_RES_TO_V;
  MMC_NOB = 1;
  MMC_INT_MASK = 0;		/* Mask all interrupts */
  MMC_EOI = 0x27;		/* Clear all interrupts */

  mmc_acquire ();
}

static void mmc_report (void)
{
#if defined (MACH_TROUNCER)
  printf ("  mmc:    %s\n", (GPIO_PFD & (1<<6)) ? "no card" : "card present");
#endif
  printf ("  mmc:    %s card acquired",
	  card_acquired () ? (mmc.sd ? "sd" : "mmc") : "no");
  if (card_acquired ()) {
    printf (", rca 0x%x (%d ms)", mmc.rca, mmc.acquire_time);
    printf (", %ld.%02ld MiB",
	    mmc.device_size/(1024*1024),
	    (((mmc.device_size/1024)%1024)*100)/1024);
  }
  printf ("\n");
}

static int mmc_open (struct descriptor_d* d)
{
  return 0;
}

static ssize_t mmc_read (struct descriptor_d* d, void* pv, size_t cb)
{
  int ib = 0;
  char rgb[512];
  unsigned short s;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  DBG (1, "%s: %d %ld %d\n", __FUNCTION__, d->index, d->length, cb);

  execute_command (CMD_SELECT_CARD, mmc.rca<<16);
  execute_command (CMD_SET_BLOCKLEN, 512);
  execute_command (CMD_READ_SINGLE, d->index);

  DBG (1, "issued read\n");
  start_clock ();

  DBG (1, "waiting for status\n");

  s = wait_for_completion (MMC_STATUS_TRANDONE
			   | MMC_STATUS_FIFO_FULL
			   | MMC_STATUS_TOREAD);

  printf ("status 0x%x\n", s);
  {
    int i;
    for (i = 0; i < 512; ) {
      unsigned short v = MMC_DATA_FIFO;
      rgb[i++] = (v >> 8) & 0xff;
      rgb[i++] = v & 0xff;
    }
  }

  dump (rgb, 512, 0);

  return 512;
}

static __driver_5 struct driver_d mmc_driver = {
  .name = "mmc-lh7a404",
  .description = "MMC/SD card driver",
  .open = mmc_open,
  .close = close_helper,
  .read = mmc_read,
  //  .write = memory_write,
  .seek = seek_helper,
};

static __service_6 struct service_d mmc_service = {
  .init = mmc_init,
#if !defined (CONFIG_SMALL)
  .report = mmc_report,
#endif
};


static int cmd_mmc (int argc, const char** argv)
{
#if defined (MACH_TROUNCER)
  printf ("%s: card %s\n", __FUNCTION__,
	  (GPIO_PFD & (1<<6)) ? "not inserted" : "inserted");
#endif

  mmc_acquire ();

  return 0;
}

static __command struct command_d c_mmc = {
  .command = "mmc",
  .description = "test MMC controller",
  .func = cmd_mmc,
};
