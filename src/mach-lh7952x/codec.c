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

     hexdump -v -s 0x2c -n 32768 -e '8 1 "0x%02x, " "\n\t"' k3b_success1.wav

   A 16 bit stereo file:   

     hexdump -v -s 0x108b0 -n 100000 -e '8 2 "0x%04x, " "\n\t"' track01.wav

  NOTES
  -----

  o There seems to be more at play in the signed versus unsigned
    arena.  The WAV format certainly uses signed integers for the
    sample values. 
  o As slave, the CPU appears to properly sequence data out in that
    the data starts one (or perhaps two) cycles after the start of the
    IWS signal transition.
  o As master, the CPU appears to correctly delay the data for one
    clock cycle, but does not give time for the LSB.  The IWS
    frequency is exactly sample_width*2*SSP_CLK.  In fact, the MSB
    appears within the next frame.  This does not conform to the
    Philips standard for I2S.
  o In I2S CPU master mode, setting WSDEL to 1 breaks I2S framing.
    The SSP pulse isn't converted to a proper IWS pulse.

*/

#include <config.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <apex.h>
#include <command.h>
#include "hardware.h"

//#define USE_ERIC

#define USE_I2S
#define USE_CPU_MASTER
#define USE_DMA
//#define USE_DMA_CAP
#define USE_16
#define USE_STEREO
#define USE_LOOPBACK_I2S
//#define USE_LOOPBACK_SSP
//#define USE_8KHZ
//#define USE_22KHZ
#define USE_44KHZ
#define USE_LOOPS	2000	/* Only works without DMA */

//#define USE_E5
//#define USE_E5_RIGHT


#if defined (USE_8KHZ)
# include "pcm8-8.h"
# define SAMPLE_FREQUENCY 8021
#endif

#if defined (USE_22KHZ)// || defined (USE_44KHZ)
# include "pcm2205-16.h"
# define SAMPLE_FREQUENCY 22050
#endif

#if defined (USE_44KHZ)// && 0 // || defined (USE_22KHZ)
# include "pcm441-16b.h"
# define SAMPLE_FREQUENCY 44100
#endif

#define T_SKH	1		/* Clock time high (us) */
#define T_SKL	1		/* Clock time low (us) */
#define T_CS	1		/* Minimum chip select low time (us)  */
#define T_CSS	1		/* Minimum chip select setup time (us)  */
#define T_DIS	1		/* Data setup time (us) */

#define SSP_CTRL0	__REG(SSP_PHYS + 0x00)
#define SSP_CTRL1	__REG(SSP_PHYS + 0x04)
#define SSP_DR_PHYS	(SSP_PHYS + 0x08)
#define SSP_DR		__REG(SSP_PHYS + 0x08)
#define SSP_SR		__REG(SSP_PHYS + 0x0c)
#define SSP_CPSR	__REG(SSP_PHYS + 0x10)
#define SSP_IMSC	__REG(SSP_PHYS + 0x14)
#define SSP_RIS		__REG(SSP_PHYS + 0x18)
#define SSP_MIS		__REG(SSP_PHYS + 0x1c)
#define SSP_ICR		__REG(SSP_PHYS + 0x20)
#define SSP_DCR		__REG(SSP_PHYS + 0x24)

#define SSP_CTRL1_MS	(1<<2)
#define SSP_CTRL1_LBM	(1<<0)
#define SSP_CTRL1_SSE	(1<<1)

#define SSP_SR_BSY	(1<<4)
#define SSP_SR_REFI	(1<<3)
#define SSP_SR_RNE	(1<<2)	/* Receive FIFO not empty */
#define SSP_SR_TNF	(1<<1)	/* Transmit FIFO not full */
#define SSP_SR_TFE	(1<<0)	/* Transmit FIFO empty */

#define I2S_CTRL	__REG(I2S_PHYS + 0x00)
#define I2S_STAT	__REG(I2S_PHYS + 0x04)
#define I2S_IMSC	__REG(I2S_PHYS + 0x08)
#define I2S_RIS		__REG(I2S_PHYS + 0x0c)
#define I2S_MIS		__REG(I2S_PHYS + 0x10)
#define I2S_ICR		__REG(I2S_PHYS + 0x14)

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

void ssp_set_speed (int speed);

static void msleep (int ms)
{
  unsigned long time = timer_read ();
	
  do {
  } while (timer_delta (time, timer_read ()) < ms);
}

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

  printf ("spi 0x%04x -> 0x%x\r\n", v & 0x1ff, (v >> CODEC_ADDR_SHIFT) & 0x7f);
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

#if !defined (USE_ERIC)
static void codec_enable (void)
{
  execute_spi_command (CMD (CODEC_DIGITAL_ACTIVATE, (1<<0)), 16);
}

static void codec_disable (void)
{
  execute_spi_command (CMD (CODEC_DIGITAL_ACTIVATE, (0<<0)), 16);
}

