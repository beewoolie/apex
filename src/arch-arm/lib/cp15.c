/* cp15.c

   written by Marc Singer
   25 Jul 2006

   Copyright (C) 2006 Marc Singer

   This program is licensed under the terms of the GNU General Public
   License as published by the Free Software Foundation.  Please refer
   to the file debian/copyright for further details.

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
