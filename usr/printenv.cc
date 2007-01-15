/* printenv.cc

   written by Marc Singer
   23 Mar 2006

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

   Program to read the APEX environment from user-land.  This is a
   very crude implementation.  It doesn't check that the environment's
   magic number matches APEX's expected magic number.  If APEX ever
   changes to a general purpose environment, where variables can be
   added by the user, the ENV_LINK_MAGIC number will change and this
   code will have to adapt.

   This implementation isn't terribly robust.  It doesn't parse the
   environment region very carefully.  It really only works when the
   region is of the form "nor:start+length" where start and length of
   decimal integers optinally followed by 'k' for a 1024 multiplier.
   Generally, this will work, but it ought to allow for hexadecimal
   values as well.

   This implementation is smart enough to detect the new environment
   format where it is possible to read the environment keys without
   scanning through the APEX binary.  It still isn't possible to know
   the default environment values without reading APEX, so this code
   will still probe into the APEX binary.

   There ought to be a switch that allows the user to read the
   environment region without reading APEX, thus only seeing the
   values that are modified and stored in the environment.

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


   Endianness
   ----------

   The code that scans for the presence of the APEX environment link
   structure will check for a reversed copy of the link magic.  If
   found, it triggers the code to swab32() all of the words to correct
   for the endianness mismatch.  It will assume that the environment
   does not need to be swapped.

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

#define DEVICEBASE "/dev/mtdblock"
#define DEVICE DEVICEBASE "2"

//#define TALK

#if defined (TALK)
# define PRINTF(f ...)	printf(f)
#else
# define PRINTF(f ...)
#endif

typedef unsigned short u16;
typedef unsigned long u32;

struct descriptor {
  char driver[80];
  unsigned long start;
  unsigned long length;
};

struct entry {
  entry () { index = 0xff; }
  int index;			// index of this variable in APEX or 0x7f
  const char* key;
  const char* value;
};


bool endian_mismatch;
int  env_link_version;

static inline u32 swab32(u32 l)
{
  return (  ((l & 0x000000ffUL) << 24)
	  | ((l & 0x0000ff00UL) << 8)
	  | ((l & 0x00ff0000UL) >> 8)
	  | ((l & 0xff000000UL) >> 24));
}

static inline u32 swab32_maybe (u32 l)
{
  return endian_mismatch ? swab32 (l) : l;
}

static inline void swab32_block_maybe (void* pv, int cb)
{
  if (!endian_mismatch)
    return;

  u32* pl = (u32*) pv;
  cb /= 4;
  for (; cb--; ++pl)
    *pl = swab32 (*pl);
}


int compare_env (const void* pv1, const void* pv2)
{
  return strcasecmp (((struct env_d*) pv1)->key,
		     ((struct env_d*) pv2)->key);
}

void dumpw (const void* pv, int cb, unsigned long index, int width)
{
#if defined (TALK)
  const unsigned char* rgb = (const unsigned char*) pv;
  int i;

  while (cb > 0) {
    printf ("%08lx: ", index);
    for (i = 0; i < 16; ++i) {
      if (i < cb) {
	switch (width) {
	default:
	case 1:
	  printf ("%02x ", rgb[i]);
	  break;
	case 2:
	  printf ("%04x ", *((u16*)&rgb[i]));
	  ++i;
	  break;
	case 4:
	  printf ("%08x ", *((u32*)&rgb[i]));
	  i += 3;
	  break;
	}
      }
      else
	printf ("   ");
      if (i%8 == 7)
	putchar (' ');
    }
    for (i = 0; i < 16; ++i) {
      if (i == 8)
	putchar (' ');
      putchar ( (i < cb) ? (isprint (rgb[i]) ? rgb[i] : '.') : ' ');
    }
    printf ("\n");

    cb -= 16;
    index += 16;
    rgb += 16;
  }
#endif
}

void copy_string (void* pv, const struct env_link& env_link, char** szCopy)
{
  char* szSource = (char*) pv + (*szCopy - (char*) env_link.apex_start);
  int cb = strlen (szSource);
  char* sz = new char[cb + 1];
  strcpy (sz, szSource);
  *szCopy = sz;
}


char* open_environment (struct descriptor* d)
{
  MTDPartition mtd;

  //  printf ("environment %s %d %d\n", d->driver, d->start, d->length);

  if (strcasecmp (d->driver, "nor") == 0)
    mtd = MTDPartition::find (d->start);
  else
    ;

  if (!mtd.is ())
    return NULL;

  char sz[80];
  snprintf (sz, sizeof (sz), "/dev/%s", mtd.device);

  int fh = open (sz, O_RDONLY);
  if (fh == -1)
    return NULL;
  lseek (fh, d->start - mtd.base, SEEK_SET);

  char* env = new char[d->length];
  read (fh, env, d->length);
  close (fh);
  return env;
}


/* parse_region

   performs a simplified parse of a region descriptor
*/

