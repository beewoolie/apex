/* cmd-alias.c
     $Id$

   written by Marc Singer
   6 Jul 2005

   Copyright (C) 2005 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#include <config.h>
#include <linux/string.h>
#include <apex.h>
#include <command.h>
#include <error.h>
#include <alias.h>

static int cmd_alias (int argc, const char** argv)
{
  void* pv;
  const char* key;
  const char* value;

  switch (argc) {
  case 1:
    /* Show aliases */
    for (pv = NULL; (pv = alias_enumerate (pv, &key, &value)); )
      printf ("%s\t%s\n", key, value);
    break;

  case 2:
    /* Show single alias */
    value = alias_lookup (argv[1]);
    if (value)
      printf ("%s\t%s\n", argv[1], value);
    break;
  case 3:
    /* Set alias */
    alias_unset (argv[1]);	/* Just in case */
    alias_set (argv[1], argv[2]);
    break;
  default:
    return ERROR_PARAM; 
  }

  return 0;
}

static int cmd_unalias (int argc, const char** argv)
{
  if (argc != 2)
    return ERROR_PARAM;

  alias_unset (argv[1]);
  return 0;
}

static __command struct command_d c_alias = {
  .command = "alias",
  .description = "Set/show aliases",
  .func = cmd_alias,
  COMMAND_HELP(
"alias NAME [VALUE]\n"
"  Show or set an alias\n"
"  Similar to th eenvironment, aliases are run-time defined only.\n" 
  )
};

static __command struct command_d c_unalias = {
  .command = "unalias",
  .description = "Unset aliases",
  .func = cmd_unalias,
  COMMAND_HELP(
"unalias NAME\n"
"  Remove an alias\n"
  )
};
