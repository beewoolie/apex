/* nslu2.h
     $Id$

   written by Marc Singer
   11 Feb 2005

   Copyright (C) 2005 Marc Singer

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

#define GPIO_ER_OUTPUTS	 (1<<0)		/* Status LED */\
			|(1<<1)		/* Ready LED */\
			|(1<<2)		/* Disk2 LED */\
			|(1<<3)		/* Disk1 LED */\
			|(1<<4)		/* Buzzer */
			
#define GPIO_ER_V	(0x1f3f)	/* 0,1,6,7 are outputs */

#define GPIO_CLKR_V	(0x1ff01ff)	/* MUX14&15 33MHz clocks */


#endif  /* __NSLU2_H__ */
