/* reg.h
     $Id$

   written by Marc Singer
   11 Nov 2004

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

   Macros to make register file access somewhat efficient in C.  The
   4K constant comes from the fact that ARM instructions are limited
   to a 4K immediate offset. 

*/

#if !defined (__REG_H__)
#    define   __REG_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

typedef struct { volatile unsigned long  offset[4096>>2]; } __reg32;
typedef struct { volatile unsigned short offset[4096>>1]; } __reg16;
typedef struct { volatile unsigned char  offset[4096>>0]; } __reg08;

#define __REG_M(t,x,s) ((t*)((x)&~(4096-1)))->offset[((x)&(4096-1))>>s]

#define __REG(x)	__REG_M (__reg32, (x), 2)
#define __REG16(x)	__REG_M (__reg16, (x), 1)
#define __REG8(x)	__REG_M (__reg08, (x), 0)

#endif  /* __REG_H__ */