static void codec_unmute (void)
{
  execute_spi_command (CMD (CODEC_POWER_CTRL, 0
			    | (1<<2) /* Disable ADC  */
			    | (1<<1) /* Disable MIC  */
			    | (1<<0) /* Disable LINE IN */
			    ), 16);
  execute_spi_command (CMD (CODEC_ANALOG_CTRL, 0
			    | (1<<4) /* Enable DAC */
			    | (1<<1) /* Mute Microphone */
			    ), 16);
}

static void codec_mute (void)
{
  execute_spi_command (CMD (CODEC_ANALOG_CTRL, 0
			    | (0<<4) /* Disable DAC */
			    | (1<<1) /* Mute Microphone */
			    ), 16);
}

static void codec_configure (int frequency, int sample_size)
{
  unsigned short v = 0
  //      (0<<7) /* MCLK input */
  //    | (0<<6) /* MCLK output */
  //    | (0<<0); /* Normal */
    ;
  switch (frequency) {
  default:
  case 4010:			/* 4 kHz */
    v |= (0xb<<2) /* SR3-SR0 */
      |  (0<<1); /* BOSR */
    v |= (1<<7)|(1<<6);		/* MCLK/2 */
    break;
  case 8021:			/* 8 kHz */
    v |= (0xb<<2) /* SR3-SR0 */
      |  (0<<1); /* BOSR */
    break;
  case 32100:
    break;
  case 22050:
    v |= (0x8<<2) /* SR3-SR0 */
      |  (0<<1); /* BOSR */
    v |= (1<<7)|(1<<6);		/* MCLK/2 */
    break;
  case 44100:
    v |= (0x8<<2) /* SR3-SR0 */
      |  (0<<1); /* BOSR */
    break;
  case 48000:
    break;
  case 88200:
    v |= (0xf<<2) /* SR3-SR0 */
      |  (0<<1); /* BOSR */
    break;
  case 96000:
    break;
  }

  printf ("configuring codec for %d Hz with 0x%x\r\n", frequency, v);
  execute_spi_command (CMD (CODEC_SAMPLE_RATE, v), 16);

  MASK_AND_SET (SSP_CTRL0, 0xf, (sample_size - 1) & 0xf);
  //  SSP_CTRL0 
  //    = (1<<4)			/* TI synchronous mode */
  //    | ((sample_size - 1) & 0xf)<<0;

#if defined (USE_CPU_MASTER)
  ssp_set_speed (frequency);
#else
  __REG (RCPC_PHYS + RCPC_CTRL) |= RCPC_CTRL_UNLOCK;
  MASK_AND_SET (__REG (RCPC_PHYS + RCPC_CTRL),  /* System osc. -> CLKOUT */
		3<<5, 0<<5);
  __REG (RCPC_PHYS + RCPC_CTRL) &= ~RCPC_CTRL_UNLOCK;
#endif
}
#endif

#define RCPC_SSP_PRESCALE_MAX 256
#define SSP_PRESCALE_MAX 254
#define SSP_PRESCALE_MIN 2
#define SSP_DIVIDER_MAX 256
#define MAX_SSP_FREQ (hclk_freq / SSP_PRESCALE_MIN)
#define SSP_MAX_TIMEOUT 0xffff

void ssp_set_speed (int speed)
{
  int rcpc_prescale;
  int ssp_dvsr;
  int ssp_cpd;

  switch (speed) {
  default:
  case 8012:
    /* .08% error at HCLK 50MHz */
    rcpc_prescale = 32;
    ssp_dvsr = 2;
    ssp_cpd = 99;
    break;
  case 22050:
    /* No error at HCLK 50MHz */
    rcpc_prescale = 32 ;
    ssp_dvsr = 2;
    ssp_cpd = 36;
    break;
  case 44100:
    /* No error at HCLK 50MHz */
    rcpc_prescale = 32 ;
    ssp_dvsr = 2;
    ssp_cpd = 18;
    break;
  }

  if (ssp_cpd == 0)
    ssp_cpd = 1;

  rcpc_prescale /= 32;		/* Compensate for the frame size */

  printf ("ssp_set_speed  rcpc_ssppre %d  dvsr %d  cpd %d\r\n", 
	  rcpc_prescale, ssp_dvsr, ssp_cpd);

  __REG (RCPC_PHYS + RCPC_CTRL) |= RCPC_CTRL_UNLOCK;
  __REG (RCPC_PHYS + RCPC_PCLKSEL1) &= ~(1<<1);	/* HCLK -> SSP clock */
  __REG (RCPC_PHYS + RCPC_SSPPRE) = rcpc_prescale>>1;
  SSP_CPSR = ssp_dvsr;
  MASK_AND_SET (SSP_CTRL0, (0xff<<8), (ssp_cpd - 1)<<8);
  __REG (RCPC_PHYS + RCPC_CTRL) &= ~RCPC_CTRL_UNLOCK;
}


