/* init.c
     $Id$

   written by Marc Singer
   31 Oct 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#include <linux/types.h>
#include <linux/string.h>
#include <apex.h>
#include <command.h>

extern int cmd_version (int, const char**);

void init (void)
{
  init_drivers ();
  cmd_version (0, NULL);	/* Signon */
  exec_monitor ();
}
