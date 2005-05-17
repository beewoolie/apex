/* drv-jffs2.c
     $Id$

   written by Marc Singer
   16 May 2005

   Copyright (C) 2005 Marc Singer

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

   CRC32
   -----
   
   The jffs2 filesystem doesn't use the proper CCITT CRC algorithm.
   The polynomial is correct, but they don't invert the incoming CRC
   at the start and the end.  We correct for this by passing ~0 as the
   initial CRC and inverting the computation result.

*/

#include <config.h>
#include <apex.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <spinner.h>
#include <linux/kernel.h>
#include <error.h>

//#define TALK

#if defined TALK
# define PRINTF(v...)	printf (v)
#else
# define PRINTF(v...)	do {} while (0)
#endif

#define ENTRY(l) PRINTF ("%s\n", __FUNCTION__)

#define DRIVER_NAME	"jffs2"

#define MARKER_JFFS2_OLD	0x1984
#define MARKER_JFFS2		0x1985
#define MARKER_JFFS2_REV	0x5981	/* Wrong endian magic */
#define MARKER_EMPTY		0xffff
#define MARKER_DIRTY		0x0000

#define NAME_LENGTH_MAX		254
#define DATA_LENGTH_MIN		128	/* Smallest data node */

#define JFFS2_ROOT_INO		1

enum {
  COMPRESSION_NONE	 = 0x00,
  COMPRESSION_ZERO	 = 0x01,
  COMPRESSION_RTIME	 = 0x02,
  COMPRESSION_RUBINMIPS	 = 0x03,
  COMPRESSION_COPY	 = 0x04,
  COMPRESSION_DYNRUBIN	 = 0x05,
  COMPRESSION_ZLIB	 = 0x06,
  COMPRESSION_LZO	 = 0x07,
  COMPRESSION_LZARI	 = 0x08,
};

#define COMPATIBILITY_MASK	0xc000
#define NODE_ACCURATE		0x2000
#define FEATURE_INCOMPAT	0xc000
#define FEATURE_ROCOMPAT	0xc000
#define FEATURE_RWCOMPAT_COPY	0x4000
#define FEATURE_RWCOMPAT_DELETE	0x0000

#define NODE_DIRENT		(FEATURE_INCOMPAT        | NODE_ACCURATE | 1)
#define NODE_INODE		(FEATURE_INCOMPAT        | NODE_ACCURATE | 2)
#define NODE_CLEAN		(FEATURE_RWCOMPAT_DELETE | NODE_ACCURATE | 3)
//#define NODE_CHECKPOINT	(FEATURE_RWCOMPAT_DELETE | NODE_ACCURATE | 3)
//#define NODE_OPTIONS		(FEATURE_RWCOMPAT_COPY	 | NODE_ACCURATE | 4)
//#define NODETYPE_DIRENT_ECC	(FEATURE_INCOMPAT	 | NODE_ACCURATE | 5)
//#define NODETYPE_INODE_ECC	(FEATURE_INCOMPAT	 | NODE_ACCURATE | 6)

#define INODE_FLAG_PREREAD	1 /* Read at mount-time */
#define INODE_FLAG_USERCOMPR	2 /* User selected compression */

enum {
  DT_UNKNOWN = 0,
  DT_FIFO    = 1,
  DT_CHR     = 2,
  DT_DIR     = 4,
  DT_BLK     = 6,
  DT_REG     = 8,
  DT_LNK     = 10,
  DT_SOCK    = 12,
  DT_WHT     = 14,
};

struct unknown_node
{
  u16 marker;
  u16 node_type;
  u32 length;
  u32 header_crc;
} __attribute__((packed));

struct dirent_node
{
  u16 marker;
  u16 node_type;		/* NODETYPE_DIRENT */
  u32 length;
  u32 header_crc;
  u32 pino;
  u32 version;
  u32 ino;
  u32 mctime;
  u8 nsize;
  u8 type;
  u8 unused[2];
  u32 node_crc;
  u32 name_crc;
  u8 name[0];
} __attribute__((packed));

struct inode_node
{
  u16 marker;
  u16 node_type;		/* NODETYPE_INODE */
  u32 length;
  u32 header_crc;
  u32 ino;
  u32 version;    /* Version number.  */
  u32 mode;       /* The file's type or mode.  */
  u16 uid;        /* The file's owner.  */
  u16 gid;        /* The file's group.  */
  u32 isize;      /* Total resultant size of this inode (used for truncations)  */
  u32 atime;      /* Last access time.  */
  u32 mtime;      /* Last modification time.  */
  u32 ctime;      /* Change time.  */
  u32 offset;     /* Where to begin to write.  */
  u32 csize;      /* (Compressed) data size */
  u32 dsize;	  /* Size of the node's data. (after decompression) */
  u8 compr;       /* Compression algorithm used */
  u8 usercompr;	  /* Compression algorithm requested by the user */
  u16 flags;	  /* See JFFS2_INO_FLAG_* */
  u32 data_crc;   /* CRC for the (compressed) data.  */
  u32 node_crc;   /* CRC for the raw inode (excluding data)  */
  //  u8 data[dsize];
} __attribute__((packed));

