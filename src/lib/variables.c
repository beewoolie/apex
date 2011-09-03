/* variables.c

   written by Marc Singer
   14 Jun 2005

   Copyright (C) 2005 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   Simple variable association library.  This library handles the
   essentials of variable setting, enumeration, and substitution.

*/

#include <config.h>
//#include <apex.h>		/* printf */
#include <attributes.h>
#include <linux/types.h>
//#include <environment.h>
#include <linux/string.h>
#include <linux/kernel.h>
//#include <driver.h>
#include <variables.h>
#include <error.h>

#define CB_VARIABLE_HEAP_MAX	(4*1024)

static unsigned char __xbss(variables) rgbVariables[CB_VARIABLE_HEAP_MAX];

size_t ibVariables;

struct entry {
  unsigned short cb;		/* Length of this whole node. */
  char rgb[];
};

static struct entry* _variable_find (const char* szKey)
{
  struct entry* entry;
  size_t ib = 0;

  for (; ib < ibVariables; ib += entry->cb) {
    entry = (struct entry*) (rgbVariables + ib);
    if (!entry->rgb[0])		/* Deleted */
      continue;
//    printf ("  compare '%s' '%s'\n", szKey, entry->rgb);
    if (strcmp (szKey, entry->rgb) == 0)
      return entry;
  }

  return NULL;
}

void* variables_enumerate (void* pv, const char** pszKey, const char** pszValue)
{
  struct entry* entry;

  if (!pv)
    entry = (struct entry*) rgbVariables;
  else {
    entry = (struct entry*) pv;
    entry = (struct entry*) ((unsigned char*) pv + entry->cb);
  }

  for (; ((unsigned char*) entry - rgbVariables) < ibVariables;
       entry = (struct entry*) ((unsigned char*) entry + entry->cb)) {

    if (!entry->rgb[0])
      continue;

    *pszKey = entry->rgb;
    *pszValue = entry->rgb + strlen (entry->rgb) + 1;
    return entry;
  }

  return 0;
}

const char* variable_lookup (const char* szKey)
{
  struct entry* entry;

  if (!szKey)
    return NULL;

  entry = _variable_find (szKey);
  return entry ? (entry->rgb + strlen (entry->rgb) + 1) : NULL;
}

int variable_set_hex (const char* szKey, unsigned value)
{
  char sz[60];
  snprintf (sz, sizeof (sz), "0x%x", value);
  return variable_set (szKey, sz);
}

int variable_set (const char* szKey, const char* szValue)
{
  size_t cbKey = szKey ? strlen (szKey) : 0;
  size_t cbValue = szValue ? strlen (szValue) : 0;
  size_t cbEntry = (sizeof (struct entry)
		    + cbKey + 1 + cbValue + 1 + 0x3) & ~0x3;
  struct entry* entry = (struct entry*) &rgbVariables[ibVariables];

  if (cbKey == 0 || cbValue == 0)
    return ERROR_PARAM;
  if (ibVariables + cbEntry > CB_VARIABLE_HEAP_MAX)
    return ERROR_OUTOFMEMORY;

  /* Make sure to delete existing copies */
  variable_unset (szKey);

  memset (entry, 0, sizeof (struct entry));
  entry->cb = cbEntry;
  memcpy (entry->rgb, szKey, cbKey + 1);
  memcpy (entry->rgb + cbKey + 1, szValue, cbValue + 1);

  ibVariables += cbEntry;

  return 0;
}

int variable_unset (const char* szKey)
{
  struct entry* entry;

  if (!szKey)
    return ERROR_PARAM;

  entry = _variable_find (szKey);
  if (entry)
    entry->rgb[0] = 0;		/* Delete */
  return entry ? 0 : ERROR_FALSE;
}
