/* command.h
     $Id$

   written by Marc Singer
   3 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__COMMAND_H__)
#    define   __COMMAND_H__

/* ----- Includes */

/* ----- Types */

struct command_d {
  const char* command;
  const char* description;
  const char* help;
  int cbPrefix;			/* Length of prefix (?) */
  int (*func)(int argc, const char** argv);
};

#define __command __attribute__((used,section(".command")))


/* ----- Globals */

/* ----- Prototypes */



#endif  /* __COMMAND_H__ */
