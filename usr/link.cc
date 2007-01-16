/* link.cc

   written by Marc Singer
   15 Jan 2007

   Copyright (C) 2007 Marc Singer

   -----------
   DESCRIPTION
   -----------

   Endianness
   ----------

   The code that scans for the presence of the APEX environment link
   structure checks for a byte-reversed copy of the link magic number.
   If found, it triggers the code to swab32() all of the words to
   correct for the endianness mismatch.  It will assume that the flash
   environment does not need to be swapped.


   Environment Format
   ------------------

   The in-flash format of the user's contribution to the environment
   has the following format in types 1 and 2 of the environment link.
   Note that the endianness of the in-flash environment shouldn't
   matter since the writer of the environment needs to honor the
   essential endian-ness of the system.  On systems where there is no
   ambiguity between the inherent endianness and the run-time
   endianness, all is equivalent.  On systems where we run in one
   mode, but the system was engineers for the other, the flash drivers
   ought to swap so that we can read the FIS Directory.


     +---+---+----------+------+
     | A | e | Entry... | 0xff |
     +---+---+----------+------+

   The magic bytes, A and e preceed the environment.  Following it are
   the entries with 0xff being the first byte after the last valid
   entry.  Entries have one of two forms depending on whether or not
   the key already exists in the environment.

     +----+-----+----+-------+----+
     | ID | Key | \0 | Value | \0 |
     +----+-----+----+-------+----+

   The high bit of the ID is 1 when the entry is valid and zero when
   the entry is erased.  The lower seven bits are an index, starting
   at zero, of the unique keys in the environment.  The first time an
   environment entry is added to the environment, an ID is allocated
   and the above format is emitted.  The second time the same key is
   used, the Key string is omitted and the entry looks as follows.

     +----+-------+----+
     | ID | Value | \0 |
     +----+-------+----+

   This scheme replaces an earlier one that used the key values in the
   APEX image as the Key values.  The trouble with that format is that
   it required the in-flash format to match the APEX executable in a
   way that was impossible to guarantee even though a checksum of the
   available environment variables was used as the magic for the
   environment.  The current format allows for APEX upgrades without
   losing the user's environment.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/io.h>
#include <ctype.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#include "environment.h"
#include "mtdpartition.h"
#include "link.h"

//#define TALK

#if defined (TALK)
# define PRINTF(f ...)	printf(f)
#else
# define PRINTF(f ...)
#endif

typedef unsigned short u16;
typedef unsigned long u32;

#define CB_LINK_SCAN	(4096)

struct descriptor {
  char driver[80];
  unsigned long start;
  unsigned long length;
};

static int compare_env (const void* pv1, const void* pv2)
{
  return strcasecmp (((struct env_d*) pv1)->key,
		     ((struct env_d*) pv2)->key);
}


/* parse_region

   performs a simplified parse of a region descriptor

*/

static struct descriptor parse_region (const char* sz)
{
  struct descriptor d;
  memset (&d, 0, sizeof (d));

  char* pch;
  if ((pch = index (sz, ':'))) {
    int c = pch - sz;
    if (c > sizeof (d.driver) - 1)
      c = sizeof (d.driver) - 1;
    memcpy (d.driver, sz, c);
    d.driver[c] = 0;
    sz = pch + 1;
  }
  if (sz[0] == '0' && sz[1] == 'x')
    d.start = strtoul (sz, (char**) &sz, 16);
  else
    d.start = strtoul (sz, (char**) &sz, 10);
  if (*sz == 'k' || *sz == 'K') {
    ++sz;
    d.start *= 1024;
  }
  if (*sz == '+') {
    if (sz[0] == '0' && sz[1] == 'x')
      d.length = strtoul (sz, (char**) &sz, 16);
    d.length = strtoul (sz, (char**) &sz, 10);
    if (*sz == 'k' || *sz == 'K') {
      ++sz;
      d.length *= 1024;
    }
  }
  return d;
}


/* Link::open

   opens the link by locating APEX, copying the loader, generating a
   fixed-up env_link structure, scanning for the environment variables
   and therir defaults, and opening the flash instance of the
   environment.  A tall order.

*/

