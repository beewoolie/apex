/* drv-clcdc.h
     $Id$

   written by Marc Singer
   24 Dec 2005

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

#if !defined (__MACH_DRV_CLCDC_H__)
#    define   __MACH_DRV_CLCDC_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

  /* Note that PE4 must be driven high on the LPD7A404 to prevent the
     CPLD JTAG chain from crashing the board.  See the target_init ()
     code. */

#define DRV_CLCDC_SETUP ({ GPIO_PINMUX |= (1<<1) | (1<<0); }) /* LCDVD[15:4] */

#if defined (CONFIG_MACH_LPD7A400)
# define DRV_CLCDC_BACKLIGHT_ENABLE\
	({ CPLD_CONTROL |= CPLD_CONTROL_LCD_VEEEN; })
#endif

#if defined (CONFIG_MACH_LPD7A404)
# define DRV_CLCDC_BACKLIGHT_ENABLE\
	({ GPIO_PCDD |= (1<<3); GPIO_PCD  |= (1<<3); })
#endif

#if defined (CONFIG_MACH_LPD7A400)
# define DRV_CLCDC_BACKLIGHT_DISABLE\
	({ CPLD_CONTROL &= ~CPLD_CONTROL_LCD_VEEEN;
#endif
#if defined (CONFIG_MACH_LPD7A404)
# define DRV_CLCDC_BACKLIGHT_DISABLE\
	({ GPIO_PCD  &= ~(1<<3); })
#endif


#if defined (CONFIG_ARCH_LH7A400)
# define DRV_CLCDC_DISABLE\
	({ HRTFTC_SETUP &= ~(1<<13); }) /* Disable HRTFT controller */
#endif

#if defined (CONFIG_ARCH_LH7A404)
# define DRV_CLCDC_DISABLE\
	({ ALI_SETUP &= ~(1<<13); }) /* Disable ALI */
#endif


#endif  /* __MACH_DRV_CLCDC_H__ */
