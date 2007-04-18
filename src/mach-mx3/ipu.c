/* ipu.c

   written by Marc Singer
   17 Apr 2007

   Copyright (C) 2007 Marc Singer

   -----------
   DESCRIPTION
   -----------

   o I2C Timing.  This code appears to have a slightly different wave
     form from the code in Matt Jakuc's test code.  The start of the
     frame, signaled by the initial falling clock pulse from the
     master has a long delay in this code, but a very short delay in
     Matt's.  I'm not sure why and it doesn't appear to make a
     difference in the correctness of the I2C frames.

   o Releasing the I2C bus.  It appears to take some time for the bus
     to go not-busy once it is released (in i2c_stop).  The trouble
     with this code is that we should allow for the bus to be taken
     over by another master.  In this boot loader, it isn't really an
     issue, but a driver should cope with the fact that sending STOP
     is enough and we really need to wait for the bus to go idle when
     we want to start a transaction.

   o Calculating the IFDR divisor.  Unfortunately, the Freescale I2C
     core doesn't use a simple divisor register to divide the base
     frequency of 66MHz.  We're using a divisor (1280) that yields
     52KHz as the I2C clock.  If we want to be able to calculate the
     divisor from the target frequency, we'll have a tough row to hoe.

   o I2C Node Addressing.  The I2C bus uses a seven bit address field
     where the node address is in the upper seven bits of the byte and
     the R/W bit is in the lowest bit.  The addresses passed around
     are the 'real' address of 0-127.

   o I2C_I2SR_RXAK.  This bit is set when the ACK is not found on the
     transfer of a byte.  It really should be named NRXAK.  We use the
     name that exists in the documentation.

*/

#include <config.h>
#include <apex.h>
#include <command.h>
#include <service.h>
#include <error.h>
#include <linux/string.h>

#include "hardware.h"

#undef ENTRY
//#define ENTRY printf ("%s\n", __FUNCTION__)
#define ENTRY

	/* Definitions for GPIO pins to make the code readable. */
#define PIN_SENSOR_PWR_EN     MX31_PIN_CSI_D4
#define PIN_SENSOR_BUF_EN     MX31_PIN_KEY_ROW4
#define PIN_CMOS_STBY	      MX31_PIN_KEY_ROW7
#define PIN_NCMOS_RESET	      MX31_PIN_KEY_ROW6
#define PIN_CMOS_EXPOSURE     MX31_PIN_KEY_ROW5
#define PIN_ILLUMINATION_EN1  MX31_PIN_KEY_COL5
#define PIN_ILLUMINATION_EN2  MX31_PIN_KEY_COL4

#define I2C_IFDR_V	      (0x19)		/* Divisor of 1280 */

#if 0
static const char* i2c_status_decode (int sr)
{
  static char sz[80];
  *sz = 0;
  if (sr & I2C_I2SR_ICF)
    strcat (sz, " ICF");
  if (sr & I2C_I2SR_IAAS)
    strcat (sz, " IAAS");
  if (sr & I2C_I2SR_IBB)
    strcat (sz, " IBB");
  if (sr & I2C_I2SR_IAL)
    strcat (sz, " IAL");
  if (sr & I2C_I2SR_SRW)
    strcat (sz, " SRW");
  if (sr & I2C_I2SR_IIF)
    strcat (sz, " IIF");
  if (sr & I2C_I2SR_RXAK)
    strcat (sz, " RXAK");

  return sz;
}
#endif

static void i2c_stop (void)
{
  ENTRY;

  I2C_I2CR &= ~(I2C_I2CR_MSTA | I2C_I2CR_MTX);
  I2C_I2SR = 0;

  while (I2C_I2SR & I2C_I2SR_IBB) 	/* Wait for STOP condition */
    ;
}

static void i2c_setup (void)
{
  ENTRY;

  IOMUX_PIN_CONFIG_FUNC  (MX31_PIN_I2C_CLK);
  IOMUX_PIN_CONFIG_FUNC  (MX31_PIN_I2C_DAT);

  I2C_IFDR = I2C_IFDR_V;

  I2C_I2SR = 0;
  i2c_stop ();
  I2C_I2CR = I2C_I2CR_IEN;	/* Enable I2C */
}

static void i2c_setup_sensor_i2c (void)
{
  ENTRY;

  IOMUX_PIN_CONFIG_GPIO	 (PIN_SENSOR_PWR_EN);
  GPIO_PIN_CONFIG_OUTPUT (PIN_SENSOR_PWR_EN);
  GPIO_PIN_SET		 (PIN_SENSOR_PWR_EN);	/* Enable sensor power */

  IOMUX_PIN_CONFIG_GPIO  (PIN_SENSOR_BUF_EN);
  GPIO_PIN_CONFIG_OUTPUT (PIN_SENSOR_BUF_EN);
  GPIO_PIN_SET		 (PIN_SENSOR_BUF_EN);	/* Enable sensor signal bufr */

  usleep (1000);		/* delay 1ms while camera powers on */

  IOMUX_PIN_CONFIG_FUNC  (MX31_PIN_CSI_PIXCLK);
  IOMUX_PIN_CONFIG_FUNC  (MX31_PIN_CSI_MCLK);

	/* Post divider for CSI. */
  MASK_AND_SET (CCM_PDR0, (0x1ff << 23), (0x58 << 23));
}

