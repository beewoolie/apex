/* drv-cf.h
     $Id$

   written by Marc Singer
   7 Feb 2005

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

#if !defined (__MACH_DRV_CF_H__)
#    define   __MACH_DRV_CF_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#if defined (CONFIG_MACH_LPD79520)

# if !defined (CF_PHYS)
#  define CF_PHYS		(0x50200000)
#  define CF_IOBARRIER_PHYS	(0x20000000)
# endif

# define CF_WIDTH		(16)
# define CF_ADDR_MULT		(1)
# define CF_REG			(1<<12)
# define CF_ALT			(1<<11)
# define CF_ATTRIB		(1<<10)

#endif

#if defined (CONFIG_MACH_LPD79524)

# if !defined (CF_PHYS)
#  define CF_PHYS		(0x48200000)
#  define CF_IOBARRIER_PHYS	(0x20000000)
# endif

# define CF_WIDTH		(16)
# define CF_ADDR_MULT		(2)
# define CF_REG			(1<<13)
# define CF_ALT			(1<<12)
# define CF_ATTRIB		(1<<10)

#endif


#endif  /* __MACH_DRV_CF_H__ */