#if defined (USE_ERIC)
static void codec_init (void)
{
  printf ("codec_init eric\r\n");

  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_MUXCTL5),
		(3<<6)|(3<<4)|(3<<2)|(3<<0),
		(1<<6)|(1<<4)|(1<<2)|(1<<0));	/* SSP/I2S signals */

#if defined (USE_LOOPBACK_I2S)
  I2S_CTRL = I2S_CTRL_I2SLBM;
#else
  I2S_CTRL = 0;
#endif
  I2S_ICR = 0xff;

  SSP_CTRL0 = 
    (1<<4)			/* TI mode */
    |(2<<8)			/* CPD == 2 */
    |((16 - 1)<<0);		/* 16 bit frame */
  //  SSP_CTRL1 = 0;

  /* * think that this is very very wrong */
  ssp_set_speed ();		/* This is derived from Eric's method */

  SSP_CTRL1 = SSP_CTRL1_MS
#if defined (USE_LOOPBACK_SSP)
    | SSP_CTRL1_LBM
#endif
    ;

  __REG (RCPC_PHYS + RCPC_CTRL) |= RCPC_CTRL_UNLOCK;
  MASK_AND_SET (__REG (RCPC_PHYS + RCPC_CTRL),  /* System osc. -> CLKOUT */
		3<<5, 0<<5);
//  __REG (RCPC_PHYS + RCPC_PCLKSEL1) |= 1<<1;	/* System osc. -> SSP clock */
  __REG (RCPC_PHYS + RCPC_PCLKSEL1) &= ~(1<<1);	/* HCLK -> SSP clock */
//#if defined (USE_CPU_MASTER)
////  __REG (RCPC_PHYS + RCPC_SSPPRE) = (8>>1);	/* Prescalar = 8 */
//  __REG (RCPC_PHYS + RCPC_SSPPRE) = (1>>1);	/* Prescalar = 1 */
//#else
//  __REG (RCPC_PHYS + RCPC_SSPPRE) = (1>>1);	/* Prescalar = 1 */
//#endif
  __REG (RCPC_PHYS + RCPC_PCLKCTRL1) &= ~(1<<1); /* Enable SSP clock */
  __REG (RCPC_PHYS + RCPC_CTRL) &= ~RCPC_CTRL_UNLOCK;

  //#if defined (USE_CPU_MASTER)
  //  SSP_CPSR = 16;
  //#else
  //  SSP_CPSR = 2; /* Smallest prescalar is 2 */
  //#endif

  //#if defined (USE_I2S)
  //  I2S_CTRL = 0;
  //#endif

  //  SSP_CTRL1 |= (1<<1); /* Enable */

  execute_spi_command (CMD (CODEC_RESET, 0), 16);
  msleep (600);
  execute_spi_command (CMD (CODEC_POWER_CTRL, 7), 16);
  execute_spi_command (CMD (CODEC_DIGITAL_FORMAT, 0x52), 16);
  execute_spi_command (CMD (CODEC_DIGITAL_ACTIVATE, 0x1), 16);
  execute_spi_command (CMD (CODEC_DIGITAL_CTRL, 0x0), 16);
//  execute_spi_command (CMD (CODEC_SAMPLE_RATE, 0x20 | 0x40), 16); /* 22KHz */

#if defined (USE_8KHZ)
  execute_spi_command (CMD (CODEC_SAMPLE_RATE, 0xb<<2), 16); /* 8KHz */
  //  execute_spi_command (CMD (CODEC_SAMPLE_RATE, 0x60), 16);
#endif

#if defined (USE_22KHZ)
  execute_spi_command (CMD (CODEC_SAMPLE_RATE, 0x60), 16);
  //  execute_spi_command (CMD (CODEC_SAMPLE_RATE, 0xb<<2), 16);
#endif

#if defined (USE_44KHZ)
//  execute_spi_command (CMD (CODEC_SAMPLE_RATE, 0x3c | 0xc0), 16); /* 44KHz */
  execute_spi_command (CMD (CODEC_SAMPLE_RATE, 0x20), 16); /* 44KHz */
#endif

  execute_spi_command (CMD (CODEC_ROUT_VOLUME, 0xff), 16);
  execute_spi_command (CMD (CODEC_LOUT_VOLUME, 0xff), 16);

#if defined (USE_DMA)
  __REG (RCPC_PHYS + RCPC_CTRL) |= RCPC_CTRL_UNLOCK;
  __REG (RCPC_PHYS | RCPC_AHBCLKCTRL) &= ~(1<<0); /* Enable DMA AHB clock */
  __REG (RCPC_PHYS + RCPC_CTRL) &= ~RCPC_CTRL_UNLOCK;

  DMA0_CTRL
    = (0<<13)			/* Peripheral source */
    | (0<<9)			/* Load base addresses on start  */
    | (1<<7)			/* Destination size 2 bytes */
    | (1<<3)			/* Source size is 2 bytes */
    | (0<<2)			/* Destination fixed */
    | (0<<1);			/* Source fixed */

  DMA1_CTRL
    = (1<<13)			/* Peripheral destination */
