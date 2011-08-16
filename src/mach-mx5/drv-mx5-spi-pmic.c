/** @file drv-mx5-spi-pmic.c

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

  NOTES
  =====

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

#define USE_FIFO	/* Use FIFOs for more effcient IO */

#include <talk.h>

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
  .select             = mx5_spi_select,
  .talk               = true,
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
