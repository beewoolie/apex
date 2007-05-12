/* attributes.h

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


#define __weak			__attribute__((weak))
#define __naked			__attribute__((naked))
#define __used			__attribute_used__
#define __section(s)		__attribute__((section(#s)))
#define __xbss(s)		__attribute__((section("." #s ".xbss")))
#define __rodata	  const __section(.rodata)
#define __irq_handler		__attribute__((interrupt ("IRQ")))
#define __fiq_handler		__attribute__((interrupt ("FIQ")))
#define __swi_handler		__attribute__((interrupt ("SWI")))
#define __abort_handler		__attribute__((interrupt ("ABORT")))
#define __undef_handler		__attribute__((interrupt ("UNDEF")))

#define __aligned		__attribute__((aligned (32)))

#define __bootstrap_func	__section(.bootstrap.func) /* Early funcs */

#define __bootstrap_0		__section(.bootstrap.0)	/* Bootstrap entry */
#define __bootstrap_1		__section(.bootstrap.1)
#define __bootstrap_2		__section(.bootstrap.2)	/* Target preinit */
#define __bootstrap_3		__section(.bootstrap.3)
#define __bootstrap_4		__section(.bootstrap.4)
							/* Preinit here */
#define __bootstrap_5		__section(.bootstrap.5)
#define __bootstrap_6		__section(.bootstrap.6)
#define __bootstrap_7		__section(.bootstrap.7)
#define __bootstrap_8		__section(.bootstrap.8)
							/* Target init here */
#define __bootstrap_9		__section(.bootstrap.9)	/* Bootstrap exit */

#endif  /* __ATTRIBUTES_H__ */
