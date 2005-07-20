/* lh7a400.h
     $Id$

   written by Marc Singer
   8 Jul 2005

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

#if !defined (__LH7A400_H__)
#    define   __LH7A400_H__

/* ----- Includes */

#include <asm/reg.h>

/* ----- Types */

#define HRTFTC_PHYS	(0x80001000)	/* High-res TFT Controller */

#define HRTFTC_SETUP	__REG (HRTFTC_PHYS + 0x00)
#define HRTFTC_CON	__REG (HRTFTC_PHYS + 0x04)
#define HRTFTC_TIMING1	__REG (HRTFTC_PHYS + 0x08)
#define HRTFTC_TIMING2	__REG (HRTFTC_PHYS + 0x0c)

#endif  /* __LH7A400_H__ */
