/* cmd-image.c

   written by Marc Singer
   3 Aug 2008

   Copyright (C) 2008 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   o Dispatch to image handlers.  This code is a simple dispatcher for
     the various image handles.  There are only two, so it is a bit of
     over-engineering, but the fact that the two handlers can be
     included independently makes some sort of dispatch desirable.

*/

#define TALK 2

#include <config.h>
//#include <linux/types.h>
//#include <linux/string.h>
#include <apex.h>
#include <command.h>
#include <error.h>
//#include <linux/kernel.h>
#include <driver.h>
#include "cmd-image.h"
#include <talk.h>

#include <debug_ll.h>

#if defined (CONFIG_CMD_IMAGE_SHOW)

const char* describe_size (uint32_t cb)
{
  static __xbss(image) char sz[60];

  int s = cb/1024;
  int rem = cb%1024;
  char unit = 'K';

  if (s > 1024) {
    rem = s%1024;
    s /= 1024;
    unit = 'M';
  }
  snprintf (sz, sizeof (sz), "%d bytes (%d.%02d %ciB)",
            cb, (int) s, (rem*100)/1024, unit);
  return sz;
}

#endif

int cmd_image (int argc, const char** argv)
{
  struct descriptor_d d;
  int result = 0;
  int op = 's';
  bool fRegionCanExpand = false;
  char rgb[16];                 /* Storage for bytes to determine image type */
  pfn_handle_image pfn = NULL;

  if (argc < 2)
    return ERROR_PARAM;

  if (argc > 2) {
#if defined (CONFIG_CMD_IMAGE_SHOW)
    if      (PARTIAL_MATCH (argv[1], "s", "how") == 0)
      op = 's';
    else
#endif
    if      (PARTIAL_MATCH (argv[1], "c", "heck") == 0)
      op = 'c';
    else if (PARTIAL_MATCH (argv[1], "l", "oad") == 0)
      op = 'l';
    else
      ERROR_RETURN (ERROR_PARAM, "unrecognized OP");
    --argc;
    ++argv;
  }

  if ((result = parse_descriptor (argv[1], &d))) {
    printf ("Unable to open '%s'\n", argv[1]);
    goto fail;
  }

  fRegionCanExpand = d.length == 0;

  if (fRegionCanExpand && d.length - d.index < sizeof (rgb))
    d.length = d.index + sizeof (rgb);

	/* Read some bytes so we can determine the image type */
  result = d.driver->read (&d, rgb, sizeof (rgb));
  if (result != sizeof (rgb)) {
    result = ERROR_RESULT (ERROR_IOFAILURE, "image read error");
    goto fail;
  }

	/* Detect image type */
#if defined (CONFIG_CMD_IMAGE_APEX)
  if (pfn == NULL)
    pfn = is_apex_image (rgb, sizeof (rgb));
#endif
#if defined (CONFIG_CMD_IMAGE_UBOOT)
  if (pfn == NULL)
    pfn = is_uboot_image (rgb, sizeof (rgb));
#endif

  if (pfn)
    result = pfn (op, &d, fRegionCanExpand);
  else
    result = ERROR_RESULT (ERROR_UNRECOGNIZED, "unrecognized image");

 fail:
  close_descriptor (&d);
  return result;
}

static __command struct command_d c_image = {
  .command = "image",
  .func = cmd_image,
  COMMAND_DESCRIPTION ("image handling")
  COMMAND_HELP(
"image [OP] REGION\n"
"  Operation on an image.\n"
"  Without an OP, the image command verifies the integrity of the image.\n"
"  Choices for OP are\n"
"    load     - load image payload or payloads into memory\n"
#if defined (CONFIG_CMD_IMAGE_SHOW)
"    show     - show the image metadata\n"
#endif
"    check    - check the integrity of the image including payload CRCs\n"
"  In some cases, the region length may be omitted and the command\n"
"  will infer the length from the image header.\n"
"  e.g.  image check 0xc1000000\n"
#if defined (CONFIG_CMD_IMAGE_SHOW)
"        image show  0xc0000000\n"
#endif
"        image load  0xc0000000\n"
  )
};
