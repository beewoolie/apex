/* driver.h
     $Id$

   written by Marc Singer
   1 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__DRIVER_H__)
#    define   __DRIVER_H__

/* ----- Includes */

#include <linux/types.h>

/* ----- Types */

struct driver_d;

struct open_d {
  int fh;			/* file handle */
  struct driver_d* driver;
  char driver_name[16];
  unsigned long start;
  unsigned long length;
};

#define DRIVER_LENGTH_MAX	(0x7fffffff)

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

#define DRIVER_SERIAL	(1<<1)
#define DRIVER_CONSOLE	(1<<2)
#define DRIVER_MEMORY	(1<<3)
#define DRIVER_PRESENT	(1<<8)

#define driver_can_seek(p)  ((p)->seek != NULL)
#define driver_can_read(p)  ((p)->read != NULL)
#define driver_can_write(p) ((p)->write != NULL)

struct driver_d {
  const char* name;
  const char* description;
  unsigned long flags;
  void* priv;			/* Driver's private data */
  int           (*probe) (void);
  unsigned long (*open)  (struct open_d*);
  void		(*close) (unsigned long fd);
  ssize_t	(*read)  (unsigned long fd, void* pv, size_t cb);
  ssize_t	(*write) (unsigned long fd, const void* pv, size_t cb);
  ssize_t	(*poll)  (unsigned long fh, size_t cb);
  void		(*erase) (unsigned long fd, size_t cb);
  size_t	(*seek)  (unsigned long fd, ssize_t cb, int whence);
  void		(*info)  (void); /* User information request */
};

#define __driver  __attribute__((used,section(".driver")))
#define __driver_0 __attribute__((used,section(".driver_0")))
#define __driver_1 __attribute__((used,section(".driver_1")))
#define __driver_2 __attribute__((used,section(".driver_2")))
#define __driver_3 __attribute__((used,section(".driver_2")))

/* ----- Globals */

/* ----- Prototypes */

extern int parse_descriptor (const char* sz, struct open_d* descriptor);
extern int open_descriptor (struct open_d* descriptor);
extern void close_descriptor (struct open_d* descriptor);

#endif  /* __DRIVER_H__ */
