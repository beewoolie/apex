/* driver.c
     $Id$

   written by Marc Singer
   4 Nov 2004

   Copyright (C) 2004 Marc Singer

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

   o Lower-case pathnames.

     Until there is a strcasecmp, we need to locase pathnames.

*/

#include <linux/string.h>
#include <linux/ctype.h>
#include <driver.h>
#include <linux/kernel.h>
#include <error.h>
#include <apex.h>
#include <config.h>

void close_helper (struct descriptor_d* d)
{
  d->length = 0;
}

void close_descriptor (struct descriptor_d* d)
{
  if (d->driver && d->driver->close)
    d->driver->close (d);
}

int is_descriptor_open (struct descriptor_d* d)
{
  return d->length != 0;
}

int open_descriptor (struct descriptor_d* d)
{
  if (!d->driver || !d->driver->open)
    return ERROR_UNSUPPORTED;
  return d->driver->open (d);
}

int parse_descriptor (const char* sz, struct descriptor_d* d)
{
  size_t cb;
  size_t ib;

  memzero (d, sizeof (*d));

  ib = cb = strcspn (sz, ":");
  if (sz[ib] == ':') {
    cb = ++ib;
    if (cb > sizeof (d->driver_name))
      cb = sizeof (d->driver_name);
    strlcpy (d->driver_name, sz, cb);
  }
  else {
    strcpy (d->driver_name, "memory");
    ib = 0;
  }
  cb = strlen (d->driver_name);

  {
    extern char APEX_DRIVER_START;
    extern char APEX_DRIVER_END;
    struct driver_d* driver;
    struct driver_d* driver_match = NULL;
    for (driver = (struct driver_d*) &APEX_DRIVER_START;
	 driver < (struct driver_d*) &APEX_DRIVER_END;
	 ++driver) {
      if (driver->name && strnicmp (d->driver_name, driver->name, cb) == 0) {
	if (driver_match)
	  return ERROR_AMBIGUOUS;
	driver_match = driver;
	if (d->driver_name[cb] == 0)
	  break;
      }
    }
    d->driver = driver_match;
  }

  if (!d->driver)
    return ERROR_NODRIVER;

#if defined (CONFIG_DRIVER_FAT)
  if (d->driver->flags & DRIVER_DESCRIP_FS) {
    int state = 0;
    strlcpy (d->rgb, sz + ib, sizeof (d->rgb));
//    printf (" descript (%s)\n", d->rgb);
    d->c = 0;
    d->iRoot = 0;
    for (ib = 0; d->rgb[ib]; ++ib) {
      switch (state) {
      case 0:
	if (d->rgb[ib] == '/')
	  ++state;
	else if (d->rgb[ib] == '@' || d->rgb[ib] == '+') {
	  d->rgb[ib] = 0;
	  goto region_parse;
	}
	else {
	  d->rgb[ib] = tolower (d->rgb[ib]);
	  d->pb[d->c++] = &d->rgb[ib];
	  state = 9;
	}
	break;
      case 1:
	if (d->rgb[ib] == '/') {
	  ++d->iRoot;
	  ++state;
	}
	else if (d->rgb[ib] == '@' || d->rgb[ib] == '+') {
	  d->rgb[ib] = 0;
	  goto region_parse;
	}
	else {
	  d->rgb[ib] = tolower (d->rgb[ib]);
	  d->pb[d->c++] = &d->rgb[ib];
	  state = 9;
	}
	break;
      case 2:
	if (d->rgb[ib] == '/')
	  return ERROR_FAILURE;
	if (d->rgb[ib] == '@' || d->rgb[ib] == '+') {
	  d->rgb[ib] = 0;
	  goto region_parse;
	}
	d->rgb[ib] = tolower (d->rgb[ib]);
	d->pb[d->c++] = &d->rgb[ib];
	state = 9;
	break;
      case 9:
	if (d->rgb[ib] == '@' || d->rgb[ib] == '+') {
	  d->rgb[ib] = 0;
	  goto region_parse;
	}
	if (d->rgb[ib] != '/') {
	  d->rgb[ib] = tolower (d->rgb[ib]);
	  continue;
	}
	d->rgb[ib] = 0;
	state = 2;
      }
    }
  }
  else 
#endif
  {			/* Region descriptor parse */
  region_parse:
    while (sz[ib]) {
      char* pchEnd;
      unsigned long* pl = 0;

      switch (sz[ib]) {
      case '@':
	++ib;
      case '0'...'9':
	pl = &d->start;
	break;
      case '#':			/* Deprecated */
      case '+':
	pl = &d->length;
	/* Fallthrough to get ++ib */
      default:
	++ib;
	/* Skip over unknown bytes so that we always terminate */
	break;
      }
      if (pl) {
	*pl = simple_strtoul (sz + ib, &pchEnd, 0);
	ib = pchEnd - sz;
	if (sz[ib] == 's' || sz[ib] == 'S') {
	  *pl *= 512;
	  ++ib;
	}
	if (sz[ib] == 'k' || sz[ib] == 'K') {
	  *pl *= 1024;
	  ++ib;
	}
	if (sz[ib] == 'm' || sz[ib] == 'M') {
	  *pl *= 1024*1024;
	  ++ib;
	}
      }
    }
  }

#if defined (CONFIG_DRIVER_FAT)
		/* Make sure there is an empty field when there are /'s */
  if (d->c == 0 && d->rgb[0])
    d->pb[d->c++] = &d->rgb[ib];
#endif

  return 0;
}

size_t seek_helper (struct descriptor_d* d, ssize_t ib, int whence)
{
  switch (whence) {
  case SEEK_SET:
    break;
  case SEEK_CUR:
    ib += d->index;
    break;
  case SEEK_END:
    ib = d->length - ib;
    break;
  }

  if (ib < 0)
    ib = 0;
  if (ib > d->length)
    ib = d->length;

  d->index = ib;

  return (size_t) ib;
}
