/* codec.c
     $Id$

   written by Marc Singer
   8 Dec 2004

   Copyright (C) 2004 Marc Singer

   -----------
   DESCRIPTION
   -----------

   Test code for the audio codec. 

   You can dump an 8 bit file with hexdump.  This one skips to the
   0x2c'th byte before dumping.

     /usr/bin/hexdump -v -s 0x2c -e '8 1 "0x%02x, " "\n\t"' k3b_success1.wav

*/

#include <config.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <apex.h>
#include <command.h>
#include "hardware.h"

#include "pcm.h"

#define USE_I2S
//#define USE_CPU_MASTER

#define T_SKH	1		/* Clock time high (us) */
#define T_SKL	1		/* Clock time low (us) */
#define T_CS	1		/* Minimum chip select low time (us)  */
#define T_CSS	1		/* Minimum chip select setup time (us)  */
#define T_DIS	1		/* Data setup time (us) */

#define SSP_CTRL0	(0x00)
#define SSP_CTRL1	(0x04)
#define SSP_DR		(0x08)
#define SSP_SR		(0x0c)
#define SSP_CPSR	(0x10)
#define SSP_IMSC	(0x14)
#define SSP_RIS		(0x18)
#define SSP_MIS		(0x1c)
#define SSP_ICR		(0x20)
#define SSP_DCR		(0x24)

#define SSP_SR_BSY	(1<<4)
#define SSP_SR_REFI	(1<<3)
#define SSP_SR_RNE	(1<<2)	/* Receive FIFO not empty */
#define SSP_SR_TNF	(1<<1)	/* Transmit FIFO not full */
#define SSP_SR_TFE	(1<<0)	/* Transmit FIFO empty */

#define I2S_CTRL	(0x00)
#define I2S_STAT	(0x04)
#define I2S_IMSC	(0x08)
#define I2S_RIS		(0x0c)
#define I2S_MIS		(0x10)
#define I2S_ICR		(0x14)

#define I2S_CTRL_MCLKINV (1<<4)
#define I2S_CTRL_WSDEL	(1<<3)
#define I2S_CTRL_WSINV	(1<<2)
#define I2S_CTRL_I2SEN	(1<<1)
#define I2S_CTRL_I2SLBM	(1<<0)

#define I2S_STAT_MS	(1<<8)
#define I2S_STAT_RFF	(1<<7)
#define I2S_STAT_RFE	(1<<6)	/* Receive FIFO not empty from SSP */
#define I2S_STAT_TFF	(1<<5)
#define I2S_STAT_TFE	(1<<4)	/* Transmit FIFO empty from SSP */
#define I2S_STAT_TXWS	(1<<3)
#define I2S_STAT_RXWS	(1<<2)
#define I2S_STAT_WS	(1<<1)
#define I2S_STAT_LBM	(1<<0)

#define CODEC_LIN_VOLUME	(0x0)
#define CODEC_RIN_VOLUME	(0x1)
#define CODEC_LOUT_VOLUME	(0x2)
#define CODEC_ROUT_VOLUME	(0x3)
#define CODEC_ANALOG_CTRL	(0x4)
#define CODEC_DIGITAL_CTRL	(0x5)
#define CODEC_POWER_CTRL	(0x6)
#define CODEC_DIGITAL_FORMAT	(0x7)
#define CODEC_SAMPLE_RATE	(0x8)
#define CODEC_DIGITAL_ACTIVATE	(0x9)
#define CODEC_RESET		(0xf)

#define CODEC_ADDR_SHIFT	(9)

#define FREQUENCY_4K		(0) /* 8kHz with MCLK/2 */
#define FREQUENCY_8K		(1)
#define FREQUENCY_22K05		(2) /* 44.1kHz with MCLK/2 */
#define FREQUENCY_32K1		(3)
#define FREQUENCY_44K1		(4)
#define FREQUENCY_48K		(5)
#define FREQUENCY_88K2		(6)
#define FREQUENCY_96K		(7)

