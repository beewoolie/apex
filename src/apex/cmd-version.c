/* cmd-version.c
     $Id$

   written by Marc Singer
   3 Nov 2004

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

#include <config.h>
#include <linux/types.h>
#include <apex.h>
#include <command.h>

extern char APEX_VMA_COPY_START;
extern char APEX_VMA_COPY_END;

#define _s(v) #v 
#define _t(v) _s(v)

command_func_t hook_cmd_version;

int cmd_version (int argc, const char** argv)
{
  printf (
"\r\n\nAPEX Boot Loader " APEXRELEASE
" -- Copyright (c) 2004 Marc Singer\r\n\n"
"APEX comes with ABSOLUTELY NO WARRANTY."
#if !defined (CONFIG_SMALL)
"  It is free software and you\r\n"
"are welcome to redistribute it under certain circumstances.\r\n"
"For details, refer to the file COPYING in the program source."
#endif
"\r\n\n"
"  apex => mem:0x%p#0x%lx\r\n"
	  ,
	  (void*) &APEX_VMA_COPY_START,
	  (unsigned long )(&APEX_VMA_COPY_END - &APEX_VMA_COPY_START));
#if defined (CONFIG_ENV_REGION)
  printf ("  env  => %s\r\n", _t(CONFIG_ENV_REGION));
#endif

  if (hook_cmd_version)
    hook_cmd_version (argc, argv);

  putchar ('\n');

  return 0;
}

static __command struct command_d c_version = {
  .command = "version",
  .description = "show version and copyright",
  .func = cmd_version,
};
