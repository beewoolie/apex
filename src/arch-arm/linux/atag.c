/* atag.c
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

#include <atag.h>
#include <linux/string.h>
#include <apex.h>
#include <config.h>

void build_atags (void)
{
	extern char APEX_ATAG_START;
	extern char APEX_ATAG_END;

	struct atag_d* d;
	struct tag* p = (struct tag*) CONFIG_ATAG_PHYS;

	for (d = (struct atag_d*) &APEX_ATAG_START;
	     d < (struct atag_d*) &APEX_ATAG_END;
	     ++d)
		p = d->append_atag (p);
}

struct tag* atag_header (struct tag* p)
{
	p->hdr.tag = ATAG_CORE;
	p->hdr.size = tag_size (tag_core);
	memzero (&p->u.core, sizeof (p->u.core));

# if !defined (CONFIG_SMALL)
	printf ("ATAG_HEADER\n");
# endif

	return tag_next (p);
}

struct tag* atag_end (struct tag* p)
{
	p->hdr.tag = ATAG_NONE;
	p->hdr.size = 0;

# if !defined (CONFIG_SMALL)
	printf ("ATAG_END\n");
# endif

	return tag_next (p);
}


static __atag_0 struct atag_d _atag_header = { atag_header };
static __atag_7 struct atag_d _atag_end    = { atag_end };
