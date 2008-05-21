/* lookup.h

   written by Marc Singer
   20 Jul 2006

   Copyright (C) 2006 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__LOOKUP_H__)
#    define   __LOOKUP_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

const char* lookup_alias_or_env (const char* szKey, const char* szDefault);

#endif  /* __LOOKUP_H__ */
