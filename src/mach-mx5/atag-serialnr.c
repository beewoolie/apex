/* atag-serialnr.c

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

   Support for board serial number ATAG.

*/

#include <atag.h>
#include <linux/string.h>
#include <config.h>
#include <apex.h>
#include <lookup.h>

struct tag* atag_serialnr (struct tag* p)
{
	p->hdr.tag = ATAG_SERIAL;
	p->hdr.size = tag_size (tag_serialnr);

        p->u.serialnr.low = 0;
        p->u.serialnr.high = 0;

# if !defined (CONFIG_SMALL)
	printf ("ATAG_SERIAL: 0x%08x-%08x\n",
		p->u.serialnr.high, p->u.serialnr.low);
# endif

	return tag_next (p);
}

static __atag_3 struct atag_d _atag_serialnr = { atag_serialnr };
