/* timer.c
     $Id$

   written by Marc Singer
   14 Jan 2005

   Copyright (C) 2005 Marc Singer

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
#include <service.h>
#include "hardware.h"

unsigned long timer_read (void)
{
  return OST_TS;
}


/* timer_delta

   returns the difference in time in milliseconds.
  
   The base counter rate is 66.66MHz or a 15ns cycle time.  1us takes
   67 counts, 1ms takes 66660 counts.  It wraps at about 64 seconds.

   The math works out such that wrapping around the end will return
   the expected result.

*/

unsigned long timer_delta (unsigned long start, unsigned long end)
{
  return (end - start)/66660;
}
