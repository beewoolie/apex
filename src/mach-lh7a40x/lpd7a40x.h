/* lpd7a40x.h
     $Id$

   written by Marc Singer
   4 Dec 2004

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

#if !defined (__LPD7A40X_H__)
#    define   __LPD7A40X_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#define CPLD_FLASH		(0x71000000)
#define CPLD_FLASH_FL_VPEN	(1<<0)
#define CPLD_FLASH_FST1		(1<<1)
#define CPLD_FLASH_FST2		(1<<2)


#endif  /* __LPD7A40X_H__ */