#if defined (USE_16)
    | (1<<7)			/* Destination size 2 bytes */
    | (1<<3)			/* Source size is 2 bytes */
//    | (1<<5)			/* Source burst 4 incrementing */
#else
    | (0<<7)			/* Destination size 1 bytes */
    | (0<<3)			/* Source size is 1 byte */
//    | (1<<5)			/* Source burst 4 incrementing */
#endif
    | (0<<9)			/* Wrapping: Load base addresses on start  */
    | (0<<2)			/* Destination fixed */
    | (1<<1);			/* Source incremented */

  DMA_CLR   = 0xff;
  DMA_MASK |= (1<<1);		/* Enable of DMA channel 1 */
#endif
  
  //  SSP_IMSC = (1<<3);		/* Allow TX interrupts */

#if defined (USE_DMA)
  SSP_DCR = 0
    | (1<<1)			/* TX DMA enabled */
    | (1<<0)			/* RX DMA enabled */
    ;
#else
  SSP_DCR = 0;
#endif

  usleep (10);
  printf ("ssp: status 0x%lx\r\n", __REG (SSP_PHYS | SSP_SR));
}

#else

static void codec_init (void)
{
  printf ("codec_init elf\r\n");

  SSP_CTRL0 = 0;
  SSP_CTRL1 = 0;

  __REG (RCPC_PHYS + RCPC_CTRL) |= RCPC_CTRL_UNLOCK;
  MASK_AND_SET (__REG (RCPC_PHYS + RCPC_CTRL),  /* System osc. -> CLKOUT */
		3<<5, 0<<5);
  __REG (RCPC_PHYS + RCPC_PCLKSEL1) |= 1<<1;	/* System osc. -> SSP clock */
//  __REG (RCPC_PHYS + RCPC_PCLKSEL1) &= ~(1<<1);	/* HCLK -> SSP clock */
#if defined (USE_CPU_MASTER)
//  __REG (RCPC_PHYS + RCPC_SSPPRE) = (8>>1);	/* Prescalar = 8 */
  __REG (RCPC_PHYS + RCPC_SSPPRE) = (1>>1);	/* Prescalar = 1 */
#else
  __REG (RCPC_PHYS + RCPC_SSPPRE) = (1>>1);	/* Prescalar = 1 */
#endif
  __REG (RCPC_PHYS + RCPC_PCLKCTRL1) &= ~(1<<1); /* Enable SSP clock */
  __REG (RCPC_PHYS + RCPC_CTRL) &= ~RCPC_CTRL_UNLOCK;

#if defined (USE_CPU_MASTER)
  SSP_CPSR = 16;
#else
  SSP_CPSR = 2; /* Smallest prescalar is 2 */
#endif

#if defined (USE_I2S)
  I2S_CTRL = 0;
#endif

  SSP_CTRL1 |= (1<<1); /* Enable */

		/* Wait for TX/RX FIFOs to empty */
  while ((SSP_SR & SSP_SR_TFE) == 0)
    ;
  while (SSP_SR & SSP_SR_RNE)
    SSP_DR;

  SSP_ICR = 0xff;		/* Clear interrupts */
  SSP_CTRL1 &= ~(1<<1);		/* Disable */

  SSP_IMSC &= 0xff;		/* Mask everything */

  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_MUXCTL5),
		(3<<6)|(3<<4)|(3<<2)|(3<<0),
		(1<<6)|(1<<4)|(1<<2)|(1<<0));	/* SSP/I2S signals */
//  __REG (IOCON_PHYS + IOCON_RESCTL5) &= ~((3<<6)|(3<<4)|(3<<2)|(3<<0));

  /* *** this is where Jun Li's code leave the I2S hardware */

#if 0
  /* Master mode */

    /* Set SCLK polarity as normal polarity */
    ssp_ioctl(dev_ssp, SSP_SET_SCLK_POLARITY, 0);
    
    /* Set SCLK phase as normal */
    ssp_ioctl(dev_ssp, SSP_SET_SCLK_PHASE, 0);
    
    /* Set SSP working in loopback mode */
    ssp_ioctl(dev_ssp, SSP_LOOP_BACK_ENABLE, 1);
    
    /* Enable SSP */
    ssp_ioctl(dev_ssp, SSP_ENABLE, 1);
    
    /* Set SSP frame format */
    ssp_ioctl(dev_ssp, SSP_SET_FRAME_FORMAT, SSP_MODE_TI);
    
    /* Set SSP data size */
    ssp_ioctl(dev_ssp, SSP_SET_DATA_SIZE, 16);
    
    /* Set SSP speed in us */
    ssp_ioctl(dev_ssp, SSP_SET_SPEED, freq*16);
    
    /* Init SSP DMA mode */
    ssp_ioctl(dev_ssp, SSP_DMA_TX_RX_INIT, 0);
