/* lpd79524.h
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

#if !defined (__LPD79524_H__)
#    define   __LPD79524_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */


#define CPLD_REVISION		(0x4ca00000)

#define CPLD_FLASH		(0x4c800000)
#define CPLD_FLASH_RDYnBSY	(1<<2)
#define CPLD_FLASH_FL_VPEN	(1<<0)
#define CPLD_FLASH_STS1		(1<<1)
#define CPLD_FLASH_NANDSPD	(1<<6)


#endif  /* __LPD79524_H__ */
