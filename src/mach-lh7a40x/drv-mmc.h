/* mmc.h

   written by Marc Singer
   26 May 2006

   Copyright (C) 2006 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   Sharp MMC driver definitions.

*/

#if !defined (__DRV_MMC_H__)
#    define   __DRV_MMC_H__

/* ----- Includes */

#include "driver.h"

/* ----- Constants */

/* *** These clock values aren't used at the moment.  They should be
   held as upper limits on the rate and determined from HCLK and the
   predivisor. */
//#define CLOCK_DETECT		(300*1024)	 /* Clock rate during detect */
//#define CLOCK_DATA		(20*1024*1024)	 /* Clock rate during I/O */

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
#define MMC_STATUS_TIMED_OUT	(1<<16)	/* Synthetic status */

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

#define MS_ACQUIRE_DELAY	(10)


#define MMC_OCR_ARG_MAX		(0x00ffff00)


#define MMC_SECTOR_SIZE 512	/* *** FIXME: should come from card */

/* ----- Types */

struct mmc_info {
  char response[20];		/* Most recent response */
  char cid[16];			/* CID of acquired card  */
  char csd[16];			/* CSD of acquired card */
  int acquire_time;		/* Count of delays to acquire card */
  int cmdcon_sd;		/* cmdcon bits for data IO */
  int rca;			/* Relative address assigned to card */
  int acquired;                 /* Boolean for marking that card has been acquired */

  int c_size;
  int c_size_mult;
  int read_bl_len;
  int mult;
  int blocknr;
  int block_len;
  unsigned long device_size;

		/* *** FIXME: should be in .xbss section */
//  char rgb[512*2];		/* Sector buffer(s) */
  unsigned long ib;		/* Index of cached data */
};

/* ----- Globals */

extern struct mmc_info mmc;	/* Single, global context structure */


/* ----- Prototypes */

static inline int mmc_card_acquired (void) {
  return mmc.acquired != 0; }

void mmc_init (void);
void mmc_acquire (void);
ssize_t mmc_read (struct descriptor_d* d, void* pv, size_t cb);

#endif  /* __DRV_MMC_H__ */