struct descriptor parse_region (const char* sz)
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


/* scan_environment

   is the guts of this program.  It scans the environment,
   constructing a map of the environment variables that it finds.
   These will be all of the entries that exist in non-volatile memory,
   but these may not all have corresponding entries among APEXs
   environment descriptors.  If it is given an env_d it will fill in
   the index field for the entries.

   The return value is the count of unique ids found, or -1 if the is
   environment is unrecognized.

*/

int scan_environment (struct env_d* env, int c_env, void* pv,
		      struct entry* rgEntry)
{
//  dumpw (pv, 256, 0, 1);

  const char* pb = (const char*) pv;

  if (pb[0] == 0xff && pb[1] == 0xff) {
    PRINTF ("# empty environment\n");
    return 0;
  }

  if (pb[0] != ENV_MAGIC_0 || pb[1] != ENV_MAGIC_1) {
    PRINTF ("# environment contains unrecognized data\n");
    return -1;
  }

  if (c_env <= 0 || env == NULL)
    c_env = 0;

  int c = 0;

  pb += 2;

  while (*pb != 0xff) {
    char flags = *pb++;
    int id = (unsigned char) flags & 0x7f;
    if (rgEntry[id].index == 0xff) {
      rgEntry[id].key = pb;

//      int cb = strlen ((char*) pb);
//      rgEntry[id].key = new char [cb + 1];
//      strcpy (rgEntry[id].key, (char*) pb);
      PRINTF ("# found %s\n", rgEntry[id].key);
      pb += strlen (pb) + 1;
      rgEntry[id].index = 0x7f;
      for (int index = 0; index < c_env; ++index)
	if (strcasecmp (rgEntry[id].key, env[index].key) == 0) {
	  rgEntry[id].index = index;
	  break;
	}
      ++c;
    }
    PRINTF ("# %s = %s\n", rgEntry[id].key, pb);
//    int cb = strlen ((char*) pb);
    if (flags & 0x80)
      rgEntry[id].value = pb;
    pb += strlen (pb) + 1;
  }
  return c;
}


/* show_environment

   knits together the two different kinds of environment data and
   displays the values.

*/

void show_environment (struct env_d* env, int c_env,
		       struct entry* rgEntry, int c_entry)
{
  char rgId[127];
  memset (rgId, 0xff, sizeof (rgId));

  if (c_entry < 0)
    c_entry = 0;

  for (int i = 0; i < c_env; ++i) {
    const char* value = NULL;
    for (int j = 0; j < c_entry; ++j)
      if (rgEntry[j].index == i) {
	value = rgEntry[j].value;
	break;
      }

    if (value)
      printf ("%s = %s\n", env[i].key, value);
    else
      printf ("%s *= %s\n", env[i].key, env[i].default_value);
  }
  for (int j = 0; j < c_entry; ++j) {
    if (rgEntry[j].index == 0x7f)
      printf ("%s #= %s\n", rgEntry[j].key, rgEntry[j].value);
  }
}




