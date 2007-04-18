/* ipu.c

   written by Marc Singer
   17 Apr 2007

   Copyright (C) 2007 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#include <config.h>
#include <apex.h>
#include <command.h>
#include <service.h>
#include <error.h>
#include <linux/string.h>

#include "hardware.h"

#undef ENTRY
#define ENTRY printf ("%s\n", __FUNCTION__)

static void i2c_stop (void)
{
  ENTRY;
  I2C_I2CR &= ~I2C_I2CR_MSTA;
}

static void i2c_setup (void)
{
  ENTRY;

  printf ("i2c %x\n", MX31_PIN_I2C_CLK);
  printf ("iomux address 0x%x\n", _PIN_MUX_R (MX31_PIN_I2C_CLK));
  printf ("iomux's 0x%lx\n", __REG (0x43fac0a0));

  IOMUX_PIN_CONFIG_FUNC (MX31_PIN_I2C_CLK);
  IOMUX_PIN_CONFIG_FUNC (MX31_PIN_I2C_DAT);

  printf ("iomux's 0x%lx\n", __REG (0x43fac0a0));

  I2C_IFDR = 0x3c;		/* *** FIXME: calc! */
  printf ("IFDR 0x%x\n", I2C_IFDR);

  I2C_I2SR = 0;			/* Clear status */
  i2c_stop ();
  I2C_I2CR = I2C_I2CR_IEN;	/* Enable I2C */
}

static void i2c_setup_sensor (void)
{
  ENTRY;

  IOMUX_PIN_CONFIG_GPIO (MX31_PIN_CSI_D4);
  IOMUX_PIN_CONFIG_GPIO (MX31_PIN_KEY_ROW4);
  GPIO_PIN_CONFIG_OUTPUT (MX31_PIN_CSI_D4);	/* SENSOR_PWR_EN */
  GPIO_PIN_CONFIG_OUTPUT (MX31_PIN_KEY_ROW4);	/* SENSOR_BUF_EN */
  GPIO_PIN_SET (MX31_PIN_CSI_D4);		/* SENSOR_PWR_EN */
  GPIO_PIN_SET (MX31_PIN_KEY_ROW4);		/* SENSOR_BUF_EN */

  IOMUX_PIN_CONFIG_GPIO (MX31_PIN_KEY_ROW7);
  GPIO_PIN_CONFIG_OUTPUT (MX31_PIN_KEY_ROW7);
  GPIO_PIN_SET (MX31_PIN_KEY_ROW7); /* Release camera from standby */

  IOMUX_PIN_CONFIG_GPIO (MX31_PIN_KEY_ROW6);
  GPIO_PIN_CONFIG_OUTPUT (MX31_PIN_KEY_ROW6);
  GPIO_PIN_SET (MX31_PIN_KEY_ROW6); /* Release camera from standby */

  usleep (1000);		/* delay 1ms while camera powers on */

//  IOMUX_PIN_CONFIG_FUNC (MX31_PIN_CSI_D14);
//  IOMUX_PIN_CONFIG_FUNC (MX31_PIN_CSI_D15);
  IOMUX_PIN_CONFIG_FUNC (MX31_PIN_CSI_VSYNC);
  IOMUX_PIN_CONFIG_FUNC (MX31_PIN_CSI_HSYNC);
  IOMUX_PIN_CONFIG_FUNC (MX31_PIN_CSI_PIXCLK);
  IOMUX_PIN_CONFIG_FUNC (MX31_PIN_CSI_MCLK);

	/* Post divider for CSI. */
  MASK_AND_SET (CCM_PDR0, (0x1ff << 23), (0x58 << 23));

  GPIO_PIN_CLEAR (MX31_PIN_KEY_ROW6);
  udelay (1);
  GPIO_PIN_SET (MX31_PIN_KEY_ROW6);
  udelay (1);
  GPIO_PIN_CLEAR (MX31_PIN_KEY_ROW7); /* Release camera from standby */
}

/* i2c_wait_for_interrupt

   waits for an interrupt and returns the status.

*/

static int i2c_wait_for_interrupt (void)
{
  ENTRY;
  while (!(I2C_I2SR & I2C_I2SR_IIF))	/* Wait for interrupt */
    ;
  printf ("  returning %x\n", I2C_I2SR);
  return I2C_I2SR;
}


/* i2c_start

   sends the slave the frame START and target address.  Return value
   is 0 on success.

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
  } while (1);
  I2C_I2CR |= I2C_I2CR_MTX;	/* Prepare for transmit */
  I2C_I2SR = 0;
  I2C_I2DR = (address << 1) | (reading ? 1 : 0);

  {
    int sr = i2c_wait_for_interrupt ();

    printf ("  sr 0x%x\n", sr);
    return (sr & I2C_I2SR_RXAK) ? 1 : 0;
  }
}


/* i2c_write

   writes the buffer to the given address.  It returns 0 on success.

*/

static int i2c_write (char address, char* rgb, int cb)
{
  int i;
  ENTRY;
  if (i2c_start (address, 0)) {
    printf ("failed to start\n");
    return 1;
  }

  I2C_I2CR |= I2C_I2CR_MTX;
  for (i = 0; i < cb; ++i) {
    I2C_I2DR = *rgb++;
    if (i2c_wait_for_interrupt () & I2C_I2SR_RXAK) {
      i2c_stop ();
      printf ("failed to receive ack\n");
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
    i2c_setup ();
  }

  if (strcmp (argv[1], "i2c") == 0) {
    int address;
    i2c_setup_sensor ();
    i2c_setup ();
    for (address = 0; address < 4; ++address) {
      char rgb[2] = { 0, 0 };
      int result = i2c_write (address, rgb, 2);
      if (result == 0)
	printf ("success at address %d (0x%x)\n", address, address);
    }
  }

  if (strcmp (argv[1], "a") == 0) {
    int address = 0x5c >> 1;
    i2c_setup_sensor ();
    i2c_setup ();
    {
      char rgb[2] = { 0, 0 };
      int result = i2c_write (address, rgb, 2);
      if (result == 0)
	printf ("success at address %d (0x%x)\n", address, address);
    }
  }
  if (strcmp (argv[1], "b") == 0) {
    int address = 0;
    i2c_setup_sensor ();
    i2c_setup ();
    {
      char rgb[2] = { 0, 0 };
      int result = i2c_write (address, rgb, 2);
      if (result == 0)
	printf ("success at address %d (0x%x)\n", address, address);
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
