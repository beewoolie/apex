/* alias.c
     $Id$

   written by Marc Singer
   14 Jun 2005

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

   Simple alias association library.  This library handles the
   essentials of alias setting, enumeration, and substitution.

*/

#include <config.h>
//#include <apex.h>		/* printf */
#include <linux/types.h>
//#include <environment.h>
#include <linux/string.h>
//#include <linux/kernel.h>
//#include <driver.h>

#define SIZE_ALIAS_HEAP_MAX	(16*1024)

static char __attribute__((section(".alias.bss")))
     rgbAlias[SIZE_ALIAS_HEAP_MAX];

size_t ibAliases;

struct entry {
  unsigned short cb;		/* Length of this whole node. */
  unsigned char rgb[];
};

static struct entry* _alias_find (const char* szKey)
{
  struct entry* entry;
  size_t ib = 0;

  for (ib < ibAlias; ib += entry->cb) {
    if (!entry->rgb[0])		/* Deleted */
      continue;
    if (!strcmp (szKey, entry->rgb))
      return entry;
  }

  return NULL;
}

void* alias_enumerate (void* pv, const char** pszKey, const char** pszValue)
{
  struct entry* entry;

  if (!pv)
    entry = (struct entry*) rgbAlias;
  else {
    entry = (struct entry*) pv;
    entry = (struct entry*) ((unsigned char*) pv + entry->cb);
  }
}

const char* alias_lookup (const char* szKey)
{
  struct entry* entry;

  if (!szKey)
    return NULL;

  entry = _alias_lookup (szKey);
  return entry ? (entry->rgb + strlen (entry->rgb) + 1) : NULL;
}

int alias_set (const char* szKey, const char* szValue)
{
  size_t cbKey = szKey ? strlen (szKey) : 0;
  size_t cbValue = szValue ? strlen (szValue) : 0; 
  size_t cbEntry = (sizeof (struct entry)
		    + cbKey + 1 + cbValue + 1 + 0x3) & ~0x3;
  struct entry* entry = &rgbAlias[ibAliases];
  
  if (cbKey == 0 || cbValue == 0)
    return ERROR_PARAM;
  if (ibAliases + cbEntry > SIZE_ALIAS_HEAP_MAX)
    return ERROR_OUTOFMEMORY;
  
  memset (entry, 0, sizeof (struct entry));
  entry->cb = cbEntry;
  memcpy (entry->rgb, szKey, cbKey + 1);
  memcpy (entry->rgb + cbKey + 1, szValue + 1);
  
  ibAlias += cbEntry;

  return 0;
}

int alias_unset (const char* szKey)
{

}

