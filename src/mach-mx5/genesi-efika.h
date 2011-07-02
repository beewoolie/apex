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

//#include "mx51-pins.h"

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

/* ----- Constants */

#define IPG_CLK		 66500000	/*  66.5 MHz */
#define IPG_PER_CLK	665000000	/* 665   MHz */
#define UART_CLK	 66500000	/*  66.5 MHz */
#define CSPI_CLK	 54000000	/*  54   MHz */

//#define PIN_BOARD_ID1		MX31_PIN_GPIO1_4
//#define PIN_BOARD_ID2		MX31_PIN_GPIO1_6

//#define PIN_ILLUMINATION_EN1	MX31_PIN_KEY_COL5
//#define PIN_ILLUMINATION_EN2	MX31_PIN_KEY_COL4

	/* Definitions for GPIO pins to make the code readable. */
//#define PIN_SENSOR_PWR_EN	MX31_PIN_CSI_D4
//#define PIN_SENSOR_BUF_EN	MX31_PIN_KEY_ROW4
//#define PIN_CMOS_STBY		MX31_PIN_KEY_ROW7
//#define PIN_NCMOS_RESET		MX31_PIN_KEY_ROW6
 //#define PIN_CMOS_EXPOSURE	MX31_PIN_KEY_ROW5

//#define CCM_PDR0_V		(0x2c071d58) /* Custom CSI divider */

#endif  /* GENESI_EFIKA_H_INCLUDED */
