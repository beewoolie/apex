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
   (improbably) that an environment may be maintained within eeprom
   (~128B) where there will be some adjustment to the algorithms.
   Perhaps the most likely variation will be to limit the keys to a
   predefined set.

   Some likely environment keywords are

     cmdline		   - Default kernel command line
     autoboot		   - Set to non-zero to boot automatically
     autoboot_timeout	   - number of milliseconds before autoboot-ing


   TODO
   ----

   - Seems reasonable to aggregate the keys into an array and leave
     them out of flash since we can only use environment keys that are
     already defined within the program.
     - I think that this has been done, but we need to add a serial
       number to the top of the environment so that we don't try to
       use an environment from an expired set.
   - The previous change also means that we could get rid of the
     length from the marker and put the key ID in there.  The size of
     each entry shall be determined by the presence of a null byte.
     This is no worse (and really much better) than adding a length to
     the header.
   - The environment variables could come from initializations in the
     code.  Each module that wants variables would call here to
     register them.  The indices for each would be transparent since
     the caller would always pass/receive strings.  Moreover, the
     environment would only be valid with the version of the firmware
     that created it.   It would be handy to use special initializers
     to define and reference these keys.  There should be a checksum
     at the top of the environment that make it impossible to load the
     values incorrectly.
   - Setup should come from the lds file and the board configuration.
   - With the keys from an array, we could probably put the
     environment into EEPROM.

   - We need to add a driver link to read and write the non-volatile,
     user data.  With a driver, we will be able to write the
     environment anywhere we want, e.g. EEPROM or flash. 

*/

#include <config.h>
#include <linux/types.h>
#include <environment.h>
#include <linux/string.h>
#include <linux/kernel.h>


extern char APEX_ENV_START;
extern char APEX_ENV_END;
extern char APEX_VMA_START;
extern char APEX_VMA_END;

/* APEX_ENV points to the location in flash memory where the
   environment resides.  It is mean to follow the APEX executable
   directly.  When we start to use a bonafide driver to access the
   environment, this will have to be revised. */
#define APEX_ENV (CONFIG_APEX_LMA + &APEX_VMA_END - &APEX_VMA_START)

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
#define ENV_HEAD	((const char*) APEX_ENV)

static const char* _env_find (int i);
static int _env_index (const char* sz);


/* _env_fetch

   returns the index and value for an environment key.  If the user
   has not set the key, the *pszValue will be NULL.  The return value
   is the key index or -1 if the key is not found.n

*/

static int _env_fetch (const char* szKey, const char** pszValue)
{
  int i = _env_index (szKey);

  if (i != -1)
    *pszValue = _env_find (i);

  return i;
}


static const char* _env_find (int i)
{
  const char* pb = (void*) ENV_HEAD;

  if (i < 0 || i > C_ENV_KEYS)
    return NULL;

  while (*pb != ENV_END) {
    if (ENV_IS_DELETED (*pb) || ENV_INDEX (*pb) != i) {
      for (++pb; *pb != 0; ++pb)
	;
      ++pb;
      continue;
    }
    return pb + 1;
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


/* _env_write

   commits bytes of flash.  pv is the address in flash to write.  rgb
   is a buffer to write, and cb is the length of that buffer.

*/

static void _env_write (const void* pv, const char* rgb, size_t cb)
{
  char rgbBuffer[cb + 8];
  const char* pb = (const char*) pv;
  const char* pbStart = (const char*) ((unsigned long) pb & ~0x3);

  if (cb > ENV_CB_MAX - 8)
    cb = ENV_CB_MAX - 8;

  memcpy (rgbBuffer, pbStart, cb + 8);
  memcpy (rgbBuffer + (pb - pbStart), rgb, cb);
#if 0
  SerialOutputString ("write to 0x");
  SerialOutputHex (pbStart);
  SerialOutputString (" for 0x");
  SerialOutputHex ((cb + (pb - pbStart) + 3) & ~0x3);
  SerialOutputString ("\n");
#endif

#if 0
  flash_write_region ((u32*) pbStart, (u32*) rgbBuffer, 
		      (cb + (pb - pbStart) + 3) >> 2);
#endif
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
  const char* pb = ENV_HEAD;

  if (i < 0 || i > C_ENV_KEYS)
    return;

  while (*pb != ENV_END) {
    char ch;
    if (ENV_IS_DELETED (*pb) || ENV_INDEX (*pb) != i) {
      for (++pb; *pb != 0; ++pb)
	;
      ++pb;
      continue;
    }
    ch = *pb;
    ch = (*pb & ~ENV_MASK_DELETED) | ENV_VAL_DELETED;
    _env_write (pb, &ch, 1);
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
  const char* pb = ENV_HEAD;
  char ch;

  if (i < 0 || i > C_ENV_KEYS)
    return 1;

  while (*pb != ENV_END) {
    if (ENV_IS_DELETED (*pb) || ENV_INDEX (*pb) != i) {
      for (++pb; *pb != 0; ++pb)
	;
      ++pb;
      continue;
    }
    ch = *pb;
    ch = (*pb & ~ENV_MASK_DELETED) | ENV_VAL_DELETED;
    _env_write (pb, &ch, 1);
  }
  ch = ENV_VAL_NOT_DELETED | (i & 0x7f);
  _env_write (pb, &ch, 1);
  _env_write (pb + 1, szValue, strlen (szValue) + 1);

  return 0;
}


