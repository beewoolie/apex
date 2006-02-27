/* memory.h

   written by Marc Singer
   22 Feb 2005

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

#if !defined (__MEMORY_H__)
#    define   __MEMORY_H__

/* ----- Includes */

#include <config.h>

/* ----- Constants */

#if defined (CONFIG_SDRAM_BANK0)
# define RAM_BANK0_START	0x00000000
# define RAM_BANK0_LENGTH	CONFIG_SDRAM_BANK_LENGTH
#endif

#if defined (CONFIG_SDRAM_BANK1)
# define RAM_BANK1_START	(RAM_BANK0_START + RAM_BANK0_LENGTH)
# define RAM_BANK1_LENGTH	CONFIG_SDRAM_BANK_LENGTH
#endif

#define MVA_CACHE_CLEAN		(0x0 + 2*CONFIG_SDRAM_BANK_LENGTH)

	/* MMU macro for assigning segment protection bits. */
#define PROTECTION_FOR(p) \
({ int v = 0;   /* uncacheable, unbuffered */ \
   if (   (p) >= 0x0 \
       && (p) <= 2*CONFIG_SDRAM_BANK_LENGTH)\
     v = 3<<2;/* cacheable, buffered */ \
   v; })

#endif  /* __MEMORY_H__ */
