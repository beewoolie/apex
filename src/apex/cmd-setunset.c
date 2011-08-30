/* cmd-set.c

   written by Marc Singer
   6 Jul 2005

   Copyright (C) 2005 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

*/

#include <config.h>
#include <linux/string.h>
#include <apex.h>
#include <command.h>
#include <error.h>
#include <variables.h>

static int cmd_set (int argc, const char** argv)
{
  void* pv;
  const char* key;
  const char* value;

  switch (argc) {
  case 1:
    /* Show variables */
    for (pv = NULL; (pv = variables_enumerate (pv, &key, &value)); )
      printf ("%s\t%s\n", key, value);
    break;

  case 2:
    /* Show single variable */
    value = variable_lookup (argv[1]);
    if (value)
      printf ("%s\t%s\n", argv[1], value);
    break;
  case 3:
    /* Set variable */
    variable_unset (argv[1]);	/* Just in case */
    variable_set (argv[1], argv[2]);
    break;
  default:
    return ERROR_PARAM;
  }

  return 0;
}

static int cmd_unset (int argc, const char** argv)
{
  if (argc != 2)
    return ERROR_PARAM;

  variable_unset (argv[1]);
  return 0;
}

static __command struct command_d c_set = {
  .command = "set",
  .func = cmd_set,
  COMMAND_DESCRIPTION ("show or set variables")
  COMMAND_HELP(
"set [KEY [VALUE]]\n"
"  Show all variables, the variable KEY or set a variable KEY to value VALUE.\n"
"  Similar to the environment, variables are run-time defined only.\n"
  )
};

static __command struct command_d c_unset = {
  .command = "unset",
  .func = cmd_unset,
  COMMAND_DESCRIPTION ("remove a variable")
  COMMAND_HELP(
"unset KEY\n"
"  Remove variable KEY.\n"
  )
};
