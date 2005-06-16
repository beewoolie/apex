/* attributes.h
     $Id$

   written by Marc Singer
   10 Feb 2005

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

#if !defined (__ATTRIBUTES_H__)
#    define   __ATTRIBUTES_H__

/* ----- Includes */

#include <linux/compiler.h>

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */


#define __weak __attribute__((weak))
#define __naked __attribute__((naked))
# define __used __attribute_used__
#define __section(s) __attribute__((section(#s)))
#define __irq_handler __attribute__((interrupt ("IRQ")))
#define __fiq_handler __attribute__((interrupt ("FIQ")))
#define __swi_handler __attribute__((interrupt ("SWI")))
#define __abort_handler __attribute__((interrupt ("ABORT")))
#define __undef_handler __attribute__((interrupt ("UNDEF")))

#endif  /* __ATTRIBUTES_H__ */
