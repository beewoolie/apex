/* environment.h
     $Id$

   written by Marc Singer
   7 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__ENVIRONMENT_H__)
#    define   __ENVIRONMENT_H__

/* ----- Types */

struct env_d {
  char* key;
  char* default_value;
  char* description;
};


#define __env  __attribute__((used,section(".env")))


#define ENV_CB_MAX	(512)


/* ----- Prototypes */

const char* env_fetch (const char* szKey);
int	    env_fetch_int (const char* szKey, int valueDefault);
int	    env_store (const char* szKey, const char* szValue);
void	    env_erase (const char* szKey);
void*	    env_enumerate (void* pv, const char** pszKey, 
			   const char** pszValue, int* fDefault);
void        env_erase_all (void);



#endif  /* __ENVIRONMENT_H__ */
