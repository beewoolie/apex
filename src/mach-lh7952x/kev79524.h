/* kev79524.h
     $Id$

   written by Marc Singer
   12 Nov 2004

   Copyright (C) 2004 Marc Singer

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

#if !defined (__KEV79524_H__)
#    define   __KEV79524_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#if 1
#define CPLD_EXGPIO_GPIO1	(1<<0)
#define CPLD_EXGPIO_LED2	(1<<1)
#define CPLD_EXGPIO_LED1	(1<<2)
#else
#define CPLD_EXGPIO_LED1	(1<<1)
#define CPLD_EXGPIO_LED2	(1<<2)
#endif

#if 1
# define CF_PHYS	0x48200000
#else
# define CF_PHYS	0x48100000
#endif

#endif  /* __KEV79524_H__ */
