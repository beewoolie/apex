/* exitstatus.cxx
     $Id: exitstatus.cc,v 1.4 2003/08/11 07:45:04 elf Exp $

   written by Marc Singer (aka Oscar Levi)
   23 May 1999

   This file is part of the project BSIGN.  See the file README for
   more information.

   Copyright (c) 1998, 2003 Marc Singer

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

*/

#include "standard.h"
#include "exitstatus.h"

#include <stdarg.h>

char g_szExitStatus[256];
eExitStatus g_exitStatus;	// Return code for the program

void set_exitstatus (eExitStatus exitStatus, const char* sz, ...)
{
  g_exitStatus = exitStatus;

  if (sz) {
    va_list ap;
    va_start (ap, sz);
    vsnprintf (g_szExitStatus, sizeof (g_szExitStatus), sz, ap);
    va_end (ap);
  }
  else
    g_szExitStatus[0] = 0;
}

void set_exitstatus_reported (void)
{
  g_szExitStatus[0] = 0;
}
