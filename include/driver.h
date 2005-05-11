/* driver.h
     $Id$

   written by Marc Singer
   1 Nov 2004

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

#if !defined (__DRIVER_H__)
#    define   __DRIVER_H__

/* ----- Includes */

#include <linux/types.h>
#include <attributes.h>

/* ----- Types */

struct driver_d;

struct descriptor_d {
  struct driver_d* driver;
  char driver_name[32];		/* *** FIXME: should be removed */
  unsigned long start;
  unsigned long length;
  size_t index;

//  unsigned non_zero_length:1;	/* Set when the user specified a length */

				/* Paths */
  unsigned char rgb[256];
  unsigned char* pb[32];
  int c;		/* Total elements in path */
  int iRoot;		/* Index of root element */

  unsigned long private;	/* Available to driver */
};

#define DRIVER_LENGTH_MAX	(0x7fffffff)

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

#define DRIVER_SERIAL	(1<<1)
#define DRIVER_CONSOLE	(1<<2)
#define DRIVER_MEMORY	(1<<3)
#define DRIVER_NET	(1<<4)			/* May receive packets */
#define DRIVER_PRESENT	(1<<8)
//#define DRIVER_DESCRIP_REGION	(1<<9)		/* Uses region descriptors */
#define DRIVER_DESCRIP_FS	(1<<10) 	/* Uses filesystem descript. */
#define DRIVER_DESCRIP_STREAM	(1<<11) 	/* Uses stresam descriptors */
#define DRIVER_DESCRIP_NET	(1<<12) 	/* Uses network descriptors */
#define DRIVER_WRITEPROGRESS_MASK (0xf)
#define DRIVER_WRITEPROGRESS_SHIFT (24)
#define DRIVER_WRITEPROGRESS(n) (((n)&0xf)<<24)	/* 2^(N+10) bytes per spin */
#define DRIVER_PRIVATE_SHIFT (16)
#define DRIVER_PRIVATE_MASK  (0xff)
#define DRIVER_PRIVATE(n) (((n)&0xff)<<16)

#define driver_can_seek(p)  ((p)->seek != NULL)
#define driver_can_read(p)  ((p)->read != NULL)
#define driver_can_write(p) ((p)->write != NULL)

struct driver_d {
  const char* name;
  const char* description;
  unsigned long flags;
  void* priv;			/* Driver's private data */
  int		(*open)  (struct descriptor_d*);
  void		(*close) (struct descriptor_d*);
  ssize_t	(*read)  (struct descriptor_d*, void* pv, size_t cb);
  ssize_t	(*write) (struct descriptor_d*, const void* pv, size_t cb);
  ssize_t	(*poll)  (struct descriptor_d*, size_t cb);
  void		(*erase) (struct descriptor_d*, size_t cb);
  size_t	(*seek)  (struct descriptor_d*, ssize_t cb, int whence);
  void		(*info)  (void); /* User information request */
};

#define __driver_0 __used __section(.driver.0) /* serial */
#define __driver_1 __used __section(.driver.1) /* memory */
#define __driver_2 __used __section(.driver.2)
#define __driver_3 __used __section(.driver.3) /* flash */
#define __driver_4 __used __section(.driver.4)
#define __driver_5 __used __section(.driver.5) /* cf */
#define __driver_6 __used __section(.driver.6) /* filesystems */
#define __driver_7 __used __section(.driver.7)

/* ----- Globals */

/* ----- Prototypes */

extern void   close_helper (struct descriptor_d* d);
extern void   close_descriptor (struct descriptor_d* d);
extern int    is_descriptor_open (struct descriptor_d* d);
extern int    open_descriptor (struct descriptor_d* d);
extern int    parse_descriptor (const char* sz, struct descriptor_d* d);
extern size_t seek_helper (struct descriptor_d* d, ssize_t ib, int whence);

#endif  /* __DRIVER_H__ */
