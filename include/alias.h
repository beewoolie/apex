/* alias.h

   written by Marc Singer
   6 July 2005

   Copyright (C) 2005 Marc Singer

   This program is licensed under the terms of the GNU General Public
   License as published by the Free Software Foundation.  Please refer
   to the file debian/copyright for further details.

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
int	    alias_set (const char* szKey, const char* szValue);
int	    alias_unset (const char* szKey);

#endif  /* __ALIAS_H__ */
