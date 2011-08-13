/* atag-revision.c

   written by Marc Singer
   13 Aug 2011

   Copyright (C) 2011 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   Support for board revision ATAG.

*/

#include <atag.h>
#include <linux/string.h>
#include <config.h>
#include <apex.h>
#include <lookup.h>

struct tag* atag_revision (struct tag* p)
{
	p->hdr.tag = ATAG_REVISION;
	p->hdr.size = tag_size (tag_revision);

        p->u.revision.rev = 0;

# if !defined (CONFIG_SMALL)
	printf ("ATAG_REVISION: %d (0x%08x)\n",
		p->u.revision.rev, p->u.revision.rev);
# endif

	return tag_next (p);
}

static __atag_3 struct atag_d _atag_revision = { atag_revision };