bool Link::open (void)
{
	  // Look for the loader by the name of the partition
  MTDPartition mtd = MTDPartition::find ("Loader");

  bool fFound = mtd.is () && open_apex (mtd);

  if (!fFound) {
		// Scan all partitions for APEX
    printf ("no Loader partition\n");
    return false;
  }

  load_env ();
  map_environment ();
  scan_environment ();

  PRINTF ("# %d entries in environment\n", cEntries);
  //  show_environment (env, c_env, rgEntry, c_entry);

  return true;
}


/* Link::load_env

   loads the environment from the APEX image and sorts the entries

*/

int Link::load_env (void)
{
  c_env = ((char*) env_link->env_end - (char*) env_link->env_start)
    /env_link->env_d_size;

  env = new struct env_d[c_env];
  memcpy (env, (const char*) pvApex
	  + ((char *) env_link->env_start - (char*) env_link->apex_start),
	  c_env*sizeof (struct env_d));
//  dumpw (env, c_env*sizeof (struct env_d), 0, 0);

  long delta =  (long) pvApex - (long) env_link->apex_start;
  for (int i = 0; i < c_env; ++i) {
    env[i].key += delta;
    env[i].default_value += delta;
    env[i].description += delta;
    printf ("# env %s %s (%s)\n",
	    env[i].key, env[i].default_value, env[i].description);
  }

  qsort (env, c_env, sizeof (struct env_d), compare_env);

  return c_env;
}


/* Link::map_environment

   locates the user-modifiable environment in flash and maps it.

*/

int Link::map_environment (void)
{
  struct descriptor d = parse_region (env_link->region);

  MTDPartition mtd;
  if (strcasecmp (d.driver, "nor") == 0)
    mtd = MTDPartition::find (d.start);

  if (!mtd.is ())
    return -1;

  fhEnv = ::open (mtd.dev (), O_RDWR);
  cbEnv = d.length;
  pvEnv = ::mmap (NULL, cbEnv, PROT_READ | PROT_WRITE, MAP_SHARED, fhEnv, 0);

  return cbEnv;
}


/* Link::open_apex

   reads APEX from the given partition.  It returns false if APEX
   wasn't found.

*/

bool Link::open_apex (const MTDPartition& mtd)
{
  printf ("%s: '%s'\n", __FUNCTION__, mtd.dev ());
  int fh = ::open (mtd.dev (), O_RDONLY);
  if (fh == -1)
    return false;

  void* pv = mmap (NULL, CB_LINK_SCAN, PROT_READ, MAP_SHARED, fh, 0);
  if (pv == MAP_FAILED) {
    printf ("failed to mmap\n");
    close (fh);
    return false;
  }

  printf ("mmaped\n");

  int index_env_link = 0;
  env_link_version = 0;
  cbApex = 0;

  {
    unsigned long* rgl = (unsigned long*) pv;
    for (int i = 0;
	 i < CB_LINK_SCAN/sizeof (unsigned long)
	   - sizeof (env_link)/sizeof (unsigned long);
	 ++i) {
      //      printf ("%d 0x%x (%x %x)\n",
      //	      i, rgl[i], ENV_LINK_MAGIC, swab32 (ENV_LINK_MAGIC));

      switch (rgl[i]) {
      case ENV_LINK_MAGIC_1:
	env_link_version = 1;
	break;
      case ENV_LINK_MAGIC:
	env_link_version = 2;
	break;
      }

      switch (swab32 (rgl[i])) {
      case ENV_LINK_MAGIC_1:
	endian_mismatch = true;
	env_link_version = 1;
	break;
      case ENV_LINK_MAGIC:
	endian_mismatch = true;
	env_link_version = 2;
	break;
      }

      if (!env_link_version)
	continue;

      switch (env_link_version) {
      case 1:
	{
	  struct env_link_1* env_link = (struct env_link_1*) &rgl[i];
	  index_env_link = i*sizeof (unsigned long);
	  cbApex = (char*) swab32_maybe ((u32) env_link->apex_end)
	    - (char*) swab32_maybe ((u32) env_link->apex_start);
	}
	break;
      case 2:
	{
	  struct env_link* env_link = (struct env_link*) &rgl[i];
	  index_env_link = i*sizeof (unsigned long);
	  cbApex = (char*) swab32_maybe ((u32) env_link->apex_end)
	    - (char*) swab32_maybe ((u32) env_link->apex_start);
	}
	break;
      }
      break;
    }
  }

  munmap (pv, CB_LINK_SCAN);

  if (!index_env_link) {
    close (fh);
    return false;
  }

  pv = mmap (NULL, cbApex, PROT_READ, MAP_PRIVATE, fh, 0);
  if (pv == MAP_FAILED) {
    close (fh);
    return false;
  }

  int cbLink = (sizeof (struct env_link) + 1024 + 4096 - 1) & ~4096;
  env_link = (struct env_link*) new char[cbLink];

  switch (env_link_version) {
  case 1:
    {
      const struct env_link_1& env_link_1
	= *(const struct env_link_1*) ((unsigned char*) pv + index_env_link);
      env_link->magic = env_link_1.magic;
      env_link->apex_start = env_link_1.apex_start;
      env_link->apex_end = env_link_1.apex_end;
      env_link->env_start = env_link_1.env_start;
      env_link->env_end = env_link_1.env_end;
      env_link->env_link	// *** Hack to accomodate partition w/swap
	= (void*) swab32_maybe (swab32_maybe ((u32) env_link->apex_start)
				+ index_env_link - 16);
      env_link->env_d_size = env_link_1.env_d_size;
      memcpy ((void*) env_link->region, env_link_1.region,
	      cbLink - sizeof (struct env_link));
    }
    break;

  case 2:
    memcpy (env_link, (const char*) pv + index_env_link, cbLink);
    break;
  }

  swab32_block_maybe (env_link, cbLink);

  mapping_offset = index_env_link
    - ((char*) env_link->env_link - (char*) env_link->apex_start);

  // *** FIXME: due to mapping_offset it could be the case that we
  // haven't mapped enough of APEX.
  pvApex = (void*) new char[cbApex];
  memcpy (pvApex, (const char*) pv + mapping_offset, cbApex);

  munmap (pv, cbApex);
  close (fh);

  return true;
}