#endif

#if 0
    /* Set SSP frame format */
    ssp_ioctl(dev_ssp, SSP_SET_FRAME_FORMAT, SSP_MODE_TI);
    
    /* Set SSP in slave mode */
    ssp_ioctl(dev_ssp, SSP_SET_SLAVE, 1);
    
     /* Set SSP data size */
    ssp_ioctl(dev_ssp, SSP_SET_DATA_SIZE, 16);
    
    /* Init SSP DMA mode */
    ssp_ioctl(dev_ssp, SSP_DMA_TX_INIT, 0);
        
    /* Set source address */
    ssp_ioctl(dev_ssp, SSP_DMA_SET_SOURCE, (INT_32)source);

    /* Set count */
    ssp_ioctl(dev_ssp, SSP_DMA_SET_COUNT, count);
            
     /* Enable TX FIFO int */
    //ssp_ioctl(dev_ssp, SSP_INT_ENABLE, SSP_TRANSMIT_FIFO_INT);
            
    ssp_data_source = source;
    
    ssp_data_left = count;
    
    ssp_data_current = 0;

#endif

    MASK_AND_SET (SSP_CTRL0, (3<<4), (1<<4)); /* TI mode */
#if defined (USE_CPU_MASTER)
    SSP_CTRL1 &= ~(1<<2);	/* CPU as master */
#else
    SSP_CTRL1 |=  (1<<2);	/* CPU as slave */
#endif

#if defined (USE_LOOPBACK_SSP)
    SSP_CTRL1 |= SSP_CTRL1_LBM;	/* Loopback */
#endif

#if defined (USE_16)
    MASK_AND_SET (SSP_CTRL0, 0xf, ((16 - 1) & 0xf));
#else
    MASK_AND_SET (SSP_CTRL0, 0xf, ((8  - 1) & 0xf));
#endif    

#if defined (USE_DMA)
  __REG (RCPC_PHYS + RCPC_CTRL) |= RCPC_CTRL_UNLOCK;
  __REG (RCPC_PHYS | RCPC_AHBCLKCTRL) &= ~(1<<0); /* Enable DMA AHB clock */
  __REG (RCPC_PHYS + RCPC_CTRL) &= ~RCPC_CTRL_UNLOCK;

  DMA0_CTRL
    = (0<<13)			/* Peripheral source */
    | (0<<9)			/* Load base addresses on start  */
    | (1<<7)			/* Destination size 2 bytes */
    | (1<<3)			/* Source size is 2 bytes */
    | (0<<2)			/* Destination fixed */
    | (0<<1);			/* Source fixed */

  DMA1_CTRL
    = (1<<13)			/* Peripheral destination */
#if defined (USE_16)
    | (1<<7)			/* Destination size 2 bytes */
    | (1<<3)			/* Source size is 2 bytes */
//    | (1<<5)			/* Source burst 4 incrementing */
#else
    | (0<<7)			/* Destination size 1 bytes */
    | (0<<3)			/* Source size is 1 byte */
//    | (1<<5)			/* Source burst 4 incrementing */
#endif
    | (0<<9)			/* Wrapping: Load base addresses on start  */
    | (0<<2)			/* Destination fixed */
    | (1<<1);			/* Source incremented */

  DMA_CLR   = 0xff;
  DMA_MASK |= (1<<1);		/* Enable of DMA channel 1 */
#endif
  
  SSP_IMSC = (1<<3);		/* Allow TX interrupts */

#if defined (USE_DMA)
  SSP_DCR = 0
    | (1<<1)			/* TX DMA enabled */
    | (1<<0)			/* RX DMA enabled */
    ;
#else
  SSP_DCR = 0;
#endif

//  codec_mute ();

  execute_spi_command (CMD (CODEC_RESET, 0), 16);
  execute_spi_command (CMD (CODEC_DIGITAL_CTRL, 
			    0
//			    |(2<<1) /* 44.1 kHz deemphasis */
			    ), 16);
  execute_spi_command (CMD (CODEC_DIGITAL_FORMAT,
			    0
#if defined (USE_CPU_MASTER)
			    | (0<<6) /* Slave */
#else
			    | (1<<6) /* Master */
#endif
			    | (1<<4) /* LRP: right channel on LRC low */
			    | (0<<2) /* 16 bit */
//			    | (3<<2) /* 32 bit */
#if defined (USE_I2S)
			    | (2<<0) /* I2S format */
#else
			    | (3<<0) /* DSP format */
#endif
			    ), 16);
  //  usleep (1);
  codec_configure (SAMPLE_FREQUENCY, 16);
  //  usleep (1);
//  execute_spi_command (CMD (CODEC_DIGITAL_ACTIVATE, (1<<0)), 16);

  usleep (10);
  printf ("ssp: status 0x%lx\r\n", __REG (SSP_PHYS | SSP_SR));
}

