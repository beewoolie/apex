/* codec.c
     $Id$

   written by Marc Singer
   8 Dec 2004

   Copyright (C) 2004 Marc Singer

   -----------
   DESCRIPTION
   -----------

   Test code for the audio codec. 

*/

#include <config.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <apex.h>
#include "hardware.h"

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

static void execute_spi_command (int v, int cwrite, int cread)
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
  __REG (IOCON_PHYS + IOCON_RESCTL5) &= ~((3<<6)|(3<<4)|(3<<2)|(3<<0));

  __REG (SSP_PHYS + SSP_CTRL0) 
    = (1<<4)			/* TI synchronous mode */
    | ((16 - 1) & 0xf)<<0;	/* Frame size = 16 */
  __REG (SSP_PHYS + SSP_CTRL1) 
    = (1<<2)			/* slave */
//    = (0<<2)			/* master */
    | (1<<1)			/* enabled */
    | (0<<3);			/* SSP can driver SSPTX */
  __REG (SSP_PHYS + SSP_CPSR) = 2; /* Smallest prescalar is 2 */
  __REG (I2S_PHYS + I2S_CTRL) = I2S_CTRL_I2SEN;

  __REG (SSP_PHYS + SSP_DR) = 0x7fff;
  __REG (SSP_PHYS + SSP_DR) = 0x7fff;
  __REG (SSP_PHYS + SSP_DR) = 0x7fff;
  __REG (SSP_PHYS + SSP_DR) = 0x7fff;
  __REG (SSP_PHYS + SSP_DR) = 0x7fff;
  __REG (SSP_PHYS + SSP_DR) = 0x7fff;
  __REG (SSP_PHYS + SSP_DR) = 0x7fff;
  __REG (SSP_PHYS + SSP_DR) = 0x7fff;

  execute_spi_command (CMD (CODEC_RESET, 0), 16, 0);
  usleep (1);
  execute_spi_command (CMD (CODEC_ANALOG_CTRL, 
			    (1<<4) /* Enable DAC */
			    |(1<<1) /* Mute Microphone */
			    ), 16, 0);
  usleep (1);
  execute_spi_command (CMD (CODEC_POWER_CTRL, 
			    (1<<2) /* Disable ADC  */
			    | (1<<1) /* Disable MIC  */
			    | (1<<0) /* Disable LINE  */
			    ), 16, 0);
  usleep (1);
  execute_spi_command (CMD (CODEC_DIGITAL_FORMAT,
			      (1<<1) /* I2S */
			    | (1<<6) /* Master */
			    | (0<<2) /* 16 bit */
			    | (2<<0) /* I2S format */
			    ), 16, 0);
  usleep (1);
  execute_spi_command (CMD (CODEC_SAMPLE_RATE,
			      (0<<7) /* MCLK input */
			    | (0<<6) /* MCLK output */
			    | (8<<2) /* SR3-SR0 */
			    | (0<<1) /* BOSR */
			    | (0<<0) /* Normal */
			    ), 16, 0);
  usleep (1);
  execute_spi_command (CMD (CODEC_DIGITAL_ACTIVATE, (1<<0)), 16, 0);

  usleep (10);
  printf ("ssp: status 0x%lx\r\n", __REG (SSP_PHYS | SSP_SR));
}

static void codec_release (void)
{
  __REG (RCPC_PHYS + RCPC_CTRL) |= RCPC_CTRL_UNLOCK;
  __REG (RCPC_PHYS + RCPC_PCLKCTRL1) |= (1<<1);	/* Disable SSP clock */
  __REG (RCPC_PHYS + RCPC_CTRL) &= ~RCPC_CTRL_UNLOCK;
}

static __service_7 struct service_d lpd79524_codec_service = {
  .init = codec_init,
  .release = codec_release,
};
