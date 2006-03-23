/* printenv.cc

   written by Marc Singer
   23 Mar 2006

   Copyright (C) 2006 Marc Singer

   -----------
   DESCRIPTION
   -----------

   Program to read the APEX environment from user-land.  This is a
   very crude implementation.  It doesn't check that the environment's
   magic number matches APEX's expected magic number.  If APEX ever
   changes to a general purpose environment, where variables can be
   added by the user, the ENV_LINK_MAGIC number will change and this
   code will have to adapt.

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

#define DEVICE "/dev/mtdblock0"

typedef unsigned short u16;
typedef unsigned long u32;

struct descriptor {
  char driver[80];
  unsigned long start;
  unsigned long length;
};

void dumpw (const void* pv, int cb, unsigned long index, int width)
{
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
}

void copy_string (void* pv, const struct env_link& env_link, char** szCopy)
{
  char* szSource = (char*) pv + (*szCopy - (char*) env_link.apex_start);
  int cb = strlen (szSource);
  char* sz = new char[cb + 1];
  strcpy (sz, szSource);
  *szCopy = sz;
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
  d.start = strtoul (sz, (char**) &sz, 10);
  if (*sz == 'k' || *sz == 'K') {
    ++sz;
    d.start *= 1024;
  }
  if (*sz && *sz == '+') {
    d.length = strtoul (sz, (char**) &sz, 10);
    if (*sz == 'k' || *sz == 'K') {
      ++sz;
      d.length *= 1024;
    }
  }
  return d;
}


/* show_environment

   is, more or less, the guts of this program.  It scans the
   environment, mapped to pv, for environment variables that match
   each of the environment variables defined in APEX.

   There are a couple of 'tricky' things that it does.  The +2 when
   assiging pv to pb skips the environment's magic number.  The |x80
   while scanning the variables checks for deleted entries.  The high
   bit is clear for deleted entries.

*/

void show_environment (struct env_d* env, int c_env, void* pv)
{
  //  dumpw (pv, 128, 0, 1);

  for (int i = 0; i < c_env; ++i) {
    //    printf ("%s: i %d\n", __FUNCTION__, i);
    const char* pbFound = NULL;
    for (const char* pb = (const char*) pv + 2; *pb != 0xff; ) {
      //      printf ("%d: %x\n", i, *pb);
      if (*pb == (i | 0x80))
	pbFound = pb;
      pb += 1 + strlen (pb + 1) + 1;
    }
    if (pbFound)
      printf ("%s = %s\n", env[i].key, pbFound + 1);
    else
      printf ("%s *= %s\n", env[i].key, env[i].default_value);
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
      if (rgl[i] == ENV_LINK_MAGIC) {
	struct env_link* env_link = (struct env_link*) &rgl[i];
	cbApex = (char*) env_link->apex_end - (char*) env_link->apex_start;
	index_env_link = i*sizeof (unsigned long);
	break;
      }
    }
  }

  if (!index_env_link) {
    printf ("no env_link\n");
    return 1;
  }

  munmap (pv, 1024);

  pv = mmap (NULL, cbApex, PROT_READ, MAP_PRIVATE, fh, 0);
  if (pv == NULL) {
    printf ("unable to mmap\n");
  }

  const struct env_link& env_link
    = *(const struct env_link*) ((unsigned char*) pv + index_env_link);
  int c_env = ((char*) env_link.env_end - (char*) env_link.env_start)
    /env_link.env_d_size;
  void* pvEnv = NULL;

  struct descriptor d = parse_region (env_link.region);
  if (d.start && d.length) {
    pvEnv = mmap (NULL, d.length, PROT_READ, MAP_PRIVATE, fh, d.start);
    if (pvEnv == MAP_FAILED) {
      printf ("unable to mmap environment");
      return 1;
    }
  }

//  printf ("environment at 0x%x+0x%x -> %p\n", d.start, d.length, pvEnv);

//  printf ("link 0x%x %d %s\n",
//	  (char*) env_link.env_start - (char*) env_link.apex_start,
//	  c_env, env_link.region);

	// --- Pull the environment descriptors from APEX

  struct env_d* env = new env_d[c_env];
  memcpy (env, (const char*) pv
	  + ((char *) env_link.env_start - (char*) env_link.apex_start),
	  c_env*sizeof (struct env_d));
  for (int i = 0; i < c_env; ++i) {
    copy_string (pv, env_link, &env[i].key);
    copy_string (pv, env_link, &env[i].default_value);
    copy_string (pv, env_link, &env[i].description);
//    printf ("%s *= %s\n", env[i].key, env[i].default_value);
  }

  show_environment (env, c_env, pvEnv);

  munmap (pv, cbApex);
  munmap (pvEnv, d.length);
  close (fh);

  return 0;
}