union node {
	struct inode_node i;
	struct dirent_node d;
	struct unknown_node u;
} __attribute__((packed));

struct dirent_cache {
  u32 ino;
  u32 pino;
  u32 version;
  u32 index;			/* Offset to the struct dirent_node */
};

struct inode_cache {
  u32 ino;
  u32 version;
  u32 offset;
  u32 dsize;
  u32 index;			/* Offset to the struct inode_node */
};

#define DIRENT_CACHE_MAX	8192
#define INODE_CACHE_MAX		8192

struct jffs2_info {
  struct descriptor_d d;	/* Descriptor for underlying driver */

  int fCached;			/* Set after the caches are loaded */
  //  struct partition partition[4];
  //  struct superblock superblock;
  //  int block_size;
  //  int rg_blocking[3];		/* Tier block counts */

  //  struct inode inode;		/* Current inode */
  //  int inode_number;		/* Number of current inode */
  //  int blockCache;		/* Number of first block in cache */
  //  int cCache;			/* Count of cached block numbers */
  //  char rgbCache[BLOCK_SIZE_MAX]; /* Cache of block numbers from inode */

};

static struct jffs2_info jffs2;

static struct dirent_cache __attribute__((section(".jffs2.bss")))
     dirent_cache[DIRENT_CACHE_MAX];
static struct inode_cache __attribute__((section(".jffs2.bss")))
     inode_cache[INODE_CACHE_MAX];

static const char szBlockDriver[]  /* Underlying block driver */
  = CONFIG_DRIVER_JFFS2_BLOCKDEVICE;


extern unsigned long compute_crc32 (unsigned long crc, const void *pv, int cb);

static int verify_header_crc (struct unknown_node* node)
{
  return node->header_crc == ~compute_crc32 (~0, node, sizeof (*node) - 4);
}

static int verify_dirent_crc (struct dirent_node* node)
{
  return node->node_crc == ~compute_crc32 (~0, node, sizeof (*node) - 8);
}

int compare_dirent_cache (const void* _a, const void* _b)
{
  struct dirent_cache& a = *(struct dirent_cache*) _a;
  struct dirent_cache& b = *(struct dirent_cache*) _b;

  if (a.pino == b.pino) {
    if (a.ino == b.ino)
      return b.version - a.version;
    return a.ino - b.ino;
  }
  return a.pino - b.pino;
}

int compare_inode_cache (const void* _a, const void* _b)
{
  struct inode_cache& a = *(struct inode_cache*) _a;
  struct inode_cache& b = *(struct inode_cache*) _b;

  if (a.ino == b.ino) {
    if (a.offset == b.offset)
      return b.version - a.version;
    return a.offset - b.offset;
  }
  return a.ino - b.ino;
}

/* jffs2_load_cache

   reads the whole filesystem, caching the directory entries and the
   inodes.  It captures all of the active entries and then sorts them
   for the sake of reading files from the filesystem.

   Unlike the kernel, we don't have mmap'd access to the flash.  This
   means we cannot save pointers into flash.  Instead, we save
   offsets.

*/

