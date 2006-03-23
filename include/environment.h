/* environment.h

   written by Marc Singer
   7 Nov 2004

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

*/

#if !defined (__ENVIRONMENT_H__)
#    define   __ENVIRONMENT_H__

#include <attributes.h>

/* ----- Types */

struct env_d {
  char* key;
  char* default_value;
  char* description;
};

struct env_link {
  unsigned long magic;
  void* apex_start;
  void* apex_end;
  void* env_start;
  void* env_end;
  unsigned long env_d_size;
  const char region[];
};

#define __env  __used __section(.env)

#define ENV_CB_MAX	(512)

#define ENV_LINK_MAGIC	(unsigned long)(('A'<<0)|('E'<<8)|('L'<<16)|('0'<<24))

/* ----- Prototypes */

const char* env_fetch (const char* szKey);
int	    env_fetch_int (const char* szKey, int valueDefault);
int	    env_store (const char* szKey, const char* szValue);
void	    env_erase (const char* szKey);
void*	    env_enumerate (void* pv, const char** pszKey,
			   const char** pszValue, int* fDefault);
void        env_erase_all (void);
int	    env_check_magic (void);


#endif  /* __ENVIRONMENT_H__ */
