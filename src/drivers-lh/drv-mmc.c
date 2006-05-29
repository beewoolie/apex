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

   NOTES
   -----

   o Detection shoulw be continuous.
     o If the card doesn't respond to the known address, we can
       perform acqurire again.
     o This can be done when the report is generated as well.
     o Perhaps we acquire every time?
     o The only way to know we have the right card is to randomize the
       ID we set in MMC cards or...we have to acquire every time we
       start a transaction.
   o IO should really be quite easy, check for boundaries and read
     blocks as we do in the CF driver.
   o Enable SD mode when available.  There are two pieces, the
     controller and the card.  It looks like the WIDE bit is the one
     we need.  Not clear what SDIO_EN does.
   o Check for the best speed.  Some cards can handle 25MHz.  Can the
     core?  The documentation states that 20MHz is the maximum.  In
     that case, we should use a predivisor of 5 and we can vary the
     rate from 20MHz to 312KHz.

   -----------
   DESCRIPTION
   -----------

   Clocking
   --------

   The specification limits the clock frequency during identification
   to 400KHz.  During I/O, the limit is 20MHz due to the CPU core

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

   Block Caching
   -------------

   The driver caches one block from the card and copied data from the
   cached block to the caller's buffer.  This is done because the
   callers aren't expected to be efficient about reading large blocks
   of data.  This is a convenience for the upper layers as the command
   routines are best when they can handle data in the manner most
   efficient to their structure.

   Data FIFO Byte Ordering
   -----------------------

   The data FIFO is reading out in MSB order even though the
   BIG_ENDIAN_BIT isn't set in the CMD_CON register.

*/

#include <config.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <apex.h>
#include <command.h>
#include <mach/hardware.h>
#include <console.h>

#include <debug_ll.h>

#include <mmc.h>

//#define USE_BIGENDIAN_RESPONSE /* Old hardware */
#define USE_SD			/* Allow SD cards */
#define USE_WIDE		/* Allow WIDE mode */
//#define USE_SLOW_CLOCK		/* Slow the transfer clock to 12MHz */
//#define USE_WAYSLOW_CLOCK	/* Slow the transfer clock to 400KHz */
//#define USE_MMC_BOOTSTRAP	/* Allow MMC driver to be used in bootstrap */

#if defined (COMPANION)
# define GPIO_WP		PH1
#endif

#if defined (TROUNCER)
# define GPIO_CARD_DETECT	PF6
# define GPIO_WP		PC1
#else
/* Only trouncer supports it right now */
# undef USE_WIDE
#endif

#if defined USE_MMC_BOOTSTRAP
# define SECTION __section (.bootstrap)
# define SECTIOND __section (.bss_bootstrap)
#else
# define SECTION
# define SECTIOND
#endif


extern char* strcat (char*, const char*);

//#define TALK 1

#if defined (USE_MMC_BOOTSTRAP)
# undef TALK
#endif

#if defined (TALK)
#define PRINTF(f...)		printf (f)
#define ENTRY(l)		printf ("%s\n", __FUNCTION__)
#define DBG(l,f...)		do { if (TALK >= l) printf (f); } while (0)
#else
#define PRINTF(f...)		do {} while (0)
#define ENTRY(l)		do {} while (0)
#define DBG(l,f...)		do {} while (0)
#endif

struct mmc_info SECTIOND mmc;

#define MS_TIMEOUT 2000

#if defined (USE_MMC_BOOTSTRAP)

static void SECTION _memcpy (void* dest, const void* src, size_t cb)
{
  while (cb--)
    *(unsigned char*) dest++ = *(unsigned char*) src++;
}
#define memcpy _memcpy

static void SECTION _memset (void* pv, int v, size_t cb)
{
  while (cb--)
    *(unsigned char*) pv++ = v;
}
#undef memset
#define memset _memset

#endif

static inline void mmc_clear (void) {
  memset (&mmc, 0, sizeof (mmc));
  mmc.ib = -1; }

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

#if defined (TALK)
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
  if (l & MMC_STATUS_TIMED_OUT)
    strcat (sz, " TIMEDOUT");
  strcat (sz, "]");
  return sz;
}
#endif

