/** @file drv-imx5-ecspi.c

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


enum {
  Read      = 0x03,
  ReadHS    = 0x08,
  Erase4k   = 0x20,
  Erase32k  = 0x52,
  Erase64k  = 0xd8,
  EraseChip = 0x60,
  ByteProg  = 0x02,
  AAI_Prog  = 0xad,
  RDSR      = 0x05,
  EWSR      = 0x50,
  WRSR      = 0x01,
  WREN      = 0x06,
  WRDI      = 0x04,
  ChipId    = 0x90,
  JEDEC_ID  = 0x9f,
  EBSY	    = 0x70,
  DBSY      = 0x80,
};

enum {
  Busy = 1<<0,                  /* Busy programming or erasing */
  nWEL = 1<<1,                  /* Write disabled */
  BP0  = 1<<2,
  BP1  = 1<<3,
  BP2  = 1<<4,
  BP3  = 1<<5,
  AAI  = 1<<6,                  /* Auto address increment enabled */
  BPL  = 1<<7,
};

enum {
  PMIC_INT_STAT_0  = 0,
  PMIC_INT_MASK_0  = 1,
  PMIC_INT_SENS_0  = 2,
  PMIC_INT_STAT_1  = 3,
  PMIC_INT_MASK_1  = 4,
  PMIC_INT_SENS_1  = 5,
  PMIC_PWRUP_SENS  = 6,
  PMIC_ID          = 7,
  PMIC_ACC_0       = 9,
  PMIC_ACC_1       = 10,
  PMIC_PWR_CTRL_0  = 13,
  PMIC_PWR_CTRL_1  = 14,
  PMIC_PWR_CTRL_2  = 15,
  PMIC_MEM_A       = 18,
  PMIC_MEM_B       = 19,
  PMIC_RTC_TIME    = 20,
  PMIC_RTC_ALM     = 21,
  PMIC_RTC_DAY     = 22,
  PMIC_RTC_DAY_ALM = 23,
  PMIC_SWITCHER_0  = 24,
  PMIC_SWITCHER_1  = 25,
  PMIC_SWITCHER_2  = 26,
  PMIC_SWITCHER_3  = 27,
  PMIC_SWITCHER_4  = 28,
  PMIC_SWITCHER_5  = 29,
  PMIC_REG_SETT_0  = 30,
  PMIC_REG_SETT_1  = 31,
  PMIC_REG_MODE_0  = 32,
  PMIC_REG_MODE_1  = 33,
  PMIC_PWR_MISC    = 34,
  PMIC_ADC_0       = 43,
  PMIC_ADC_1       = 44,
  PMIC_ADC_2       = 45,
  PMIC_ADC_3       = 46,
  PMIC_ADC_4       = 47,
  PMIC_CHRGR_0     = 48,
  PMIC_USB_0       = 49,
  PMIC_CHRGR_USB_1 = 50,
  PMIC_LED_0       = 51,
  PMIC_LED_1       = 52,
  PMIC_LED_2       = 53,
  PMIC_LED_3       = 54,
  PMIC_TRIM_0      = 57,
  PMIC_TRIM_1      = 58,
  PMIC_TEST_0      = 59,
  PMIC_TEST_1      = 60,
  PMIC_TEST_2      = 61,
  PMIC_TEST_3      = 62,
  PMIC_TEST_4      = 63,
};

extern void spi_select (const struct mx5_spi*);

static const struct mx5_spi mx5_spi_flash = {
  .bus                = 1,
  .slave              = 1,
  .sclk_frequency     = 25*1000*1000,
//  .sclk_frequency     = 80*1000*1000,
  .ss_active_high     = 0,
  .data_inactive_high = 0,
  .sclk_inactive_high = 0,
  .sclk_polarity      = 0,
  .sclk_phase         = 0,
  .select             = spi_select,
};