static void ipu_setup_sensor (void)
{
  ENTRY;

  IOMUX_PIN_CONFIG_GPIO  (PIN_CMOS_STBY);
  GPIO_PIN_CONFIG_OUTPUT (PIN_CMOS_STBY);
  GPIO_PIN_SET		 (PIN_CMOS_STBY);	/* Put camera in standby */

  IOMUX_PIN_CONFIG_GPIO  (PIN_NCMOS_RESET);
  GPIO_PIN_CONFIG_OUTPUT (PIN_NCMOS_RESET);
  GPIO_PIN_SET		 (PIN_NCMOS_RESET);	/* Prep camera reset signal */

  IOMUX_PIN_CONFIG_GPIO  (PIN_CMOS_EXPOSURE);
  GPIO_PIN_CONFIG_OUTPUT (PIN_CMOS_EXPOSURE);
  GPIO_PIN_CLEAR	 (PIN_CMOS_EXPOSURE);

  IOMUX_PIN_CONFIG_GPIO  (PIN_ILLUMINATION_EN1);
  GPIO_PIN_CONFIG_OUTPUT (PIN_ILLUMINATION_EN1);
  GPIO_PIN_SET		 (PIN_ILLUMINATION_EN1);

  IOMUX_PIN_CONFIG_GPIO  (PIN_ILLUMINATION_EN2);
  GPIO_PIN_CONFIG_OUTPUT (PIN_ILLUMINATION_EN2);
  GPIO_PIN_SET		 (PIN_ILLUMINATION_EN2);

  IOMUX_PIN_CONFIG_FUNC  (MX31_PIN_CSI_D15);
  IOMUX_PIN_CONFIG_FUNC  (MX31_PIN_CSI_D14);
  IOMUX_PIN_CONFIG_FUNC  (MX31_PIN_CSI_D13);
  IOMUX_PIN_CONFIG_FUNC  (MX31_PIN_CSI_D12);
  IOMUX_PIN_CONFIG_FUNC  (MX31_PIN_CSI_D11);
  IOMUX_PIN_CONFIG_FUNC  (MX31_PIN_CSI_D10);
  IOMUX_PIN_CONFIG_FUNC  (MX31_PIN_CSI_D9);
  IOMUX_PIN_CONFIG_FUNC  (MX31_PIN_CSI_D8);
  IOMUX_PIN_CONFIG_FUNC  (MX31_PIN_CSI_D7);
  IOMUX_PIN_CONFIG_FUNC  (MX31_PIN_CSI_D6);

  IOMUX_PIN_CONFIG_FUNC  (MX31_PIN_CSI_VSYNC);
  IOMUX_PIN_CONFIG_FUNC  (MX31_PIN_CSI_HSYNC);

  GPIO_PIN_CLEAR	 (PIN_NCMOS_RESET);
  udelay (1);
  GPIO_PIN_SET		 (PIN_NCMOS_RESET);
  udelay (1);
  GPIO_PIN_CLEAR	 (PIN_CMOS_STBY); /* Release camera from standby */
}


/* i2c_wait_for_interrupt

   waits for an interrupt and returns the status.

*/

static int i2c_wait_for_interrupt (void)
{
  ENTRY;
  while (!(I2C_I2SR & I2C_I2SR_IIF))	/* Wait for interrupt */
    ;

  I2C_I2SR = 0;				/* Only IIF can be cleared */
  return I2C_I2SR;
}


/* i2c_start

   sends the slave the frame START and target address.  Return value
   is 0 on success.

   *** FIXME: the check for IAL isn't really necessary since this is
   *** really a slave issue, I think.

*/

static int i2c_start (char address, int reading)
{
  ENTRY;
  address &= 0x7f;

  I2C_I2CR |= I2C_I2CR_MSTA;	/* Acquire bus */
  do {
    int sr = I2C_I2SR;
    if (sr & I2C_I2SR_IBB)	/* Arbitration won */
      break;
    if (sr & I2C_I2SR_IAL)	/* Arbitration lost */
      return 1;
    usleep (10);
  } while (1);
  I2C_I2SR = 0;
  I2C_I2CR |= I2C_I2CR_MTX;	/* Prepare for transmit */
  I2C_I2DR = (address << 1) | (reading ? 1 : 0);

				/* Address must be ACK'd */
  return (i2c_wait_for_interrupt () & I2C_I2SR_RXAK) ? 1 : 0;
}


/* i2c_write

   writes the buffer to the given address.  It returns 0 on success.

*/

static int i2c_write (char address, char* rgb, int cb)
{
  int i;
  ENTRY;
  if (I2C_I2SR & I2C_I2SR_IBB) {
    printf ("error: i2c bus busy %d\n", address);
    i2c_stop ();
    return 1;
  }

  if (i2c_start (address, 0)) {
    i2c_stop ();
    return 1;
  }

  I2C_I2CR |= I2C_I2CR_MTX;
  for (i = 0; i < cb; ++i) {
    I2C_I2DR = *rgb++;
    if (i2c_wait_for_interrupt () & I2C_I2SR_RXAK) {
      i2c_stop ();
      return 1;
    }
  }

  i2c_stop ();
  return 0;
}


static int cmd_ipu (int argc, const char** argv)
{
  ENTRY;
  if (argc < 2)
    return ERROR_PARAM;

  if (strcmp (argv[1], "i") == 0) {
    i2c_setup_sensor_i2c ();
    i2c_setup ();
    ipu_setup_sensor ();
  }

  if (strcmp (argv[1], "i2c") == 0) {
    int address;
    i2c_setup_sensor_i2c ();
    i2c_setup ();
    for (address = 0; address < 128; ++address) {
      char rgb[2] = { 0, 0 };
      int result = i2c_write (address, rgb, 2);
      if (result == 0)
	printf ("I2C device found at address %d (0x%x)\n", address, address);
    }
  }

  return 0;
}

static __command struct command_d c_ipu = {
  .command = "ipu",
  .func = (command_func_t) cmd_ipu,
  COMMAND_DESCRIPTION ("ipu test")
  COMMAND_HELP(
"ipu\n"
"  IPU tests.\n"
  )
};