static void SECTION start_clock (void)
{
  if (!(MMC_STATUS & MMC_STATUS_CLK_DIS))
    return;

  MMC_CLKC = MMC_CLKC_START_CLK;

  /* *** FIXME: may be good to implement a timeout check. */

//  while (MMC_STATUS & MMC_STATUS_CLK_DIS)
//    ;
}

static void SECTION stop_clock (void)
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


/* clear_all

   clears the FIFOs, response and data, and the interrupt status.
*/

static void SECTION clear_all (void)
{
  unsigned long prediv = MMC_PREDIV;
  int i;

  MMC_PREDIV |= MMC_PREDIV_APB_RD_EN;
  while (!(MMC_STATUS & MMC_STATUS_FIFO_EMPTY))
    MMC_DATA_FIFO;

  MMC_PREDIV = prediv;

  for (i = 16; i--; )
    MMC_RES_FIFO;

  /* Clear interrupt status */
//  MMC_EOI = (1<<5)|(1<<2)|(1<<1)|(1<<0);

}

static void SECTION set_clock (int speed)
{
  if (speed < 1024*1024) {
    /* 195KHz */
    MASK_AND_SET (MMC_PREDIV,
		  MMC_PREDIV_MMC_PREDIV_MASK<<MMC_PREDIV_MMC_PREDIV_SHIFT,
		  8<<MMC_PREDIV_MMC_PREDIV_SHIFT);
    MMC_RATE = 6;
  }

  if (speed >= 1024*1024) {
    /* 20MHz */
    MASK_AND_SET (MMC_PREDIV,
		  MMC_PREDIV_MMC_PREDIV_MASK<<MMC_PREDIV_MMC_PREDIV_SHIFT,
		  8<<MMC_PREDIV_MMC_PREDIV_SHIFT);
    MMC_RATE = 0;

#if defined (USE_SLOW_CLOCK)
    /* 12MHz */
    MASK_AND_SET (MMC_PREDIV,
		  MMC_PREDIV_MMC_PREDIV_MASK<<MMC_PREDIV_MMC_PREDIV_SHIFT,
		  6<<MMC_PREDIV_MMC_PREDIV_SHIFT);
    MMC_RATE = 0;
#endif
#if defined (USE_WAYSLOW_CLOCK)
    /* 195KHz */
    MASK_AND_SET (MMC_PREDIV,
		  MMC_PREDIV_MMC_PREDIV_MASK<<MMC_PREDIV_MMC_PREDIV_SHIFT,
		  8<<MMC_PREDIV_MMC_PREDIV_SHIFT);
    MMC_RATE = 6;
#endif
  }
}

/* pull_response

   retrieves a command response.  The length is the length of the
   expected response, in bits.

*/

static void SECTION pull_response (int length)
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

static unsigned long SECTION wait_for_completion (unsigned long bits)
{
  unsigned short status = 0;
#if defined (TALK) && TALK > 0
  unsigned short status_last = 0;
#endif
#if !defined (USE_MMC_BOOTSTRAP)
  unsigned long time_start = timer_read ();
#else
  int timeout_count = 100000;
#endif
  int timed_out = 0;

  DBG (3, " (%lx) ", bits);
  do {
    udelay (1);
    status = MMC_STATUS;
#if defined (TALK) && TALK > 0
    if (status != status_last)
      DBG (3, " %04x", status);
    status_last = status;
#endif

#if !defined (USE_MMC_BOOTSTRAP)
    if (timer_delta (time_start, timer_read ()) >= MS_TIMEOUT)
      timed_out = 1;
#else
    if (timeout_count-- <= 0)
      timed_out = 1;
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
	   == 0 && !timed_out);
  DBG (3, " => %s %lx\n", report_status (status), MMC_INT_STATUS);
  stop_clock ();

  if (timed_out)
    DBG (1, " timedout => %s %lx\n", report_status (status), MMC_INT_STATUS);

  return status | (timed_out ? MMC_STATUS_TIMED_OUT : 0);
}

#if defined (TALK) && ! defined (USE_MMC_BOOTSTRAP)
static void report_command (void)
{
  DBG (2, "cmd 0x%02lx (%03ld) arg 0x%08lx  cmdcon 0x%04lx"
       "  rate 0x%02lx/%04ld\n",
       MMC_CMD, MMC_CMD, MMC_ARGUMENT, MMC_CMDCON, MMC_PREDIV, MMC_RATE);
}
#else
# define report_command(v)
#endif

