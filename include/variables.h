/* variables.h

   written by Marc Singer
   6 July 2005

   Copyright (C) 2005 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__VARIABLES_H__)
#    define   __VARIABLES_H__

/* ----- Types */

/* ----- Prototypes */

void*       variables_enumerate (void* pv, const char** pszKey,
                                 const char** pszValue);
const char* variable_lookup     (const char* szKey);
int	    variable_set        (const char* szKey, const char* szValue);
int	    variable_unset      (const char* szKey);
int	    variable_set_hex    (const char* szKey, unsigned value);

#endif  /* __VARIABLES_H__ */
