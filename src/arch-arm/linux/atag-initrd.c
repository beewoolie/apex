/* atag-initrd.c

   written by Marc Singer
   5 May 2006

   Copyright (C) 2006 Marc Singer

   This program is licensed under the terms of the GNU General Public
   License as published by the Free Software Foundation.  Please refer
   to the file debian/copyright for further details.

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

   /* *** Hack to prevent serious problems until all of the targets
      get the ramdisk size defined. */
#if defined (CONFIG_RAMDISK_LMA) && defined (CONFIG_RAMDISK_SIZE)

struct tag* atag_initrd (struct tag* p)
{
	p->hdr.tag = ATAG_INITRD2;
	p->hdr.size = tag_size (tag_initrd);

	p->u.initrd.start = CONFIG_RAMDISK_LMA;
	p->u.initrd.size  = CONFIG_RAMDISK_SIZE;

# if !defined (CONFIG_SMALL)
	printf ("ATAG_INITRD2: start 0x%08x  size 0x%08x\n",
		p->u.initrd.start, p->u.initrd.size);
# endif

	return tag_next (p);
}

static __atag_6 struct atag_d _atag_initrd = { atag_initrd };

#endif