/* Clock is idle off. Data lines sampled on rising clock.  Clock low
   between bits.  Phase and polarity 0.  38ns clock period, 26MHz. */
static const struct mx5_spi mx5_spi_pmic = {
  .bus                = 1,
  .slave              = 0,
  .sclk_frequency     = 25*1000*1000,
  .ss_active_high     = 1,
  .data_inactive_high = 0,
  .sclk_inactive_high = 0,
  .sclk_polarity      = 0,
  .sclk_phase         = 0,
  .select             = spi_select,
  .talk               = true,
};

struct flash_chip {
  unsigned long total_size;
  unsigned long erase_size;
  char id[3];
  char name[32];
  bool erase_4k;
  bool erase_32k;
  bool erase_64k;
  bool erase_chip;
};

static const struct flash_chip flash_chips[] = {
  { .name = "Microchip SST25VF032B",
    .total_size = 4*1024*1024,
    .erase_size = 64*1024,
    .erase_4k = true,
    .erase_32k = true,
    .erase_64k = true,
    .erase_chip = true,
    .id = { 0xbf, 0x25, 0x4a } },
  { .name = "Microchip SST25VF016B",
    .total_size = 2*1024*1024,
    .erase_size = 64*1024,
    .id = { 0xbf, 0x25, 0x41 } },
  { .name = "MX25VF016B",
    .total_size = 4*1024*1024,
    .erase_size = 64*1024,
    .id = { 0xc2, 0x20, 0x16 } },
  { .name = "Winbond W25Q32BV",
    .total_size = 4*1024*1024,
    .erase_size = 64*1024,
    .id = { 0xef, 0x20, 0x16 } },
  { .name = "Atmel AT25DF161",
    .total_size = 2*1024*1024,
    .erase_size = 64*1024,
    .id = { 0x1f, 0x46, 0x02 } },
  { .name = "Numonyx M25P16",
    .total_size = 8*1024*1024,
    .erase_size = 64*1024,
    .id = { 0x20, 0x20, 0x17 } },
  { .name = "Numonyx M25P64",
    .total_size = 2*1024*1024,
    .erase_size = 64*1024,
    .id = { 0x20, 0x20, 0x15 } },
};

static const struct flash_chip* chip;