#endif

static void codec_release (void)
{
  __REG (RCPC_PHYS + RCPC_CTRL) |= RCPC_CTRL_UNLOCK;
  __REG (RCPC_PHYS + RCPC_PCLKCTRL1) |= (1<<1);	/* Disable SSP clock */
  __REG (RCPC_PHYS + RCPC_CTRL) &= ~RCPC_CTRL_UNLOCK;
}


#if defined (USE_16)
typedef unsigned short buffer_t;
#else
typedef unsigned char buffer_t;
#endif
static buffer_t __attribute__((section("pcm.data"))) buffer[128*1024];

/* convert_source
   
   returns the number of samples used in the buffer. 

*/

static int convert_source (void)
{
  int source_samples = C_SAMPLES;
  int buffer_samples = sizeof (buffer)/sizeof (buffer[0]); 
  sample_t* rgb = (sample_t*) &rgbPCM[0];
  int is;
  int ib;

#if defined (SOURCE_MONO) && defined (USE_STEREO)
  source_samples *= 2;
#endif

#if defined (SOURCE_STEREO) && !defined (USE_STEREO)
  source_samples /= 2;
#endif

  if (buffer_samples > source_samples)
    buffer_samples = source_samples;

  printf ("source");
#if defined (SOURCE_SIGNED)
  printf (" signed");
#else
  printf (" unsigned");
#endif
#if defined (SOURCE_8BIT)
  printf (" 8 bit");
#endif
#if defined (SOURCE_16BIT)
  printf (" 16 bit");
#endif
#if defined (SOURCE_MONO)
  printf (" mono");
#endif
#if defined (SOURCE_STEREO)
  printf (" stereo");
#endif
  printf (" %d samples to", source_samples);
#if defined (USE_16)
  printf (" 16 bit");
#else
  printf (" 8 bit");
#endif
#if defined (USE_STEREO)
  printf (" stereo");
#else
  printf (" mono");
#endif
  printf (" %d samples\r\n", buffer_samples);

  is = 0;
  ib = 0;
  while (ib < buffer_samples) {
    buffer_t v;
#if defined (SOURCE_8BIT) && !defined (USE_16)
    v = rgb[is++];
#elif defined (SOURCE_8BIT) &&  defined (USE_16)
    v = rgb[is]<<8 | rgb[is];
    ++is;
#elif defined (SOURCE_16BIT) && !defined (USE_16)
    v = rgb[is++]>>8;
#elif defined (SOURCE_16BIT) &&  defined (USE_16)
    v = rgb[is++];
#else
    error "Whoops!";
#endif

    /* Convert to unsigned */
#if defined (SOURCE_SIGNED)
# if defined (USE_16)
    {
      int l = v + 0x8000;
      v = (unsigned long) l;
    }
# else
    {
      int l = v + 0x80;
      v = (unsigned long) l;
    }
# endif
#endif

    /* Modulate everyone for the sake of headphones */
    v >>= 1;
    v >>= 1;

# if defined (USE_CPU_MASTER)
    v &= ~1;			/* As master, CPU drops LS bit */
# endif


#if defined (USE_E5)
# if defined (USE_16)
    if (is & 1)
      v = 0x80bb;
    else
      v = 0x7f02;
# else
    if (is & 1)
      v = 0x8b;
    else
      v = 0x7f;
# endif
#endif

#if defined (USE_E5_RIGHT)
# if defined (USE_16)
    if (is & 1)
      v = 0x80bb;
# else
    if (is & 1)
      v = 0x8b;
# endif
#endif

#if defined (SOURCE_MONO)     && !defined (USE_STEREO)
    buffer[ib++] = v;
#elif defined (SOURCE_STEREO) &&  defined (USE_STEREO)
    buffer[ib++] = v;
#elif defined (SOURCE_MONO)   &&  defined (USE_STEREO)
    buffer[ib++] = v;
    buffer[ib++] = v;
#elif defined (SOURCE_STEREO) && !defined (USE_STEREO)
    if ((is & 1) == 0)
      buffer[ib++] = v;
#else
error
#endif
  }
  printf ("conversion complete\r\n");
  
  return buffer_samples;
} 

#if defined (USE_ERIC)

