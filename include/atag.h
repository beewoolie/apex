/* atag.h
     $Id$

   written by Marc Singer
   6 Nov 2004

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

#if !defined (__ATAG_H__)
#    define   __ATAG_H__

/* ----- Includes */

#include <linux/types.h>
#include <asm-arm/setup.h>
#include <attributes.h>

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

struct atag_d {
  struct tag* (*append_atag) (struct tag*);
};


#define __atag_0 __used __section(.atag.0)
#define __atag_1 __used __section(.atag.1)
#define __atag_2 __used __section(.atag.2)
#define __atag_3 __used __section(.atag.2)
#define __atag_4 __used __section(.atag.2)
#define __atag_5 __used __section(.atag.2)
#define __atag_6 __used __section(.atag.2)
#define __atag_7 __used __section(.atag.2)

#endif  /* __ATAG_H__ */