#define CMD(a,c)	((((a) & 0x7f)<<CODEC_ADDR_SHIFT)|((c) & 0x1ff))

static void enable_cs (void)
{
  __REG8 (CPLD_SPI) &= ~(1<<5);
  usleep (T_CSS);
}

static void disable_cs (void)
{
  usleep (T_CS);
  __REG8 (CPLD_SPI) |=  (1<<5);
  usleep (T_CS);
}

static void pulse_clock (void)
{
  unsigned char reg = __REG8 (CPLD_SPI) & ~CPLD_SPI_SCLK;
  __REG8 (CPLD_SPI) = reg | CPLD_SPI_SCLK;
  usleep (T_SKH);
  __REG8 (CPLD_SPI) = reg;
  usleep (T_SKL);
}


/* execute_spi_command

   sends a spi command to a device.  It first sends cwrite bits from
   v.  If cread is greater than zero it will read cread bits
   (discarding the leading 0 bit) and return them.  If cread is less
   than zero it will check for completetion status and return 0 on
   success or -1 on timeout.  If cread is zero it does nothing other
   than sending the command.

*/

static void execute_spi_command (int v, int cwrite)
{
  //  unsigned long l = 0;
  unsigned char reg;

  enable_cs ();

  reg = __REG8 (CPLD_SPI) & ~CPLD_SPI_TX;
  v <<= CPLD_SPI_TX_SHIFT; /* Correction for position of SPI_TX bit */
  while (cwrite--) {
    __REG8 (CPLD_SPI) 
      = reg | ((v >> cwrite) & CPLD_SPI_TX);
    usleep (T_DIS);
    pulse_clock ();
  }

#if 0
  if (cread < 0) {
    int delay = 10;
    disable_cs ();
    usleep (1);
    enable_cs ();
	
    l = -1;
    do {
      if (__REG8 (CPLD_SPI) & CPLD_SPI_RX) {
	l = 0;
	break;
      }
    } while (usleep (1), --delay);
  }
  else
	/* We pulse the clock before the data to skip the leading zero. */
    while (cread-- > 0) {
      pulse_clock ();
      l = (l<<1) 
	| (((__REG8 (CPLD_SPI) & CPLD_SPI_RX) 
	    >> CPLD_SPI_RX_SHIFT) & 0x1);
    }
#endif
  
  disable_cs ();
//  return l;
}

static void codec_unmute (void)
{
  execute_spi_command (CMD (CODEC_POWER_CTRL, 
			    (1<<2) /* Disable ADC  */
			    | (1<<1) /* Disable MIC  */
			    | (1<<0) /* Disable LINE IN */
			    ), 16);
  execute_spi_command (CMD (CODEC_ANALOG_CTRL, 
			    (1<<4) /* Enable DAC */
			    |(1<<1) /* Mute Microphone */
			    ), 16);
  execute_spi_command (CMD (CODEC_DIGITAL_ACTIVATE, (1<<0)), 16);
}

static void codec_mute (void)
{
  execute_spi_command (CMD (CODEC_ANALOG_CTRL, 
			    (0<<4) /* Disable DAC */
			    |(1<<1) /* Mute Microphone */
			    ), 16);
  execute_spi_command (CMD (CODEC_DIGITAL_ACTIVATE, (0<<0)), 16);
}

