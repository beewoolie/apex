/** @file genesi-efika.h

   written by Marc Singer
   2 Jul 2011

   Copyright (C) 2011 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (GENESI_EFIKA_H_INCLUDED)
#    define   GENESI_EFIKA_H_INCLUDED

/* ----- Includes */

#include "mx51-pins.h"

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

/* ----- Constants */

#define PIN_PWR_SW_REQ	(MX51_PIN_EIM_DTACK)
#define PIN_PMIC_IRQ    (MX51_PIN_GPIO1_6)
#define PIN_WDG         (MX51_PIN_DI1_PIN13)
#define PIN_SYS_PWROFF  (MX51_PIN_CSI2_VSYNC)
#define PIN_AC_ADAP_INS (MX51_PIN_DI1_D0_CS)

#define PIN_SDHC1_CD    (MX51_PIN_GPIO1_0)
#define PIN_SDHC1_WP    (MX51_PIN_GPIO1_1)
#define PIN_SDHC2_CD    (MX51_PIN_GPIO1_8)
#define PIN_SDHC2_WP    (MX51_PIN_GPIO1_7)


#define HUB_RESET_PIN		MX51_PIN_GPIO1_5
#define PMIC_INT_PIN		MX51_PIN_GPIO1_6
#define USB_PHY_RESET_PIN	MX51_PIN_EIM_D27

#define WIRELESS_SW_PIN		MX51_PIN_DI1_PIN12
#define WLAN_PWRON_PIN		MX51_PIN_EIM_A22
#define WLAN_RESET_PIN		MX51_PIN_EIM_A16
#define BT_PWRON_PIN		MX51_PIN_EIM_A17
#define WWLAN_PWRON_PIN		MX51_PIN_CSI2_D13

#define LCD_PWRON_PIN		MX51_PIN_CSI1_D9
#define LCD_BL_PWM_PIN		MX51_PIN_GPIO1_2
#define LCD_BL_PWRON_PIN	MX51_PIN_CSI2_D19
#define LVDS_RESET_PIN		MX51_PIN_DISPB2_SER_DIN

#define CAM_PWRON_PIN		MX51_PIN_EIM_CRE

#define BATT_LOW_PIN		MX51_PIN_DI1_PIN11
#define BATT_INS_PIN		MX51_PIN_DISPB2_SER_DIO
#define AC_ADAP_INS_PIN		MX51_PIN_DI1_D0_CS

#define AUD_MUTE_PIN		MX51_PIN_EIM_A23
#define HPJACK_INS_PIN		MX51_PIN_DISPB2_SER_RS

#define POWER_BTN_PIN		MX51_PIN_EIM_DTACK
#define SYS_PWROFF_PIN		MX51_PIN_CSI2_VSYNC
#define SYS_PWRGD_PIN		MX51_PIN_CSI2_PIXCLK


//#define SPI1_SS_PMIC            (0)
//#define SPI1_SS_FLASH           (1)


#endif  /* GENESI_EFIKA_H_INCLUDED */