static unsigned short SECTION execute_command (unsigned long cmd,
					       unsigned long arg,
					       unsigned short wait_status)
{
  int state = (cmd & CMD_BIT_APP) ? 99 : 0;
  unsigned short s = 0;

  if (!wait_status)
    wait_status = MMC_STATUS_ENDRESP;

 top:
  stop_clock ();

  set_clock ((cmd & CMD_BIT_LS) ? 400*1024 : 20*1024*1024);

  if (s)
    DBG (3, "%s: state %d s 0x%x\n", __FUNCTION__, state, s);

  switch (state) {

  case 0:			/* Execute command */
    MMC_CMD = ((cmd & CMD_MASK_CMD) >> CMD_SHIFT_CMD);
    MMC_ARGUMENT = arg;
    MMC_CMDCON
      = ((cmd & CMD_MASK_RESP) >> CMD_SHIFT_RESP)
      | ((cmd & CMD_BIT_INIT)  ? MMC_CMDCON_INITIALIZE : 0)
      | ((cmd & CMD_BIT_BUSY)  ? MMC_CMDCON_BUSY       : 0)
      | ((cmd & CMD_BIT_DATA)  ? (MMC_CMDCON_DATA_EN
//				  | MMC_CMDCON_BIG_ENDIAN
#if defined (USE_WIDE)
				  | mmc.cmdcon_sd
#endif
				  ) : 0)
      | ((cmd & CMD_BIT_WRITE)  ? MMC_CMDCON_WRITE     : 0)
      | ((cmd & CMD_BIT_STREAM)	? MMC_CMDCON_STREAM    : 0)
      ;
    ++state;
    break;

  case 1:
    return s;

  case 99:			/* APP prefix */
    MMC_CMD = MMC_APP_CMD;
    MMC_ARGUMENT = mmc.rca<<16;
    MMC_CMDCON = 0
      | MMC_CMDCON_RESPONSE_R1	/* Response is status */
      | ((cmd & CMD_BIT_INIT) ? MMC_CMDCON_INITIALIZE : 0);
    state = 0;
    break;
  }

  clear_all ();
  report_command ();
  start_clock ();
  s = wait_for_completion (wait_status);

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

void SECTION mmc_acquire (void)
{
  unsigned short s;
  int tries = 0;
  unsigned long ocr = 0;
  unsigned long r;
  int state = 0;
  unsigned long command = 0;
  int cmdcon_sd = MMC_CMDCON_WIDE;

  mmc_clear ();

  stop_clock ();

//  PUTC_LL('I');
  s = execute_command (CMD_IDLE, 0, 0);
//  PUTC_LL('i');

  while (state < 100) {
//    PUTHEX_LL (state);
//    PUTC_LL (':');

    /* It would be great if I could use a switch statement here,
       especially because the compile is very efficient about
       converting it to a table.  However, the absolute PCs in the
       table make that impossible when we use this code in the
       bootstrap.  So, we're left with a cascading sequence of if's.
    */

//    switch (state) {

//    case 0:			/* Setup for SD */
    if (state == 0) {
      command = CMD_SD_OP_COND;
      tries = 10;		/* *** We're not sure we need to wait
				   for the READY bit to be clear, but
				   it should be in any case. */
      ++state;
//      break;
    }

    else if (state == 10) {
//    case 10:			/* Setup for MMC */
      command = CMD_MMC_OP_COND;
      tries = 10;
      cmdcon_sd = 0;
      ++state;
//      break;
    }

    else if (state == 1 || state == 11) {
//    case 1:
//    case 11:
      s = execute_command (command, 0, 0);
      if (s & MMC_STATUS_TORES)
	state += 8;		/* Mode unavailable */
      else
	++state;
//      break;
    }

    else if (state == 2 || state == 12) {
//    case 2:			/* Initial OCR check  */
//    case 12:
      ocr = response_ocr ();
      if (ocr & OCR_ALL_READY)
	++state;
      else
	state += 2;
//      break;
    }

    else if (state == 3 || state == 13) {
//    case 3:			/* Initial wait for OCR clear */
//    case 13:
      while ((ocr & OCR_ALL_READY) && --tries > 0) {
	mdelay (MS_ACQUIRE_DELAY);
	s = execute_command (command, 0, 0);
	ocr = response_ocr ();
      }
      if (ocr & OCR_ALL_READY)
	state += 6;
      else
	++state;
//      break;
    }

    else if (state == 4 || state == 14) {
//    case 4:			/* Assign OCR */
//    case 14:
      tries = 200;
      ocr &= 0x00ff8000;	/* Mask for the bits we care about */
      do {
	mdelay (MS_ACQUIRE_DELAY);
	mmc.acquire_time += MS_ACQUIRE_DELAY;
	s = execute_command (command, ocr, 0);
	r = response_ocr ();
      } while (!(r & OCR_ALL_READY) && --tries > 0);
      if (r & OCR_ALL_READY)
	++state;
      else
	state += 5;
//      break;
    }

    else if (state == 5 || state == 15) {
//    case 5:			/* CID polling */
//    case 15:
      mmc.cmdcon_sd = cmdcon_sd;
      s = execute_command (CMD_ALL_SEND_CID, 0, 0);
      memcpy (mmc.cid, mmc.response + 1, 16);
      ++state;
//      break;
    }

    else if (state == 6) {
//    case 6:			/* RCA send */
      s = execute_command (CMD_SD_SEND_RCA, 0, 0);
      mmc.rca = (mmc.response[1] << 8) | mmc.response[2];
      ++state;
//      break;
    }

    else if (state == 16) {
//    case 16:			/* RCA assignment */
      mmc.rca = 1;
      s = execute_command (CMD_MMC_SET_RCA, mmc.rca << 16, 0);
      ++state;
//      break;
    }

    else if (state == 7 || state == 17) {
//    case 7:
//    case 17:
      s = execute_command (CMD_SEND_CSD, mmc.rca << 16, 0);
      memcpy (mmc.csd, mmc.response + 1, 16);
      state = 100;
//      break;
    }

    else if (state == 9) {
//    case 9:
      ++state;			/* Continue with MMC */
//      break;
    }

    else if (state == 19 || state == 20) {
//    case 19:
//    case 20:
      state = 999;
//      break;			/* No cards */
    }
    else
      break;
  }

  if (mmc_card_acquired ()) {
    PUTC_LL ('A');
    mmc.c_size = csd_c_size ();
    mmc.c_size_mult = csd_c_size_mult ();
    mmc.read_bl_len = mmc.csd[5] & 0xf;
    mmc.mult = 1<<(mmc.c_size_mult + 2);
    mmc.block_len = 1<<mmc.read_bl_len;
    mmc.blocknr = (mmc.c_size + 1)*mmc.mult;
    mmc.device_size = mmc.blocknr*mmc.block_len;
  }
}

void SECTION mmc_init (void)
{
//  ENTRY ();

#if defined (MACH_TROUNCER)
  GPIO_PFDD |= (1<<6);		/* Enable card detect interrupt pin */
#endif

  MMC_PREDIV = 0;
#if defined (ARCH_LH7A400)
  MMC_SPI = 0;
  MMC_BUF_PART_FULL = 0;
#endif

  DBG (2, "%s: enabling MMC\n", __FUNCTION__);

  MMC_PREDIV = MMC_PREDIV_MMC_EN | MMC_PREDIV_APB_RD_EN
    | (1<<MMC_PREDIV_MMC_PREDIV_SHIFT);
  MMC_NOB = 1;
  MMC_INT_MASK = 0;		/* Mask all interrupts */
  MMC_EOI = 0x27;		/* Clear all interrupts */

  mmc_clear ();
  //  mmc_acquire ();
}

static void mmc_report (void)
{
#if defined (MACH_TROUNCER)
  printf ("  mmc:    %s\n", (GPIO_PFD & (1<<6)) ? "no card" : "card present");
#endif
  printf ("  mmc:    %s card acquired",
	  mmc_card_acquired () ? (mmc.cmdcon_sd ? "sd" : "mmc") : "no");
  if (mmc_card_acquired ()) {
    printf (", rca 0x%x (%d ms)", mmc.rca, mmc.acquire_time);
    printf (", %ld.%02ld MiB\n",
	    mmc.device_size/(1024*1024),
	    (((mmc.device_size/1024)%1024)*100)/1024);
//    dump (mmc.cid, 16, 0);
//    dump (mmc.csd, 16, 0);
//    printf ("\n");
  }
  else
    printf ("\n");
}

static int mmc_open (struct descriptor_d* d)
{
  DBG (2,"%s: opened %ld %ld\n", __FUNCTION__, d->start, d->length);
  return 0;
}


/* mmc_read

   performs the read of data from the SD/MMC card.  It handles
   unaligned, and sub-block length I/O.

*/

ssize_t SECTION mmc_read (struct descriptor_d* d, void* pv, size_t cb)
{
  ssize_t cbRead = 0;
  unsigned short s;

#if 1
  PUTHEX_LL (d->index);
  PUTC_LL ('/');
  PUTHEX_LL (d->start);
  PUTC_LL ('/');
  PUTHEX_LL (d->length);
  PUTC_LL ('/');
  PUTHEX_LL (cb);
  PUTC_LL ('\r');
  PUTC_LL ('\n');
#endif

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  //  DBG (1, "%s: %ld %ld %d; %ld\n",
  //       __FUNCTION__, d->start + d->index, d->length, cb, mmc.ib);

 {
   unsigned long sp;
   __asm volatile ("mov %0, sp\n\t"
		   : "=r" (sp));
   PUTHEX_LL (sp);
 }

  while (cb) {
    unsigned long index = d->start + d->index;
    int availableMax = MMC_SECTOR_SIZE - (index & (MMC_SECTOR_SIZE - 1));
    int available = cb;

#if 0
    PUTC_LL ('X');
    PUTHEX_LL (index);
    PUTC_LL ('/');
    PUTHEX_LL (cbRead);
    PUTC_LL ('\r');
    PUTC_LL ('\n');
#endif

    if (available > availableMax)
      available = availableMax;

    //    DBG (1, "%ld %ld\n", mmc.ib, index);

    if (mmc.ib == -1
	|| mmc.ib >= index  + MMC_SECTOR_SIZE
	|| index  >= mmc.ib + MMC_SECTOR_SIZE) {

      mmc.ib = index & ~(MMC_SECTOR_SIZE - 1);

      DBG (1, "%s: read av %d  avM %d  ind %ld  cb %d\n", __FUNCTION__,
	   available, availableMax, index, cb);

      execute_command (CMD_SELECT_CARD, 0, 0);	      /* Deselect */
      execute_command (CMD_SELECT_CARD, mmc.rca<<16, 0);  /* Select card */

#if defined (USE_WIDE)
      if (mmc.cmdcon_sd)
	execute_command (CMD_SD_SET_WIDTH, 2, 0);	/* SD, 4 bit width */
#endif
      execute_command (CMD_SET_BLOCKLEN, MMC_SECTOR_SIZE, 0);
      MMC_BLK_LEN = MMC_SECTOR_SIZE;
      s = execute_command (CMD_READ_SINGLE, mmc.ib,
			       MMC_STATUS_TRANDONE
			       | MMC_STATUS_FIFO_FULL
			       | MMC_STATUS_TOREAD);	/* Perform read */

//  DBG (1, "issued read\n");
//      start_clock ();

//  DBG (1, "waiting for status\n");

//      s = wait_for_completion ();

      //      printf ("status 0x%x\n", s);

	/* Pull data from FIFO */
      {
	int i;
	for (i = 0; i < MMC_SECTOR_SIZE; ) {
	  unsigned short v = MMC_DATA_FIFO;
	  mmc.rgb[i++] = (v >> 8) & 0xff;
	  mmc.rgb[i++] = v & 0xff;
	}
      }
    }

    memcpy (pv, mmc.rgb + (index & (MMC_SECTOR_SIZE - 1)), available);
    d->index += available;
    cb -= available;
    cbRead += available;
    pv += available;
  }

  return cbRead;
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

  mmc_init ();
  mmc_acquire ();

  return 0;
}

static __command struct command_d c_mmc = {
  .command = "mmc",
  .description = "test MMC controller",
  .func = cmd_mmc,
};