static void codec_configure (int frequency, int sample_size)
{
  unsigned short v = 0;
  //      (0<<7) /* MCLK input */
  //    | (0<<6) /* MCLK output */
  //    | (0<<0); /* Normal */

  switch (frequency) {
  default:
  case FREQUENCY_4K:
    v |= (0xb<<2) /* SR3-SR0 */
      |  (0<<1); /* BOSR */
    v |= (1<<7)|(1<<6);		/* MCLK/2 */
    break;
  case FREQUENCY_8K:
    v |= (0xb<<2) /* SR3-SR0 */
      |  (0<<1); /* BOSR */
    break;
  case FREQUENCY_32K1:
    break;
  case FREQUENCY_22K05:
    v |= (0x8<<2) /* SR3-SR0 */
      |  (0<<1); /* BOSR */
    v |= (1<<7)|(1<<6);		/* MCLK/2 */
    break;
  case FREQUENCY_44K1:
    v |= (0x8<<2) /* SR3-SR0 */
      |  (0<<1); /* BOSR */
    break;
  case FREQUENCY_48K:
    break;
  case FREQUENCY_88K2:
    v |= (0xf<<2) /* SR3-SR0 */
      |  (0<<1); /* BOSR */
    break;
  case FREQUENCY_96K:
    break;
  }

  execute_spi_command (CMD (CODEC_SAMPLE_RATE, v), 16);
  //  usleep (1);

  __REG (SSP_PHYS + SSP_CTRL0) 
    = (1<<4)			/* TI synchronous mode */
    | ((sample_size - 1) & 0xf)<<0;
}

static void codec_init (void)
{
  __REG (RCPC_PHYS + RCPC_CTRL) |= RCPC_CTRL_UNLOCK;
  MASK_AND_SET (__REG (RCPC_PHYS + RCPC_CTRL),  /* System osc. -> CLKOUT */
		3<<5, 0<<5);
  __REG (RCPC_PHYS + RCPC_PCLKSEL1) |= 1<<1;	/* System osc. -> SSP clock */
  __REG (RCPC_PHYS + RCPC_SSPPRE) = (1 - 1);	/* Prescalar = 1 */
  __REG (RCPC_PHYS + RCPC_PCLKCTRL1) &= ~(1<<1); /* Enable SSP clock */
  __REG (RCPC_PHYS + RCPC_CTRL) &= ~RCPC_CTRL_UNLOCK;

  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_MUXCTL5),
		(3<<6)|(3<<4)|(3<<2)|(3<<0),
		(1<<6)|(1<<4)|(1<<2)|(1<<0));	/* SSP/I2S signals */
//  __REG (IOCON_PHYS + IOCON_RESCTL5) &= ~((3<<6)|(3<<4)|(3<<2)|(3<<0));

  __REG (SSP_PHYS + SSP_CTRL1) = 0
#if defined (USE_CPU_MASTER)
    | (0<<2)			/* master */
#else
    | (1<<2)			/* slave */
#endif
    | (1<<1)			/* enabled */
    | (0<<3);			/* SSP can driver SSPTX */
  __REG (SSP_PHYS + SSP_CPSR) = 2; /* Smallest prescalar is 2 */
#if defined (USE_I2S)
  __REG (I2S_PHYS + I2S_CTRL)
    = I2S_CTRL_I2SEN
    ;
#endif

  codec_mute ();

  execute_spi_command (CMD (CODEC_RESET, 0), 16);
  //  usleep (1);
  //  usleep (1);
  execute_spi_command (CMD (CODEC_DIGITAL_CTRL, 
			    0
//			    |(2<<1) /* 44.1 kHz deemphasis */
			    ), 16);
  //  usleep (1);
  //  usleep (1);
  execute_spi_command (CMD (CODEC_DIGITAL_FORMAT,
			    0
#if defined (USE_CPU_MASTER)
			    | (0<<6) /* Slave */
#else
			    | (1<<6) /* Master */
#endif
//			    | (1<<4) /* LRP: right channel on LRC low */
			    | (0<<2) /* 16 bit */
//			    | (3<<2) /* 32 bit */
#if defined (USE_I2S)
			    | (2<<0) /* I2S format */
#else
			    | (3<<0) /* DSP format */
#endif
			    ), 16);
  //  usleep (1);
  codec_configure (FREQUENCY_44K1, 16);
  //  usleep (1);
//  execute_spi_command (CMD (CODEC_DIGITAL_ACTIVATE, (1<<0)), 16);

  usleep (10);
  printf ("ssp: status 0x%lx\r\n", __REG (SSP_PHYS | SSP_SR));
}