static int cmd_codec_test (int argc, const char** argv)
{
  int i;
  int samples = convert_source ();
#if defined (USE_DMA_CAP)
  static unsigned long cap;
#endif

  printf ("I2S_CTRL 0x%lx\r\n", I2S_CTRL);

  while ((SSP_SR & SSP_SR_TNF) == SSP_SR_TNF) {
    SSP_DR = buffer[0];
    if (I2S_RIS & 0x70) {
      printf ("priming I2S_RIS %lx\r\n", I2S_RIS);
    }
  }
  printf ("primed and going\r\n");
  printf ("I2S_CTRL 0x%lx\r\n", I2S_CTRL);

  SSP_CTRL1 |= SSP_CTRL1_SSE;
  I2S_CTRL  |= I2S_CTRL_I2SEN;
  printf ("I2S_CTRL 0x%lx\r\n", I2S_CTRL);

#if defined (USE_DMA)
//  __REG (RCPC_PHYS + RCPC_CTRL) |= RCPC_CTRL_UNLOCK;
//  __REG (RCPC_PHYS | RCPC_AHBCLKCTRL) &= ~(1<<0); /* Enable DMA AHB clock */
//  __REG (RCPC_PHYS + RCPC_CTRL) &= ~RCPC_CTRL_UNLOCK;

  //  SSP_IMSC |= 0
  //    | (1<<3)
  //    | (1<<2);			/* Interrupts needed for DMA */
//  SSP_DCR  |= 0
//    | (1<<1)			/* TX DMA enabled */
//    | (1<<0)			/* RX DMA enabled */
//    ;

//  DMA_MASK	= 0; /* Mask all interrupts */
//  DMA_CLR	= 0xff; /* Clear pending interrupts */

//  DMA0_CTRL     = 0; /* Disable */
#if defined (USE_DMA_CAP)
  DMA0_SOURCELO =  SSP_DR_PHYS        & 0xffff;
  DMA0_SOURCEHI = (SSP_DR_PHYS >> 16) & 0xffff;
  DMA0_DESTLO   =  ((unsigned long)&cap)        & 0xffff;
  DMA0_DESTHI   = (((unsigned long)&cap) >> 16) & 0xffff;
  DMA0_MAX	= 0xffff;
#endif
//  DMA0_CTRL
//    = (0<<13)			/* Peripheral source */
//    | (0<<9)			/* Load base addresses on start  */
//    | (1<<7)			/* Destination size 2 bytes */
//    | (1<<3)			/* Source size is 2 bytes */
//    | (0<<2)			/* Destination fixed */
//    | (0<<1);			/* Source fixed */

//  DMA1_CTRL     = 0; /* Disable */
  DMA1_SOURCELO =  ((unsigned long)buffer)        & 0xffff;
  DMA1_SOURCEHI = (((unsigned long)buffer) >> 16) & 0xffff;
  DMA1_DESTLO   =  SSP_DR_PHYS        & 0xffff;
  DMA1_DESTHI   = (SSP_DR_PHYS >> 16) & 0xffff;
  DMA1_MAX	= samples;
//  DMA1_CTRL
//    = (1<<13)			/* Peripheral destination */
//#if defined (USE_16)
//    | (1<<7)			/* Destination size 2 bytes */
//    | (1<<3)			/* Source size is 2 bytes */
////    | (1<<5)			/* Source burst 4 incrementing */
//#else
//    | (0<<7)			/* Destination size 1 bytes */
//    | (0<<3)			/* Source size is 1 byte */
//    | (1<<5)			/* Source burst 4 incrementing */
//#endif
//    | (0<<9)			/* Wrapping: Load base addresses on start  */
//    | (0<<2)			/* Destination fixed */
//    | (1<<1);			/* Source incremented */

//  return 0;			/* *** FIXME */

#if defined (USE_DMA_CAP)
  DMA0_CTRL |= (1<<0);		/* Enable RX DMA */
#endif
  DMA1_CTRL |= (1<<0);		/* Enable TX DMA */

  //  codec_enable ();

				/* Wait for completion */
  while ((DMA_STATUS & (1<<1)) == 0)
    ;

#else

  //  codec_unmute ();

  //  codec_enable ();

  {
    int j = 
#if defined USE_LOOPS
      USE_LOOPS
#else
      1
#endif
      ;
    while (j--) {
      for (i = 0; i < samples; ++i) {
	/* Wait for room in the FIFO */
	while (!(SSP_SR & SSP_SR_TNF))
	  ;
	SSP_DR = buffer[i];
      }
    }
  }

#endif /* !USE_DMA */

  //  codec_disable ();
  //  codec_mute ();
  return 0;
}

#else

