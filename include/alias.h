/* alias.h
     $Id$

   written by Marc Singer
   6 July 2005

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

#if !defined (__ALIAS_H__)
#    define   __ALIAS_H__

/* ----- Types */

/* ----- Prototypes */

void*       alias_enumerate (void* pv, const char** pszKey, 
			     const char** pszValue);
const char* alias_lookup (const char* szKey);
int alias_set (const char* szKey, const char* szValue);
int alias_unset (const char* szKey);

#endif  /* __ALIAS_H__ */
