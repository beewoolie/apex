/* tty.cxx
     $Id: tty.cc,v 1.6 2003/08/11 07:45:04 elf Exp $

   written by Marc Singer (aka Oscar Levi)
   13 December 1998

   This file is part of the project BSIGN.  See the file README for
   more information.

   Copyright (c) 1998,2003 Marc Singer 

   This program is free software; you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License

   -----------
   DESCRIPTION
   -----------

   TTY IO control functions.

*/

#include "standard.h"
#include <termios.h>
#include "tty.h"
#include <errno.h>
#include "args.h"


struct termios g_termiosSave;

void disable_echo (int fh)
{
  struct termios termios;

  if (tcgetattr (fh, &g_termiosSave))
    fprintf (stderr, "%s: tcgetattr failed on %d (%s)\n", 
	     g_arguments.szApplication, fh, strerror (errno));
  //        restore_termios = 1;
  termios = g_termiosSave;
  termios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
  if (tcsetattr (fh, TCSAFLUSH, &termios))
    fprintf (stderr, "%s: tcsetattr failed on %d (%s)\n", 
	     g_arguments.szApplication, fh, strerror (errno));
}

void restore_echo (int fh)
{
  if (tcsetattr (fh, TCSAFLUSH, &g_termiosSave))
    fprintf (stderr, "%s: tcsetattr failed on %d (%s)\n", 
	     g_arguments.szApplication, fh, strerror (errno));
}

int open_user_tty (void)
{
  return open ("/dev/tty", O_RDWR);
}