static int cmd_codec_test (int argc, const char** argv)
{
  int samples = convert_source ();
  int index;			/* Index for DMA */
  int count;
  int loops = 
#if defined USE_LOOPS
      USE_LOOPS
#else
      1
#endif
    ;

#if defined (USE_DMA)
  codec_unmute ();
  codec_enable ();

//  __REG (RCPC_PHYS + RCPC_CTRL) |= RCPC_CTRL_UNLOCK;
//  __REG (RCPC_PHYS | RCPC_AHBCLKCTRL) &= ~(1<<0); /* Enable DMA AHB clock */
//  __REG (RCPC_PHYS + RCPC_CTRL) &= ~RCPC_CTRL_UNLOCK;

  //  SSP_IMSC |= 0
  //    | (1<<3)
  //    | (1<<2);			/* Interrupts needed for DMA */
//  SSP_DCR  |= 0
//    | (1<<1)			/* TX DMA enabled */
//    | (1<<0)			/* RX DMA enabled */
//    ;

//  DMA_MASK	= 0; /* Mask all interrupts */
//  DMA_CLR	= 0xff; /* Clear pending interrupts */

//  DMA0_CTRL     = 0; /* Disable */
#if defined (USE_DMA_CAP)
  DMA0_SOURCELO =  SSP_DR_PHYS        & 0xffff;
  DMA0_SOURCEHI = (SSP_DR_PHYS >> 16) & 0xffff;
  DMA0_DESTLO   =  ((unsigned long)&cap)        & 0xffff;
  DMA0_DESTHI   = (((unsigned long)&cap) >> 16) & 0xffff;
  DMA0_MAX	= 0xffff;
#endif
//  DMA0_CTRL
//    = (0<<13)			/* Peripheral source */
//    | (0<<9)			/* Load base addresses on start  */
//    | (1<<7)			/* Destination size 2 bytes */
//    | (1<<3)			/* Source size is 2 bytes */
//    | (0<<2)			/* Destination fixed */
//    | (0<<1);			/* Source fixed */

  //  msleep (10);			/* Fill the pipe */

  SSP_CTRL1 |= SSP_CTRL1_SSE;		/* Enable SSP  */

#if defined (USE_I2S)
  I2S_CTRL |= 0
    | I2S_CTRL_I2SEN
#if defined (USE_LOOPBACK_I2S)
    | I2S_CTRL_I2SLBM
#endif
    ;
#endif

 restart:
  index = 0;

 play_more:
  count = samples;
  if (index + count > samples)
    count = samples - index;
  if (count > 65534)
    count = 65534;

  DMA1_CTRL     &= ~(1<<0); /* Disable */
  DMA1_SOURCELO =  ((unsigned long)(buffer + index))        & 0xffff;
  DMA1_SOURCEHI = (((unsigned long)(buffer + index)) >> 16) & 0xffff;
  DMA1_DESTLO   =  SSP_DR_PHYS        & 0xffff;
  DMA1_DESTHI   = (SSP_DR_PHYS >> 16) & 0xffff;
  DMA1_MAX	= count;
//  DMA1_CTRL
//    = (1<<13)			/* Peripheral destination */
//#if defined (USE_16)
//    | (1<<7)			/* Destination size 2 bytes */
//    | (1<<3)			/* Source size is 2 bytes */
////    | (1<<5)			/* Source burst 4 incrementing */
//#else
//    | (0<<7)			/* Destination size 1 bytes */
//    | (0<<3)			/* Source size is 1 byte */
//    | (1<<5)			/* Source burst 4 incrementing */
//#endif
//    | (0<<9)			/* Wrapping: Load base addresses on start  */
//    | (0<<2)			/* Destination fixed */
//    | (1<<1);			/* Source incremented */

//  return 0;			/* *** FIXME */

#if defined (USE_DMA_CAP)
  DMA0_CTRL |= (1<<0);		/* Enable RX DMA */
#endif
  DMA1_CTRL |= (1<<0);		/* Enable TX DMA */

    DMA_CLR = 0xff;

				/* Wait for completion */
  while ((DMA_STATUS & (1<<1)) == 0)
    ;

  index += count;
  if (index < samples)
    goto play_more;

  if (loops--) {
    goto restart;
  }

#else

  codec_unmute ();
  codec_enable ();

  printf ("I2S_CTRL 0x%lx\r\n", I2S_CTRL);

  while ((SSP_SR & SSP_SR_TNF) == SSP_SR_TNF) {
    SSP_DR = buffer[0];
    if (I2S_RIS & 0x70) {
      printf ("priming I2S_RIS %lx\r\n", I2S_RIS);
    }
  }
  printf ("primed and going\r\n");
  printf ("I2S_CTRL 0x%lx\r\n", I2S_CTRL);

  SSP_CTRL1 |= (1<<1);		/* Enable SSP */
#if defined (USE_I2S)
  I2S_CTRL
    = I2S_CTRL_I2SEN
#if defined (USE_LOOPBACK_I2S)
    | I2S_CTRL_I2SLBM
#endif
    ;
#endif

  //  codec_enable ();

  {
    int j = 
#if defined USE_LOOPS
      USE_LOOPS
#else
      1
#endif
      ;
    while (j--) {
      int i;
      for (i = 0; i < samples; ++i) {
	/* Wait for room in the FIFO */
	while (!(SSP_SR & SSP_SR_TNF))
	  ;
	SSP_DR = buffer[i];
      }
    }
  }

#endif /* !USE_DMA */

  codec_disable ();
  codec_mute ();
  return 0;
}
#endif

static __service_7 struct service_d lpd79524_codec_service = {
  .init = codec_init,
  .release = codec_release,
};

static __command struct command_d c_codec = {
  .command = "codec",
  .description = "test codec",
  .func = cmd_codec_test,
};