static void mx5_ecspi_setup (const struct mx5_spi* spi, int bytes)
{
  u32 v;

  v = ECSPIX_CONTROL(spi->bus);
  ECSPIX_CONTROL(spi->bus) = 0;          /* Reset eCSPI module */
  ECSPIX_CONTROL(spi->bus) = v | 1;

  v = ECSPIX_CONFIG (spi->bus);
  MASK_AND_SET (v,
                0
                | (1<<(20 + spi->slave))
                | (1<<(16 + spi->slave))
                | (1<<(12 + spi->slave))
                | (1<<( 8 + spi->slave))
                | (1<<( 4 + spi->slave))
                | (1<<( 0 + spi->slave))
                ,0
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
      | ((bytes*8 - 1)<<20)
      | (prediv      << 12)
      | (postdiv     << 8)
      | (spi->slave  << 18)
      | (1           <<(4 + spi->slave))
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

static void mx5_ecspi_transfer (const struct mx5_spi* spi,
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

static void mx5_spi_flash_init (void)
{
  char rgb[3] = { 0x9f };
  mx5_ecspi_transfer (&mx5_spi_flash, rgb, 1, 3, 1);

  for (chip = &flash_chips[0];
       chip < &flash_chips[sizeof (flash_chips)
                           /sizeof (struct flash_chip)];
       ++chip)
    if (   chip->id[0] == rgb[0]
        && chip->id[1] == rgb[1]
        && chip->id[2] == rgb[2])
      break;

  if (chip >= &flash_chips[sizeof (flash_chips)/sizeof (struct flash_chip)])
    chip = NULL;

  DBG(3, "ecspi read 0x%x 0x%x 0x%x\n",
      rgb[0], rgb[1], rgb[2]);
}

static void mx5_spi_flash_release (void)
{
  u32 v;

  spi_select (NULL);

  v = ECSPIX_CONTROL(0);
  ECSPIX_CONTROL(0) = 0;        /* Reset eCSPI module */
  ECSPIX_CONTROL(0) = v | 1;
  ECSPIX_CONTROL(0) = 0;        /* Shut down all SPI */
}

static int mx5_spi_flash_open (struct descriptor_d* d)
{
  ECSPIX_CONTROL(mx5_spi_flash.bus) = 0;      /* Shut down all SPI */

  return 0;
}

static void mx5_spi_flash_close (struct descriptor_d* d)
{
  close_helper (d);
}


static ssize_t mx5_spi_flash_read (struct descriptor_d* d, void* pv, size_t cb)
{
  ssize_t cbRead = 0;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  while (cb) {
    unsigned long index = d->start + d->index;
    int available = cb;
    if (index < chip->total_size && index + available > chip->total_size)
      available = chip->total_size - index;
    if (available > CBIT_BURST_MAX/8 - 4)
      available = CBIT_BURST_MAX/8 - 4;

    {
      char* rgb = pv;
//      printf ("index %ld (0x%lx)\n", index, index);
      rgb[0] = Read;
      rgb[1] = (index >> 16) & 0xff;
      rgb[2] = (index >>  8) & 0xff;
      rgb[3] = (index >>  0) & 0xff;
    }

    mx5_ecspi_transfer (&mx5_spi_flash, pv, 4, available, 4);

    d->index += available;
    cb -= available;
    cbRead += available;

    //    printf ("nor: 0x%p 0x%08lx %d\n", pv, index, available);

    pv += available;
  }

  return cbRead;
}

int mx5_spi_flash_byte_write_perform (u32 index, u8 data)
{
  static const char rgbEWSR[] = { EWSR };
  static const char rgbWRSR[] = { WRSR, 0 };
  static const char rgbWREN[] = { WREN };
  char rgbWrite[5] = { ByteProg };
//  EWSR, WRSR (0), WREN, PROGRAM,  while (RDSR & RDSR_BUSY);

  mx5_ecspi_transfer (&mx5_spi_flash, (void*) rgbEWSR, sizeof (rgbEWSR), 0, 0);
  mx5_ecspi_transfer (&mx5_spi_flash, (void*) rgbWRSR, sizeof (rgbWRSR), 0, 0);
  mx5_ecspi_transfer (&mx5_spi_flash, (void*) rgbWREN, sizeof (rgbWREN), 0, 0);
  rgbWrite[1] = (index >> 16) & 0xff;
  rgbWrite[2] = (index >>  8) & 0xff;
  rgbWrite[3] = (index >>  0) & 0xff;
  rgbWrite[4] = data;
  mx5_ecspi_transfer (&mx5_spi_flash, rgbWrite, sizeof (rgbWrite), 0, 0);

  while (true) {
    char rgbRDSR[1] = { RDSR };
    mx5_ecspi_transfer (&mx5_spi_flash, rgbRDSR, 1, 1, 1);
    if ((rgbRDSR[0] & Busy) == 0)
      return rgbRDSR[0];
    /* *** FIXME: there should be a timeout here */
  }
}

static ssize_t mx5_spi_flash_write (struct descriptor_d* d,
                                    const void* pv, size_t cb)
{
  size_t cbWrote = 0;

  while (cb) {
    unsigned long index = d->start + d->index;
    unsigned long status = 0;
    int step = 1;

#if defined (NO_WRITE)
//    printf ("write [0x%lx]<-0x%0*lx  page 0x%lx  step %d  cb 0x%x\n",
//	    index, NOR_BUS_WIDTH/4, (nor_t) data, page, step, cb);
#else
    status = mx5_spi_flash_byte_write_perform (index, *(u8*) pv);
#endif

    SPINNER_STEP;

    if (status) {
//    fail:
      printf ("Program failed at 0x%p (%lx)\n", (void*) index, status);
      return cbWrote;
    }

    d->index += step;
    pv       += step;
    cb       -= step;
    cbWrote  += step;
  }

  return cbWrote;
}


/* select the optimal erase size given a starting address of index and
   a total number of bytes to erase from index of cb.  This function
   attempts to optimize the size of erase operations while minimizing
   the amount of extra data erased. */

#define ERASE_ORIGIN(i,cb) ((i) & ~(cb - 1))

static size_t choose_erase_size (u32 index, size_t cb)
{
  size_t cbErase = 0;

  if (chip->erase_64k
      && (cbErase == 0
          || ERASE_ORIGIN(index, cbErase) < ERASE_ORIGIN(index, 64*1024)
          || ERASE_ORIGIN(index, cbErase) + cbErase > index + cb))
    cbErase = 64*1024;

  if (chip->erase_32k
      && (cbErase == 0
          || ERASE_ORIGIN(index, cbErase) < ERASE_ORIGIN(index, 32*1024)
          || ERASE_ORIGIN(index, cbErase) + cbErase > index + cb))
    cbErase = 32*1024;

  if (chip->erase_4k
      && (cbErase == 0
          || ERASE_ORIGIN(index, cbErase) < ERASE_ORIGIN(index, 4*1024)
          || ERASE_ORIGIN(index, cbErase) + cbErase > index + cb))
    cbErase = 4*1024;

  if (chip->erase_chip
      && (cbErase == 0
          || cb == chip->total_size))
    cbErase = chip->total_size;

  return cbErase;
}


int mx5_spi_flash_erase_perform (u32 index, size_t cbErase )
{
  static const char rgbEWSR[] = { EWSR };
  static const char rgbWRSR[] = { WRSR, 0 };
  static const char rgbWREN[] = { WREN };
  char rgbErase[4];
  size_t cb = 4;

  mx5_ecspi_transfer (&mx5_spi_flash, (void*) rgbEWSR, sizeof (rgbEWSR), 0, 0);
  mx5_ecspi_transfer (&mx5_spi_flash, (void*) rgbWRSR, sizeof (rgbWRSR), 0, 0);
  mx5_ecspi_transfer (&mx5_spi_flash, (void*) rgbWREN, sizeof (rgbWREN), 0, 0);

  rgbErase[1] = (index >> 16) & 0xff;
  rgbErase[2] = (index >>  8) & 0xff;
  rgbErase[3] = (index >>  0) & 0xff;

  switch (cbErase) {
  default:
    if (cbErase == chip->total_size) {
      rgbErase[0] = EraseChip;
      cb = 1;
    }
    else
      return 0;
    break;
  case 64*1024:
    rgbErase[0] = Erase64k;
    break;
  case 32*1024:
    rgbErase[0] = Erase32k;
    break;
  case 4*1024:
    rgbErase[0] = Erase4k;
    break;
  }

  mx5_ecspi_transfer (&mx5_spi_flash, rgbErase, cb, 0, 0);

  while (true) {
    char rgbRDSR[1] = { RDSR };
    mx5_ecspi_transfer (&mx5_spi_flash, rgbRDSR, 1, 1, 1);
    if ((rgbRDSR[0] & Busy) == 0)
      return rgbRDSR[0];
    /* *** FIXME: there should be a timeout here */
  }
}

static void mx5_spi_flash_erase (struct descriptor_d* d, size_t cb)
{
  if (d->index + cb > d->length)
    cb = d->length - d->index;

  SPINNER_STEP;

  while (cb > 0) {
    unsigned long index = d->start + d->index;
    size_t cbErase = choose_erase_size (index, cb);
    u8 status = 0;
    size_t available = cbErase - (index - (index & ~(cbErase - 1)));
    if (available > cb)
      available = cb;

#if defined (NO_WRITE)
//    printf ("  unlock status %lx\n", status);
#endif

#if defined (NO_WRITE)
//    WRITE_ONE (index, CMD (ReadArray));
//    printf ("fake erasing 0x%04x\n", REGA(index));
//    printf ("nor_erase index 0x%lx  available 0x%lx\n",
//	    index, available);
//    status = STAT (Ready);
#else
    DBG (2,"index %ld (0x%lx)  cb %d  cbErase %d  av %d\n",
         index, index, cb, cbErase, available);
    status = mx5_spi_flash_erase_perform (index, cbErase);
#endif

    SPINNER_STEP;

    if (status) {
//    fail:
      printf ("Erase failed at 0x%p (%x)\n", (void*) index, status);
      return;
    }

    cb -= available;
    d->index += available;
  }
}


#if !defined (CONFIG_SMALL)

static void mx5_spi_flash_report (void)
{
  if (!chip)
    return;

  printf ("  flash:  %ld Mib %s [%02x %02x %02x]\n",
          chip->total_size*8/(1024*1024),
          chip->name,
          chip->id[0], chip->id[1], chip->id[2]);
}

#endif

static __driver_3 struct driver_d mx5_spi_flash_driver = {
  .name        = "flash-spi-mx5",
  .description = "SPI flash driver",
#if defined (USE_BUFFERED_WRITE)
  .flags       = DRIVER_WRITEPROGRESS(5),
#else
  .flags       = DRIVER_WRITEPROGRESS(2),
#endif
  .open        = mx5_spi_flash_open,
  .close       = mx5_spi_flash_close,
  .read        = mx5_spi_flash_read,
  .write       = mx5_spi_flash_write,
  .erase       = mx5_spi_flash_erase,
  .seek        = seek_helper,
  //  .query       = mx5_spi_flash_query,
};

static __service_9 struct service_d mx5_spi_flash_service = {
  .init	       = mx5_spi_flash_init,
  .release     = mx5_spi_flash_release,
#if !defined (CONFIG_SMALL)
  .report      = mx5_spi_flash_report,
#endif
};

static u32 mx5_spi_pmic_transfer (int reg, u32 data, bool write)
{
  char rgb[4];
  rgb[0] = (write ? (1<<7) : 0) | ((reg & 0x3f) << 1);
  rgb[1] = (data >> 16) & 0xff;
  rgb[2] = (data >>  8) & 0xff;
  rgb[3] = (data >>  0) & 0xff;

  mx5_ecspi_transfer (&mx5_spi_pmic, rgb, 4, 4, 0);

  return 0
    | (rgb[1] << 16)
    | (rgb[1] <<  8)
    | (rgb[2] << 0);
}

static void mx5_spi_pmic_init (void)
{
  char rgb[4] = { (0<<7) | (PMIC_ID << 1) };
  mx5_ecspi_transfer (&mx5_spi_pmic, rgb, 4, 4, 0);

  printf ("PMIC ID %x %x %x %x\n", rgb[0], rgb[1], rgb[2], rgb[3]);
}

static __service_9 struct service_d mx5_spi_pmic_service = {
//  .init	       = mx5_spi_pmic_init,
//  .release     = mx5_spi_flash_release,
#if !defined (CONFIG_SMALL)
//  .report      = mx5_spi_flash_report,
#endif
};


static int cmd_pmic (int argc, const char** argv)
{
  u32 value;
  mx5_spi_pmic_transfer (PMIC_PWR_MISC, 0x00200000, true);
  value = mx5_spi_pmic_transfer (PMIC_ID, 0, false);
  printf ("PMIC ID 0x%x\n", value);

  return 0;
}

static __command struct command_d c_pmic = {
  .command = "pmic",
  .func = cmd_pmic,
  COMMAND_DESCRIPTION ("test PMIC")
};
