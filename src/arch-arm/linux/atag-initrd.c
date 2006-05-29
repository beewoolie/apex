/* atag-initrd.c
     $Id$

   written by Marc Singer
   5 May 2006

   Copyright (C) 2006 Marc Singer

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

   Support for Ramdisk ATAG.  This is only included if the appropriate
   configuration options are set.

*/

#include <atag.h>
#include <linux/string.h>
#include <config.h>
#include <apex.h>

struct tag* atag_initrd (struct tag* p)
{
       p->hdr.tag = ATAG_INITRD2;
       p->hdr.size = tag_size (tag_initrd);

       p->u.initrd.start = CONFIG_RAMDISK_LMA;
       p->u.initrd.size  = CONFIG_RAMDISK_SIZE;

       printf ("ATAG_INITRD2: start 0x%08x  size 0x%08x\n",
	       p->u.initrd.start, p->u.initrd.size);

       return tag_next (p);
}


static __atag_6 struct atag_d _atag_initrd = { atag_initrd };
