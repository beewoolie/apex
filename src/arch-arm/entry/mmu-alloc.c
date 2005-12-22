/* mmu-alloc.c
     $Id$

   written by Marc Singer
   22 Dec 2005

   Copyright (C) 2005 Marc Singer

   -----------
   DESCRIPTION
   -----------

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

*/

#include <config.h>
#include <debug_ll.h>
#include <mach/coprocessor.h>
#include <attributes.h>
#include <linux/string.h>
#include <service.h>
#include <apex.h>
#include <asm/mmu.h>

void* pvAlloc;			/* Address of next allocatable address */

void* alloc_uncached (size_t cb, size_t alignment)
{
  void* pv;

  if (alignment == 0)
    alignment = 1;

  if (pvAlloc == 0) {
    extern char APEX_VMA_END;
    pvAlloc = (void*) (((unsigned long) &APEX_VMA_END + (1024*1024 - 1))
	       & ~(1024*1024 - 1));
  }

  pv = (void*) (((unsigned long) pvAlloc + (alignment - 1))
		& ~(alignment - 1));
  pvAlloc = pv + cb;

#if defined (CONFIG_MMU)
  mmu_protsegment (pv, 0, 0);
#endif

  return pv;  
}
