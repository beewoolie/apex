/** @file drv-mx5-ecspi.c

   written by Marc Singer
   12 Jul 2011

   Copyright (C) 2011 Marc Singer

   -----------
   DESCRIPTION
   -----------

   SS0
		/ * pmic * /
		dev->base = CSPI1_BASE_ADDR;
		dev->freq = 2500000;
		dev->ss_pol = IMX_SPI_ACTIVE_HIGH;
		dev->ss = 0;
		dev->fifo_sz = 64 * 4;
		dev->us_delay = 0;
		break;
   SS1
		/ * spi_nor * /
		dev->base = CSPI1_BASE_ADDR;
		dev->freq = 2500000;
		dev->ss_pol = IMX_SPI_ACTIVE_LOW;
		dev->ss = 1;
		dev->fifo_sz = 64 * 4;
		dev->us_delay = 0;
		break;

  NOTES
  =====

  SCLK Frequency
  --------------

  The SST documentation states that most commands should run at 80MHz.
  Only the standard read command is rated at 25MHz.  The base clock
  frequency for the eCSPI module is 56MHz.  Yet setting the dividers
  to 1 and communicating with the SPI flash part at 56MHz yields
  incorrect JEDEC IDs.  This could be a problem with the setup of the
  SPI port and it could be an electrical problem with the
  interconnect between the CPU and the SPI flash part.

  The correct JEDEC ID is 0xbf, 0x25, 0x41.  The value read at 56MHz
  is consistently 0x5f, 0x92, 0xa5.


  Transfer Size Limits
  --------------------

  There are two significant limits to SPI transfers.  There is a
  maximum size of the exchange s.t. the SS line is active for the
  duration.  This is determined by a field in the ECSPI control
  register.  There is also a limit based on the number of bytes in the
  FIFO.  Unless we implement DMA, we may want to use the known size of
  the FIFOs to batch IO with the SPI interface.  Without this
  optimization we write and read a single word in step.


  USE_FIFO
  --------

  Use of the FIFO to permit the bulk of the transfer to execute in the
  background.  A 1MiB transfer goes from 750ms to 550ms.  DMA would
  provide a cleaner solution.


  Transferring an Uneven Multiple of Bits
  ---------------------------------------

  Code is in place to deal with transfers of a multiple of bytes that
  isn't an even multiple of words.  If instead the transfer depends on
  an odd number of bits something more complicated will have to be
  done.  The eCSPI transfers the uneven number of bits in the first
  transaction.  It does so from the LSB of the word.  So, for
  instance, moving a single bit would send only the LSB of the word
  written to TXD.  This may be a non-obvious result and may
  necessitate some negotiation with the upper layer.

  I'm unable to compose a problem case at the momement.  We'll have to
  wait until there is a situation where the existing code fails to
  properly handle the transfer.


  Fast Writes
  -----------

  There are two methods for writing quickly to the flash array on the
  SST parts, for eliminating the BSY polling step.  We can either
  delay the pair of bytes by Tbp (10us), or we can configure SO as a
  status line and watch the logic level change.

  Because the HW busy line change requires a reconfiguration of the
  port we are going to focus on the timing-based solution. Even
  without these improvements, writing to the flash array isn't so
  inefficient to be a problem.

*/

#include <config.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <error.h>
#include <command.h>
#include <spinner.h>
#include <apex.h>

#include "hardware.h"
#include "mx5-spi.h"

#include <debug_ll.h>

#define TALK 1
//#define TALK 3
//#define NOISY                   /* Use when CFI not detecting properly */

#define USE_FIFO	/* Use FIFOs for more effcient IO */
//#define USE_AAI         /* Use AAI mode for programming */
//#define US_TBP		(10)    /* Delay between work program cycles */

#include <talk.h>

#define CB_FIFO	(64*4)          /* Size of each FIFO */
#define CBIT_BURST_MAX (1<<12)  /* Largest single transfer in bits */

static void mx5_ecspi_setup (const struct mx5_spi* spi, int bytes)
{
  u32 v;

  v = ECSPIX_CONTROL(spi->bus);
  ECSPIX_CONTROL(spi->bus) = 0;          /* Reset eCSPI module */
  ECSPIX_CONTROL(spi->bus) = v | 1;

  v = ECSPIX_CONFIG (spi->bus);
  MASK_AND_SET (v, 0
                | (1<<(20 + spi->slave))
                | (1<<(16 + spi->slave))
                | (1<<(12 + spi->slave))
                | (1<<( 8 + spi->slave))
                | (1<<( 4 + spi->slave))
                | (1<<( 0 + spi->slave))
                , 0
                | ((spi->sclk_inactive_high ? 0 : 1)<<(20 + spi->slave))
                | ((spi->data_inactive_high ? 0 : 1)<<(16 + spi->slave))
                | ((spi->ss_active_high     ? 1 : 0)<<(12 + spi->slave))
                | (0<<8)                                /* single burst */
                | ((spi->sclk_polarity      ? 1 : 0)<<( 4 + spi->slave))
                | ((spi->sclk_phase         ? 1 : 0)<<( 0 + spi->slave))
                );

  ECSPIX_CONFIG (spi->bus) = v;

  {
    u32 clock = cspi_clk ();
    int prediv;
    int postdiv = 0;
    while (clock/spi->sclk_frequency > 16 && postdiv < 0xf) {
      ++postdiv;
      clock >>= 1;
    }
    prediv = clock/spi->sclk_frequency;
    if (prediv > 16)
      prediv = 16;

    ECSPIX_CONTROL(spi->bus) = 0
      | ((bytes*8 - 1) << 20)
      | (prediv        << 12)
      | (postdiv       << 8)
      | (spi->slave    << 18)
      | (1             << (4 + spi->slave))
      ;
  }

  DBG(1, "ctrl %08lx [%ld bits]  cfg %08x\n",
      ECSPIX_CONTROL (spi->bus),
      (ECSPIX_CONTROL (spi->bus) >> 20) + 1,
      v);

  ECSPIX_INT(spi->bus) = 0;       /* Clear interrupt control */
  ECSPIX_STATUS(spi->bus) = 3<<6; /* Clear status */
}