static void codec_release (void)
{
  __REG (RCPC_PHYS + RCPC_CTRL) |= RCPC_CTRL_UNLOCK;
  __REG (RCPC_PHYS + RCPC_PCLKCTRL1) |= (1<<1);	/* Disable SSP clock */
  __REG (RCPC_PHYS + RCPC_CTRL) &= ~RCPC_CTRL_UNLOCK;
}

#define USE_DMA
#define USE_16

static int cmd_codec_test (int argc, const char** argv)
{
  static unsigned short __attribute__((section("pcm.data"))) buffer[16384];
  int i;
  int cSampleMax = sizeof (rgbPCM);
  static unsigned long cap;

#if defined (USE_16)
#define rgb buffer  
#else
#define rgb rgbPCM
#endif

#if defined (USE_16)
  codec_configure (FREQUENCY_8K, 16);
#else
  codec_configure (FREQUENCY_8K, 8);
#endif

  codec_unmute ();

#if defined (USE_16)
  cSampleMax = sizeof (buffer)/sizeof (buffer[0]);
  for (i = 0; i < cSampleMax; ++i) {
    unsigned short v = (rgbPCM[i]<<8) | rgbPCM[i];
    v = v/2;// + 0x8000/2;
    buffer[i] = v;
  }
#endif

#if defined (USE_DMA)
  __REG (RCPC_PHYS + RCPC_CTRL) |= RCPC_CTRL_UNLOCK;
  __REG (RCPC_PHYS | RCPC_AHBCLKCTRL) &= ~(1<<0); /* Enable DMA AHB clock */
  __REG (RCPC_PHYS + RCPC_CTRL) &= ~RCPC_CTRL_UNLOCK;

  //  __REG (SSP_PHYS + SSP_IMSC) |= 0
  //    | (1<<3)
  //    | (1<<2);			/* Interrupts needed for DMA */
  __REG (SSP_PHYS + SSP_DCR)  |= 0
    | (1<<1)			/* TX DMA enabled */
    | (1<<0)			/* RX DMA enabled */
    ;

  __REG (DMA_PHYS + DMA_MASK)	   = 0; /* Mask all interrupts */
  __REG (DMA_PHYS + DMA_CLR)	   = 0xff; /* Clear pending interrupts */

#if 1
  __REG (DMA0_PHYS + DMA_CTRL)     = 0; /* Disable */
  __REG (DMA0_PHYS + DMA_SOURCELO) =  (SSP_PHYS + SSP_DR)          & 0xffff;
  __REG (DMA0_PHYS + DMA_SOURCEHI) = ((SSP_PHYS + SSP_DR) >> 16)   & 0xffff;
  __REG (DMA0_PHYS + DMA_DESTLO)   =  ((unsigned long)&cap)        & 0xffff;
  __REG (DMA0_PHYS + DMA_DESTHI)   = (((unsigned long)&cap) >> 16) & 0xffff;
  __REG (DMA0_PHYS + DMA_MAX)	   = 0xffff;
  __REG (DMA0_PHYS + DMA_CTRL)
    = (0<<13)			/* Peripheral source */
    | (0<<9)			/* Load base addresses on start  */
    | (1<<7)			/* Destination size 2 bytes */
    | (1<<3)			/* Source size is 2 bytes */
    | (0<<2)			/* Destination fixed */
    | (0<<1);			/* Source fixed */

#endif

  __REG (DMA1_PHYS + DMA_CTRL)     = 0; /* Disable */
  __REG (DMA1_PHYS + DMA_SOURCELO) =  ((unsigned long)rgb)        & 0xffff;
  __REG (DMA1_PHYS + DMA_SOURCEHI) = (((unsigned long)rgb) >> 16) & 0xffff;
  __REG (DMA1_PHYS + DMA_DESTLO)   =  (SSP_PHYS + SSP_DR)	     & 0xffff;
  __REG (DMA1_PHYS + DMA_DESTHI)   = ((SSP_PHYS + SSP_DR) >> 16)     & 0xffff;
  __REG (DMA1_PHYS + DMA_MAX)	   = cSampleMax;
  __REG (DMA1_PHYS + DMA_CTRL)
    = (1<<13)			/* Peripheral destination */
#if defined (USE_16)
    | (1<<7)			/* Destination size 2 bytes */
    | (1<<3)			/* Source size is 2 bytes */
//    | (1<<5)			/* Source burst 4 incrementing */
#else
    | (0<<7)			/* Destination size 1 bytes */
    | (0<<3)			/* Source size is 1 byte */
    | (1<<5)			/* Source burst 4 incrementing */
#endif
    | (0<<9)			/* Wrapping: Load base addresses on start  */
    | (0<<2)			/* Destination fixed */
    | (1<<1);			/* Source incremented */

  __REG (DMA0_PHYS + DMA_CTRL) |= (1<<0); /* Enable RX DMA */
  __REG (DMA1_PHYS + DMA_CTRL) |= (1<<0); /* Enable TX DMA */

				/* Wait for completion */
  while ((__REG (DMA_PHYS + DMA_STATUS) & (1<<1)) == 0)
    ;

#else

#if defined (USE_16)

  for (i = 0; i < cSampleMax; ++i) {
    //    unsigned char v = rgbPCM[i]/2 + 0x8000/2;

				/* Wait for room in the FIFO */
    while ((__REG16 (SSP_PHYS + SSP_SR) & SSP_SR_TNF) == 0)
      ;
    //    __REG (SSP_PHYS + SSP_DR) = ((rgbPCM[i] << 8)|rgbPCM[i]);
//    __REG16 (SSP_PHYS + SSP_DR) = (v<<8)|(v);
    __REG16 (SSP_PHYS + SSP_DR) = rgb[i];
				/* Wait for room in the FIFO */
    //    while ((__REG (SSP_PHYS + SSP_SR) & SSP_SR_TNF) == 0)
    //      ;
    //    __REG (SSP_PHYS + SSP_DR) = (rgbPCM[i] << 8)|rgbPCM[i];
  }
#else

  for (i = 0; i < sizeof (rgbPCM); ++i) {
//    unsigned char v = rgbPCM[i]/2 + 0x8000/2;
    unsigned char v = rgbPCM[i];
    
				/* Wait for room in the FIFO */
    while ((__REG16 (SSP_PHYS + SSP_SR) & SSP_SR_TNF) == 0)
      ;
    //    __REG (SSP_PHYS + SSP_DR) = ((rgbPCM[i] << 8)|rgbPCM[i]);
//    __REG16 (SSP_PHYS + SSP_DR) = (v<<8)|(v);
    __REG16 (SSP_PHYS + SSP_DR) = v;
				/* Wait for room in the FIFO */
    //    while ((__REG (SSP_PHYS + SSP_SR) & SSP_SR_TNF) == 0)
    //      ;
    //    __REG (SSP_PHYS + SSP_DR) = (rgbPCM[i] << 8)|rgbPCM[i];
  }

				/* Clear the pipe  */
  __REG (SSP_PHYS + SSP_DR) = 0x7fff;
  __REG (SSP_PHYS + SSP_DR) = 0x7fff;
  __REG (SSP_PHYS + SSP_DR) = 0x7fff;
  __REG (SSP_PHYS + SSP_DR) = 0x7fff;
  __REG (SSP_PHYS + SSP_DR) = 0x7fff;
  __REG (SSP_PHYS + SSP_DR) = 0x7fff;
  __REG (SSP_PHYS + SSP_DR) = 0x7fff;
  __REG (SSP_PHYS + SSP_DR) = 0x7fff;
#endif

#endif /* !USE_DMA */

  codec_mute ();
  return 0;
}

static __service_7 struct service_d lpd79524_codec_service = {
  .init = codec_init,
  .release = codec_release,
};

static __command struct command_d c_codec = {
  .command = "codec",
  .description = "test codec",
  .func = cmd_codec_test,
};
