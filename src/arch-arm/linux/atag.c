/* atag.c
     $Id$

   written by Marc Singer
   6 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#include <atag.h>
#include <linux/string.h>
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
	memset (&p->u.core, 0, sizeof (p->u.core));

	return tag_next (p);
} 

struct tag* atag_end (struct tag* p)
{
	p->hdr.tag = ATAG_NONE;
	p->hdr.size = 0;

	return tag_next (p);
} 


static __atag_0 struct atag_d _atag_header = { atag_header };
static __atag_3 struct atag_d _atag_end = { atag_end };