/* simple-minded SPI transfer function.  The caller provides a single
   buffer for both the transmitted data as well as the received data.
   From this buffer, cbOut bytes are shifted out and sent to the SPI
   slave.  Of the bytes read from the SPI interface, the first
   cbInSkip are discarded and cbIn are written to the buffer starting
   at the beginning.  If cbIn + cbInSkip are greater than cbOut then
   zero bytes are transmitted until all of the requested bytes have
   been read.

   There is logic to handle an odd number of bytes being transfered.
   The eCSPI module uses the first word read and written to round the
   transfer size to an even number of words.  If the transfer is for
   40 bits it will send the 8 least-significant bits from the first
   word and all 32 from the second.  Similarly, the LSB 8 bits of the
   first word read contain the data read while transmitting the first
   word.

*/

void mx5_ecspi_transfer (const struct mx5_spi* spi,
                         void* rgb, size_t cbOut,
                         size_t cbIn, size_t cbInSkip)
{
  int cbTransfer;
  u8* rgbOut = (u8*) rgb;
  u8* rgbIn  = (u8*) rgb;
  int cbOutExtra = 0;

  if (cbIn + cbInSkip > cbOut)
    cbOutExtra = cbIn + cbInSkip - cbOut;

  cbTransfer = cbOut + cbOutExtra;
  DBG(3, "cbOut %d  cbOutExtra %d  cbIn %d  cbInSkip %d cbTrans %d\n",
      cbOut, cbOutExtra, cbIn, cbInSkip, cbTransfer);

  mx5_ecspi_setup (spi, cbTransfer);
  spi->select (spi);

  ECSPIX_CONTROL(spi->bus) |= 1;

  /* Clean out queue */
  while (ECSPIX_STATUS (spi->bus) & (1<<3))
    ECSPIX_RXD (spi->bus);
  ECSPIX_STATUS(spi->bus) = 3<<6;

  while ((cbTransfer = (cbOut + cbOutExtra)) > 0) {
    int i;
    int availableTx;
    int availableRx;
    int bytes;

#if defined (USE_FIFO)
    availableTx = cbOut + cbOutExtra;
    if (availableTx > CB_FIFO)
      availableTx = CB_FIFO;
#else
    /* I think it's OK that availableTx is 4 as this only reflects the
       number of bytes that *can* be read during this cycle.  The
       'bytes' value is used to control the number of bytes actually
       exchanged at one time in a single write/read with the eCSPI
       module. */
    availableTx = 4;
#endif

    availableRx = availableTx;

    DBG(3, "cbOut %d  cbOutExtra %d  cbIn %d\n",
        cbOut, cbOutExtra, cbIn);

    for (bytes = (cbTransfer & 3) ? (cbTransfer & 3) : 4;
         availableTx > 0; bytes = 4) {
      u32 v = 0;
      if (cbOut)
        for (i = bytes; i--; )
          if (cbOut) {
            v = (v << 8) | *rgbOut++;
            --cbOut;
          }
          else {
            v <<= 8;
            --cbOutExtra;
          }
      else
        cbOutExtra -= bytes;

      if (spi->talk)
        DBG(1, "send 0x%08x  st %lx\n", v, ECSPIX_STATUS (spi->bus));
      ECSPIX_TXD(spi->bus) = v;
      availableTx -= bytes;
    }

    ECSPIX_CONTROL(spi->bus) |= 1<<2; /* Initiate transfer  */
    //    while ((ECSPIX_STATUS (spi->bus) & (1<<7)) == 0)

    for (bytes = (cbTransfer & 3) ? (cbTransfer & 3) : 4;
         availableRx > 0; bytes = 4) {
      u32 status;
      u32 v = 0;
      while (((status = ECSPIX_STATUS (spi->bus)) & (1<<3)) == 0)
        ;
      v = ECSPIX_RXD(spi->bus);
      if (spi->talk)
        DBG(1, "recv 0x%08x  st %x\n", v, status);
      for (i = bytes; i--; ) {
        if (cbInSkip) {
          --cbInSkip;
          continue;
        }
        if (cbIn) {
          *rgbIn++ = (v >> (i*8)) & 0xff;
          --cbIn;
        }
        else
          break;
      }
      availableRx -= bytes;
    }

    ECSPIX_STATUS(spi->bus) = 3<<6;
  }
}
