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

void close_descriptor (struct descriptor_d* d)
{
  d->length = 0;
}

int is_descriptor_open (struct descriptor_d* d)
{
  return d->length != 0;
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

  while (sz[ib]) {
    char* pchEnd;
    unsigned long* pl = 0;

    switch (sz[ib]) {
    case '@':
      ++ib;
    case '0'...'9':
      pl = &d->start;
      break;
    case '#':
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
  return 0;
}

size_t seek_descriptor (struct descriptor_d* d, ssize_t ib, int whence)
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
