/* env.c
     $Id$

   written by Marc Singer
   15 Oct 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

   Code to handle an environment.  The application can maintain a
   persistent environment in flash with key/value pairs.  These
   routines read and write the environment.  It is possible
   (improbable) that an environment may be maintained within eeprom
   (~128B) where there will be some adjustment to the algorithms.  For
   example, in eeprom, there is no reason to maintain the erased
   values.  Instead, we'd rewrite the whole kit every time.


   TODO
   ----

   - We may need to add a magic number to the top of the environment
     so that the indices used in non-volatile storage are guaranteed
     to match the APEX variable list.
   - There should be a checksum at the top of the environment that
     make it impossible to load the values incorrectly.  
   - There may be a desire to be able to erase all of the environment
     and start again.  This might mitigate the issue of using EEPROM
     in that we would force the user to do the erase by hand.
   - Use a strcasecmp instead of strcmp when looking for keys.

*/

#include <config.h>
#include <linux/types.h>
#include <environment.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <driver.h>

extern char APEX_ENV_START;
extern char APEX_ENV_END;
extern char APEX_VMA_START;
extern char APEX_VMA_END;

#define ENV_CB_MAX	(512)
#define ENV_MASK_DELETED (0x80)
#define ENV_VAL_DELETED	 (0x00)
#define ENV_VAL_NOT_DELETED (0x80)
#define ENV_INDEX(m)	((int)((m) & 0x7f))
#define ENV_END		(0xff)
#define ENV_IS_DELETED(m) (((m) & ENV_MASK_DELETED) == ENV_VAL_DELETED)
#define C_ENV_KEYS	(((unsigned int) &APEX_ENV_END \
			- (unsigned int) &APEX_ENV_START)\
			/sizeof (struct env_d))
#define ENVLIST(i)	(((struct env_d*) &APEX_ENV_START)[i])

static const char* _env_find (int i);
static int _env_index (const char* sz);

struct descriptor_d env_d;	/* Environment storage region */

/* _env_fetch

   returns the index and value for an environment key.  If the user
   has not set the key, the *pszValue will be NULL.  The return value
   is the key index or -1 if the key is not found.n

*/

static int _env_fetch (const char* szKey, const char** pszValue)
{
  int i = _env_index (szKey);;
  if (i != -1)
    *pszValue = _env_find (i);
  return i;
}

static ssize_t _env_write (const void* pv, size_t cb)
{
  return env_d.driver->write (&env_d, pv, cb);
}

static ssize_t _env_read (void* pv, size_t cb)
{
  return env_d.driver->read (&env_d, pv, cb);
}

static void _env_rewind (void)
{
  env_d.driver->seek (&env_d, 0, SEEK_SET);
}

static char _env_locate (int i)
{
  char ch;

  while (_env_read (&ch, 1) == 1 && ch != ENV_END) {
    if (!ENV_IS_DELETED (ch) && ENV_INDEX (ch) == i) 
      return ch;
    do {
      _env_read (&ch, 1);
    } while (ch);
  }

  return ch;
}


/* _env_find

   looks for a user's set environment data in the non-volatile store.
   The index, i, is the index of the variable in the list of variable
   structures.

*/

static const char* _env_find (int i)
{
  static char rgb[ENV_CB_MAX];

  if (!is_descriptor_open (&env_d))
    return NULL;

  _env_rewind ();

  if (_env_locate (i) != ENV_END) {
    _env_read (rgb, sizeof (rgb));
    return rgb;
  }

  return NULL;
}


static int _env_index (const char* sz) 
{
  int i;

  for (i = 0; i < C_ENV_KEYS; ++i)
    if (strcmp (sz, ENVLIST(i).key) == 0)
      return i;

  return -1;
}


/* env_enumerate

   walks the caller through all of the undeleted environment entries.
   The return value is a handle to the item being enumerated.  The
   function returns NULL when all of the items are enumerated.  When
   the return value is not NULL, *pszKey points to a key and *pszValue
   points to the corresponding value.

*/

void* env_enumerate (void* pv, const char** pszKey, 
		     const char** pszValue, int* pfDefault)
{
  int i = (int) pv + 1;

  if (i - 1 >= C_ENV_KEYS) 
    return 0;

  *pszKey = ENVLIST(i - 1).key;
  *pszValue = _env_find (i - 1);
  if (*pszValue == NULL) {
    *pszValue = ENVLIST(i - 1).default_value;
    *pfDefault = 1;
  }
  else
    *pfDefault = 0;

  return (void*) i;
} 


/* env_fetch

   returns a read-only pointer to the value for the given environment
   key entry.  If there are no matching, not-deleted entries, the
   return value is NULL.

*/

const char* env_fetch (const char* szKey)
{
  const char* szValue;
  int i = _env_fetch (szKey, &szValue);

  if (i != -1 && szValue == NULL)
    return ENVLIST(i).default_value;
  return szValue;
}


int env_fetch_int (const char* szKey, int valueDefault)
{
  const char* sz = env_fetch (szKey);
  char* pbEnd;
  u32 value;

  if (sz == NULL)
    return valueDefault;

  value = simple_strtoul (sz, &pbEnd, 0);
  return sz == pbEnd ? valueDefault : value;
}


/* env_erase

   removes a key from the environment.

*/

void env_erase (const char* szKey)
{
  int i = _env_index (szKey);
  char ch;
  if (i < 0 || !is_descriptor_open (&env_d))
    return;

  _env_rewind ();

  if ((ch = _env_locate (i)) != ENV_END) {
    ch = (ch & ~ENV_MASK_DELETED) | ENV_VAL_DELETED;
    _env_write (&ch, 1);
  }
}


/* env_store

   adds a key/value pair to the environment.  An existing version of
   the key will be erased.  The return value is non-zero if the data
   cannot be written to flash.
 
*/

int env_store (const char* szKey, const char* szValue)
{
  int i = _env_index (szKey);
  int cch = strlen (szValue);
  char ch;

  if (i < 0 || !is_descriptor_open (&env_d) || cch > ENV_CB_MAX - 1)
    return 1;

  _env_rewind ();

  while ((ch = _env_locate (i)) != ENV_END) {
    ch = (ch & ~ENV_MASK_DELETED) | ENV_VAL_DELETED;
    _env_write (&ch, 1);
  }
  ch = ENV_VAL_NOT_DELETED | (i & 0x7f);
  _env_write (&ch, 1);
  _env_write (szValue, cch + 1);

  return 0;
}


