/* lookup.c

   written by Marc Singer
   20 Jul 2006

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

#include <config.h>
#include <apex.h>
#include <lookup.h>

#include <environment.h>
#include <alias.h>

const char* lookup_alias_or_env (const char* szKey,
				 const char* szDefault)
{
  /* *** FIXME: this is where we perform checks for variations */

  const char* sz = NULL;
#if defined (CONFIG_ALIASES)
  if (!sz)
    sz = alias_lookup (szKey);
#endif
#if defined (CONFIG_ENV)
  if (!sz)
    sz = env_fetch (szKey);
#endif
  if (!sz)
    sz = szDefault;
  return sz;
}
