/* nslu2.h

   written by Marc Singer
   11 Feb 2005

   Copyright (C) 2005 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA.

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__NSLU2_H__)
#    define   __NSLU2_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#define LED_MASK (0xf)		/* All LEDs */
#define LED_GREEN_MASK (0xe)	/* Green only LEDs */

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
#define _LG(l) MASK_AND_SET(GPIO_OUTR, LED_GREEN_MASK, l)


#define GPIO_ER_OUTPUTS	 (1<<0)		/* Status LED */\
			|(1<<1)		/* Ready LED */\
			|(1<<2)		/* Disk2 LED */\
			|(1<<3)		/* Disk1 LED */\
			|(1<<4)		/* Buzzer */

//#define GPIO_ER_V	(0x1f3f)	/* 0,1,6,7 are outputs */
#define GPIO_ER_V	(0x1f30)	/* 0,1,6,7 are outputs */

#define GPIO_CLKR_V	(0x1ff01ff)	/* MUX14&15 33MHz clocks */

#define GPIO_I_LEDRED		0
#define GPIO_I_LEDGREEN		1
#define GPIO_I_DISK2		2
#define GPIO_I_DISK1		3
#define GPIO_I_BUZZER		4
#define GPIO_I_POWERBUTTON	5	/* 0: pressed  */
#define GPIO_I_I2C_SCL		6
#define GPIO_I_I2C_SDA		7
#define GPIO_I_POWEROFF		8
#define GPIO_I_PCI_INTC		9
#define GPIO_I_PCI_INTB		10
#define GPIO_I_PCI_INTA		11
#define GPIO_I_RESETBUTTON	12	/* 1: pressed  */
#define GPIO_I_PCI_RESET	13
#define GPIO_I_PCI_CLOCK	14

#endif  /* __NSLU2_H__ */
