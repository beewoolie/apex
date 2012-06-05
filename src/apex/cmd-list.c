/* cmd-list.c

   written by Marc Singer
   29 August 2011

   Copyright (C) 2005 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   List command for showing specific information about a region.  This
   is mostly used for showing the contents of a directory.

*/

#include <config.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/ctype.h>
#include <apex.h>
#include <command.h>
#include <driver.h>
#include <error.h>
#include <spinner.h>


#if !defined (CONFIG_SMALL)
//# define USE_COPY_VERIFY	/* Define to include verify feature */
#endif

int cmd_list (int argc, const char** argv)
{
  struct descriptor_d din;

  int result = 0;

  if (argc != 2)
    return ERROR_PARAM;

  if ((result = parse_descriptor (argv[1], &din))) {
    printf ("Unable to open '%s'\n", argv[1]);
    goto fail;
  }

  if (din.driver->info)
    din.driver->info (&din);
  else
    result = ERROR_UNSUPPORTED;

 fail:
  return result;
}

static __command struct command_d c_list = {
  .command = "list",
  .func = cmd_list,
  COMMAND_DESCRIPTION ("list information about a region")
  COMMAND_HELP(
"list"
" SRC\n"
"  List information about region.  This command either describes\n"
"  the device controlling the region descriptor, or the descriptor itself.\n"
"  If the region is a filesystem directory, the command lists the files\n"
"  in the directory.\n"
"  e.g.  list ext2://1/etc\n"
  )
};
