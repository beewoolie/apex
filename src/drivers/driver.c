/* driver.c
     $Id$

   written by Marc Singer
   4 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#include <linux/string.h>
#include <driver.h>
#include <linux/kernel.h>
#include <error.h>
#include <apex.h>

int parse_descriptor (const char* sz, struct open_d* descriptor)
{
  size_t cb;
  size_t ib;

  memset (descriptor, 0, sizeof (*descriptor));
  descriptor->fh = -1;

  ib = cb = strcspn (sz, ":");
  if (sz[ib] == ':') {
    cb = ++ib;
    if (cb > sizeof (descriptor->driver_name))
      cb = sizeof (descriptor->driver_name);
    strlcpy (descriptor->driver_name, sz, cb);
  }
  else {
    strcpy (descriptor->driver_name, "memory");
    ib = 0;
  }
  cb = strlen (descriptor->driver_name);

  {
    extern char APEX_DRIVER_START;
    extern char APEX_DRIVER_END;
    struct driver_d* d;
    struct driver_d* d_match = NULL;
    for (d = (struct driver_d*) &APEX_DRIVER_START;
	 d < (struct driver_d*) &APEX_DRIVER_END;
	 ++d) {
      if (d->name && strnicmp (descriptor->driver_name, d->name, cb) == 0) {
	if (d_match)
	  return ERROR_AMBIGUOUS;
	d_match = d;
	if (descriptor->driver_name[cb] == 0)
	  break;
      }
    }
    descriptor->driver = d_match;
  }

  if (!descriptor->driver)
    return ERROR_NODRIVER;

  while (sz[ib]) {
    char* pchEnd;
    switch (sz[ib]) {
    case '@':
      ++ib;
    case '0'...'9':
      descriptor->start = simple_strtoul (sz + ib, &pchEnd, 0);
      ib = pchEnd - sz;
      break;
    case '#':
      ++ib;
      descriptor->length = simple_strtoul (sz + ib, &pchEnd, 0);
      ib = pchEnd - sz;
      break;
    default:
      ++ib;
      /* Skip over unknown bytes so that we always terminate */
      break;
    }
  }
  return 0;
}

int open_descriptor (struct open_d* descriptor)
{
  return (descriptor->fh = descriptor->driver->open (descriptor));
}

void close_descriptor (struct open_d* descriptor)
{
  if (descriptor->fh != -1)
    descriptor->driver->close (descriptor->fh);
}
