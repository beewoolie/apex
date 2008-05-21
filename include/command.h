/* command.h

   written by Marc Singer
   3 Nov 2004

   Copyright (C) 2004 Marc Singer

   This program is licensed under the terms of the GNU General Public
   License as published by the Free Software Foundation.  Please refer
   to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__COMMAND_H__)
#    define   __COMMAND_H__

/* ----- Includes */

#include "config.h"
#include <attributes.h>

/* ----- Types */

typedef int (*command_func_t) (int argc, const char** argv);

struct command_d {
  const char* command;
  command_func_t func;
#if !defined (CONFIG_NOHELP)
  const char* description;
#endif
#if defined (CONFIG_ALLHELP)
  const char* help;
#endif
};

#if defined (CONFIG_ALLHELP)
# define COMMAND_HELP(s) .help = s,
#else
# define COMMAND_HELP(s)
#endif

#if defined (CONFIG_NOHELP)
# define COMMAND_DESCRIPTION(s)
#else
# define COMMAND_DESCRIPTION(s) .description = s,
#endif

#define __command __used __section(.command)

/* ----- Globals */

/* ----- Prototypes */



#endif  /* __COMMAND_H__ */
