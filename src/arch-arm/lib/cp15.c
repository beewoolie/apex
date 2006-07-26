/* cp15.c

   written by Marc Singer
   25 Jul 2006

   Copyright (C) 2006 Marc Singer

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

#include <linux/string.h>
#include <config.h>

const char* cp15_ctrl (unsigned long ctrl)
{
  static const char bits[] = "vizfrsbldpwcam";
  const int cbits = sizeof (bits) - 1;
  static char sz[sizeof (bits)];
  int i;

  strcpy (sz, bits);

  for (i = 0; i < cbits; ++i)
    if (ctrl & (1<<(cbits - i - 1)))
      sz[i] += 'A' - 'a';
  return sz;
}

