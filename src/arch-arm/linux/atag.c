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
	struct atag_d* d;
	struct tag* p = (struct tag*) CONFIG_ATAG_PHYS;
	extern char APEX_ATAG_START;
	extern char APEX_ATAG_END;

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

struct tag* atag_commandline (struct tag* p)
{
#if 0
	char *p;
	int i;
	const char* szCommandLine = env_fetch ("cmdline");

	/* initialise commandline */
	params->u.cmdline.cmdline[0] = '\0';

	/* copy default commandline from parameter block */
	if (szCommandLine) 
		strlcpy(params->u.cmdline.cmdline, szCommandLine, 
			COMMAND_LINE_SIZE);

	/* copy commandline */
	if(argc >= 2) {
		p = params->u.cmdline.cmdline;

		for(i = 1; i < argc; i++) {
			strlcpy(p, argv[i], COMMAND_LINE_SIZE);
			p += strlen(p);
			*p++ = ' ';
		}
	
		p--;
		*p = '\0';

		/* technically spoken we should limit the length of
		 * the kernel command line to COMMAND_LINE_SIZE
		 * characters, but the kernel won't copy longer
		 * strings anyway, so we don't care over here.
		 */
	}

	if(strlen(params->u.cmdline.cmdline) > 0) {
		params->hdr.tag = ATAG_CMDLINE;
		params->hdr.size = (sizeof(struct tag_header) + 
				    strlen(params->u.cmdline.cmdline) + 1 + 4) >> 2;
		
		params = tag_next(params);
	}
#endif
	return p;
}


static __atag_0 struct atag_d _atag_header = { atag_header };
static __atag_1 struct atag_d _atag_commandline = { atag_commandline };
static __atag_3 struct atag_d _atag_end = { atag_end };