/* Link::scan_environment

   is the essential routine of this class.  It scans the flash
   environment and builds the array of entries.  These will be all of
   the entries that exist in non-volatile memory, but these may not
   all have corresponding entries among APEXs environment descriptors.

   The return value is the count of unique ids found, or -1 if the is
   environment is unrecognized.

*/

int Link::scan_environment (void)
{
  const char* pb = (const char*) pvEnv;

  if (entries)
    delete entries;

  int cEntriesMax = c_env*2;
  entries = new struct entry[cEntriesMax];
  cEntries = 0;

  if (pb[0] == 0xff && pb[1] == 0xff) {
    PRINTF ("# empty environment\n");
    return cEntries;
  }

  if (pb[0] != ENV_MAGIC_0 || pb[1] != ENV_MAGIC_1) {
    PRINTF ("# environment contains unrecognized data\n");
    return -1;
  }

  pb += 2;

  const char* pbEnd = pb + cbEnv;
  while (pb < pbEnd && *pb != 0xff) {
    char flags = *pb++;
    int id = (unsigned char) flags & 0x7f;
    if (entries[id].index == 0xff) {
      entries[id].key = pb;

//      int cb = strlen ((char*) pb);
//      rgEntry[id].key = new char [cb + 1];
//      strcpy (rgEntry[id].key, (char*) pb);
      PRINTF ("# found %s\n", entries[id].key);
      pb += strlen (pb) + 1;
      entries[id].index = 0x7f;
      for (int index = 0; index < c_env; ++index)
	if (strcasecmp (entries[id].key, env[index].key) == 0) {
	  entries[id].index = index;
	  break;
	}
      ++cEntries;
    }
    PRINTF ("# %s = %s\n", entries[id].key, pb);
//    int cb = strlen ((char*) pb);
    if (flags & 0x80)
      entries[id].value = pb;
    pb += strlen (pb) + 1;
  }

  return cEntries;
}


/* Link::show_environment

   knits together the two different kinds of environment data and
   displays the values.

*/

void Link::show_environment (void)
{
  char rgId[127];
  memset (rgId, 0xff, sizeof (rgId));

  for (int i = 0; i < c_env; ++i) {
    const char* value = NULL;
    for (int j = 0; j < cEntries; ++j)
      if (entries[j].index == i) {
	value = entries[j].value;
	break;
      }

    if (value)
      printf ("%s = %s\n", env[i].key, value);
    else
      printf ("%s *= %s\n", env[i].key, env[i].default_value);
  }
  for (int j = 0; j < cEntries; ++j) {
    if (entries[j].index == 0x7f)
      printf ("%s #= %s\n", entries[j].key, entries[j].value);
  }
}