int main (int argc, char** argv)
{
  int fh = open (DEVICE, O_RDONLY);

  if (fh == -1) {
    printf ("unable to open %s\n", DEVICE);
    return 1;
  }

  void* pv = mmap (NULL, 1024, PROT_READ, MAP_SHARED, fh, 0);
  if (pv == MAP_FAILED) {
    printf ("unable to mmap\n");
    return 1;
  }

  int index_env_link = 0;
  size_t cbApex = 0;

  {
    unsigned long* rgl = (unsigned long*) pv;
    for (int i = 0;
	 i < 1024/sizeof (unsigned long)
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

  if (!index_env_link) {
    printf ("no env_link\n");
    return 1;
  }

  PRINTF ("index_env_link %d #%d %d swapped\n",
	  index_env_link, env_link_version, endian_mismatch);

  munmap (pv, 4096);

  pv = mmap (NULL, cbApex, PROT_READ, MAP_PRIVATE, fh, 0);
  if (pv == NULL || pv == (void*) 0xffffffff) {
    printf ("unable to mmap\n");
    return 1;
  }

  char rgb[1024];		// *** fixme wrt mmap length
  bzero (rgb, sizeof (rgb));
  struct env_link& env_link = *(struct env_link*) rgb;

  switch (env_link_version) {
  case 1:
    {
      const struct env_link_1& env_link_1
	= *(const struct env_link_1*) ((unsigned char*) pv + index_env_link);
      env_link.magic = env_link_1.magic;
      env_link.apex_start = env_link_1.apex_start;
      env_link.apex_end = env_link_1.apex_end;
      env_link.env_start = env_link_1.env_start;
      env_link.env_end = env_link_1.env_end;
      env_link.env_link		// *** Hack to accomodate partition w/swap
	= (void*) swab32_maybe (swab32_maybe ((u32) env_link.apex_start)
				+ index_env_link - 16);
      env_link.env_d_size = env_link_1.env_d_size;
      memcpy ((void*) env_link.region, env_link_1.region,
	      sizeof (rgb) - sizeof (struct env_link));
    }
    break;

  case 2:
    memcpy (rgb, (const char*) pv + index_env_link, sizeof (rgb));
    break;
  }

  swab32_block_maybe (rgb, sizeof (rgb));

	// Compute offset within pv of the APEX image
  int mapping_offset = index_env_link
    - ((char*) env_link.env_link - (char*) env_link.apex_start);

	// Fixup the whole APEX image so we don't have to fuss
  void* pvApex = pv;
  pv = new char[cbApex];
  memcpy (pv, (char*) pvApex + mapping_offset, cbApex);
  swab32_block_maybe (pv, cbApex);

  int c_env = ((char*) env_link.env_end - (char*) env_link.env_start)
    /env_link.env_d_size;

  PRINTF ("# env_link.magic      0x%lx\n", env_link.magic);
  PRINTF ("# env_link.apexrelease %s\n", env_link.apexrelease);
  PRINTF ("# env_link.apex_start 0x%lx\n", env_link.apex_start);
  PRINTF ("# env_link.apex_end   0x%lx\n", env_link.apex_end);
  PRINTF ("# env_link.env_start  0x%lx\n", env_link.env_start);
  PRINTF ("# env_link.env_end    0x%lx\n", env_link.env_end);
  PRINTF ("# env_link.env_link   0x%lx\n", env_link.env_link);
  PRINTF ("# env_link.env_d_size 0x%lx\n", env_link.env_d_size);
  PRINTF ("# c_env               %d\n", c_env);
  PRINTF ("# env_link.env_region %s\n", env_link.region);

  struct descriptor d = parse_region (env_link.region);
  char* pvEnv = open_environment (&d);

  PRINTF ("# environment at 0x%x+0x%x -> %p\n", d.start, d.length, pvEnv);

  dumpw (pvEnv, 256, 0, 1);


//  printf ("link 0x%x %d %s\n",
//	  (char*) env_link.env_start - (char*) env_link.apex_start,
//	  c_env, env_link.region);

	// --- Pull the environment descriptors from APEX

  struct env_d* env = new struct env_d[c_env];
  memcpy (env, (const char*) pv
	  + ((char *) env_link.env_start - (char*) env_link.apex_start),
	  c_env*sizeof (struct env_d));
//  dumpw (env, c_env*sizeof (struct env_d), 0, 0);

  for (int i = 0; i < c_env; ++i) {
    copy_string (pv, env_link, &env[i].key);
    copy_string (pv, env_link, &env[i].default_value);
    copy_string (pv, env_link, &env[i].description);
    PRINTF ("# env %s %s (%s)\n",
	    env[i].key, env[i].default_value, env[i].description);
//    printf ("%s *= %s\n", env[i].key, env[i].default_value);
  }

  qsort (env, c_env, sizeof (struct env_d), compare_env);

  struct entry rgEntry[127];	// Number of unique entries permitted
  int c_entry = scan_environment (env, c_env, pvEnv, rgEntry);
  PRINTF ("# %d entries in environment\n", c_entry);
  show_environment (env, c_env, rgEntry, c_entry);

  delete (char*) pv;
  munmap (pvApex, cbApex);
  munmap (pvEnv, d.length);
  close (fh);

  return 0;
}
