/* nas100d.h

   written by Rod Whitby
   05 Nov 2006

   based on nslu2.h
   written by Marc Singer
   11 Feb 2005

   Copyright (C) 2005 Marc Singer
   Copyright (C) 2006 Rod Whitby

   This program is licensed under the terms of the GNU General Public
   License as published by the Free Software Foundation.  Please refer
   to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__NAS100D_H__)
#    define   __NAS100D_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#define LED_MASK (0xf)

#define LED0	((0<<1)|(0<<0)|(1<<2)|(1<<3))
#define LED1	((1<<1)|(0<<0)|(1<<2)|(1<<3))
#define LED2	((0<<1)|(1<<0)|(1<<2)|(1<<3))
#define LED3	((1<<1)|(1<<0)|(1<<2)|(1<<3))
#define LED4	((0<<1)|(0<<0)|(0<<2)|(1<<3))
#define LED5	((1<<1)|(0<<0)|(0<<2)|(1<<3))
#define LED6	((0<<1)|(1<<0)|(0<<2)|(1<<3))
#define LED7	((1<<1)|(1<<0)|(0<<2)|(1<<3))

#define LED8	((0<<1)|(0<<0)|(1<<2)|(0<<3))
#define LED9	((1<<1)|(0<<0)|(1<<2)|(0<<3))
#define LEDa	((0<<1)|(1<<0)|(1<<2)|(0<<3))
#define LEDb	((1<<1)|(1<<0)|(1<<2)|(0<<3))
#define LEDc	((0<<1)|(0<<0)|(0<<2)|(0<<3))
#define LEDd	((1<<1)|(0<<0)|(0<<2)|(0<<3))
#define LEDe	((0<<1)|(1<<0)|(0<<2)|(0<<3))
#define LEDf	((1<<1)|(1<<0)|(0<<2)|(0<<3))

#define _L(l) MASK_AND_SET(GPIO_OUTR, LED_MASK, l)


#define GPIO_ER_OUTPUTS	 (1<<0)		/* WLAN LED */\
			|(1<<3)		/* Disk LED */\
			|(1<<15)	/* Power LED */

#define GPIO_ER_V	(0x1f30)	/* 0,1,6,7 are outputs */

#define GPIO_CLKR_V	(0x1ff01ff)	/* MUX14&15 33MHz clocks */

#define GPIO_I_LEDGREEN		0
#define GPIO_I_DISK		3
#define GPIO_I_RESETBUTTON	4
#define GPIO_I_I2C_SDA		5
#define GPIO_I_I2C_SCL		6
#define GPIO_I_PCI_INTD		8
#define GPIO_I_PCI_INTC		9
#define GPIO_I_PCI_INTB		10
#define GPIO_I_PCI_INTA		11
#define GPIO_I_POWEROFF		12
#define GPIO_I_PCI_RESET	13
#define GPIO_I_POWERBUTTON	14
#define GPIO_I_LEDBLUE		15

#endif  /* __NAS100D_H__ */