void jffs2_load_cache (void)
{
  size_t ib;
  size_t cbNode;
  union node node;

  for (ib = 0; ib < jffs2.d.length; ib += cbNode) {

    jffs2.d.driver->seek (&jffs2.d, ib, SEEK_SET);
    jffs2.d.driver->read (&jffs2.d, &node, sizeof (node));

    if (node.u.marker != MARKER_JFFS2 
	|| !verify_header_crc (&node.u)) {
      if (node.u.marker == MARKER_JFFS2_REV)
	return NULL;		/* endian mismatch */
      cbNode = 4;
      continue;
    }

    cbNode = (current->u.length + 3) & ~3;

    if (current->d.node_type == NODE_DIRENT 
	&& !verify_dirent_crc (&current->d))
      continue;

    switch (

	/* Match */
      version = current->d.version;
      node = current;
    }



}

/* jffs2_find_file

   finds a file given the parent inode number and the path.

*/

static union node *jffs2_path_to_inode (int inode, struct descriptor_d* d,
					 union node* node_result)
{
  int i = d->iRoot;
  unsigned long version = 0;
  union node nodes[2];
  union node* current = &nodes[0];
  union node* node = NULL;
  size_t cbNode;

  ENTRY (0);
  
  if (!inode)
    inode = JFFS2_ROOT_INO;

  for (; i < d->c; ++i) {
    int length = strlen (d->pb[i]);
    int dotdot = length == 2 && strcmp (d->pb[i], "..") == 0;
    size_t ib;

	/* Make sure we have an empty buffer */
    if (node && current == node)
      current = (node == &nodes[0] ? &node[1] : &node[0]);

	/* Search entire filesystem for the node */
    for (ib = 0; ib < jffs2.d.length; ib += cbNode) {

      jffs2.d.driver->seek (&jffs2.d, ib, SEEK_SET);
      jffs2.d.driver->read (&jffs2.d, current, sizeof (&current));

      if (current->u.marker != MARKER_JFFS2 
	  || !verify_header_crc (&current->u)) {
	if (current->u.marker == MARKER_JFFS2_REV)
	  return NULL;		/* endian mismatch */
	cbNode = 4;
	continue;
      }

      cbNode = (current->u.length + 3) & ~3;

      if (current->d.node_type != NODE_DIRENT
	  || !verify_dirent_crc (&current->d))
	continue;

      if (current->d.version <= version)
	continue;

      if (dotdot) {
	if (   current->d.ino   != inode)
	  continue;
      }
      else {
	if (   current->d.pino  != inode
	    || current->d.nsize != length
	    || strncmp (jffs2.d.pb[i], current->d.name, length))
	  continue;
      }
	/* Match */
      version = current->d.version;
      node = current;
    }

    if (!node) 
      return NULL;		/* path not found */

//    if (node->type == DT_LNK)
//      if (!(node = do_symlink(part, node))) 
//	return NULL;

    inode = dotdot ? node->d.pino : node->d.ino;
  }

  *node_result = *node;
  return node_result;
}

/* decompress a page from the flash, first contains a linked list of references
 * all the nodes of this file, sorted is decending order (newest first). Return
 * the number of total bytes decompressed (not accurate, because some strips
 * overlap, but at least we know we decompressed something) */

static long jffs2_decompress_page (void* pv,
				   struct data_strip *first, u32 page)
{
	int i;
	long uncompressed = 0;
	char *src = 0;

	/* look for the next reference to this page */
	for (; first; first = first->next) {
		/* This might be a page to uncompress, check that its the right
		 * page, and that the crc matches */
		if (first->page == page) {
			src = ((char *) first->inode) +
			       sizeof(struct jffs2_raw_inode);
			if (first->inode->data_crc ==
			    crc32(0, src, first->inode->csize)) break;
		}
	}
	
	/* reached the end of the list without finding any pages */		
	if (!first) return 0;
	
	/* if we aren't covering what's behind us, uncompress that first.
	 * Note: if the resulting doesn't quite fill the order, we are in
	 * trouble */
	if (first->length != 4096)
		uncompressed = jffs2_uncompress_page(dest, first->next, page);

	dest += first->inode->offset;
	switch (first->inode->compr) {
	case JFFS2_COMPR_NONE:
		memcpy(dest, src, first->length);
		break;
	case JFFS2_COMPR_ZERO:
		for (i = 0; i < first->length; i++) *(dest++) = 0;
		break;
	case JFFS2_COMPR_RTIME:
		rtime_decompress(src, dest, first->inode->csize, 
				 first->length);
		break;
	case JFFS2_COMPR_DYNRUBIN:
		dynrubin_decompress(src, dest, first->inode->csize, 
				    first->length);
		break;
	case JFFS2_COMPR_ZLIB:
		zlib_decompress(src, dest, first->inode->csize, first->length);
		break;
	default:
		/* unknown, attempt to ignore the page */
		return uncompressed;
	}
	return first->length + uncompressed;
}



static int jffs2_open (struct descriptor_d* d)
{
  return 0;
}

static void jffs2_close (struct descriptor_d* d)
{
  ENTRY (0);

  close_descriptor (&jffs2.d);
  close_helper (d);
}

static ssize_t jffs2_read (struct descriptor_d* d, void* pv, size_t cb)
{
  return 0;
}

static void jffs2_report (void)
{
}

static __driver_6 struct driver_d jffs2_driver = {
  .name = DRIVER_NAME,
  .description = "JFFS2 filesystem driver",
  .flags = DRIVER_DESCRIP_FS,
  .open = jffs2_open,
  .close = jffs2_close,
  .read = jffs2_read,
//  .write = cf_write,
//  .erase = cf_erase,
  .seek = seek_helper,
};

static __service_6 struct service_d jffs2_service = {
//  .init = jffs2_init,
#if !defined (CONFIG_SMALL)
  .report = jffs2_report,
#endif
};
