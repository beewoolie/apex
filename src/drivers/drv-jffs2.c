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

   size_inode
   ----------

   It may be better to handle this by reading through the flash data.
   Why?  It means that the cache will be smaller.  Once we starting
   caching these elements, we might as well cache everything about the
   files.  We can sacrifice some ultimate speed for simple
   completeness.  In other words, we don't cache anything about the
   inode.  We scan all of the inode records when we want to get
   information about the file.  Ugh.

   reading
   -------

   It is possible that our algorithms are too simple for reading from
   a file.  We assume that the inodes are in reasonable shape.  The
   presence of an overlay inode may cause an invalid read.  It's
   really difficult to know.  The simplest way to handle this would be
   to perform a fixup on the inode cache that makes it clear where
   valid file data can be found.

   empty blocks
   ------------

   We're making an assumption about the structure of empty storage.
   JFFS2 always writes at least one node to a block if that block is
   in use.  There will never be gaps between nodes of more than a
   single byte.  Flash contains the byte value 0xff when it is erased.
   This all translates into the assumption than a run of 0xff's of
   some length indicates that the rest of the block is unused.  We
   don't make a distinction between finding 0xff's at the beginning of
   a block or the end.  Moreover, we don't make an effort to determine
   the true eraseblock size.  This heuristic is very important when
   reducing the cache-load time for a filesystem with lots of empty
   space.

   crc's
   -----

   We're checking CRCs sparsely.  Should be done everywhere possible.
   Enough said.
   

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
#include <sort.h>
#include <linux/stat.h>
#include <zlib.h>
#include <zlib-heap.h>

#define TALK

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

#define BLOCK_SIZE_MAX		(4*1024)
#define ERASEBLOCK_SIZE		(64*1024) 
#define EMPTY_THRESHOLD		8

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

#define DIRENT_CACHE_MAX	8192
#define INODE_CACHE_MAX		8192

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
  u32 isize;      /* Total size of this inode (used for truncations)  */
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
  u32 nsize;			/* Size of the name field */
  u8 type;			/* Cached entry type (DT_) */
};

struct inode_cache {
  u32 ino;
  u32 version;
  u32 offset;
  u32 csize;			/* Length of this node's data compressed */
  u32 dsize;			/* Length of this node's data uncompressed */
  u32 index;			/* Offset to the struct inode_node */
};

struct jffs2_info {
  struct descriptor_d d;	/* Descriptor for underlying driver */

  int fCached;			/* Set after the caches are loaded */

  u32 inode;			/* Open inode */
  size_t ibCache;		/* Offset of cached block */
  size_t cbCache;		/* Length of cached block */
  char rgbCache[BLOCK_SIZE_MAX];

};

static struct jffs2_info jffs2;

static struct dirent_cache __attribute__((section(".jffs2.bss")))
     dirent_cache[DIRENT_CACHE_MAX];
static struct inode_cache __attribute__((section(".jffs2.bss")))
     inode_cache[INODE_CACHE_MAX];
int cDirentCache;
int cInodeCache;

static const char szBlockDriver[]  /* Underlying block driver */
  = CONFIG_DRIVER_JFFS2_BLOCKDEVICE;


static inline void read_node (void* pv, size_t ib, size_t cb)
{
//  int cbRead;
  jffs2.d.driver->seek (&jffs2.d, ib, SEEK_SET);
  /*  cbRead = */ jffs2.d.driver->read (&jffs2.d, pv, cb);
//  PRINTF ("%s: %d %d -> %d\n", __FUNCTION__, ib, cb, cbRead);
}

extern unsigned long compute_crc32 (unsigned long crc, const void *pv, int cb);

static int verify_header_crc (struct unknown_node* node)
{
  return node->header_crc == ~compute_crc32 (~0, node, sizeof (*node) - 4);
}

static int verify_dirent_crc (struct dirent_node* node)
{
  return node->node_crc == ~compute_crc32 (~0, node, sizeof (*node) - 8);
}

const int find_cached_inode (u32 inode)
{
  int min = 0;
  int max = cInodeCache;

  ENTRY (0);

  while (min + 1 < max) {
    int mid = (min + max)/2;
    if (inode_cache[mid].ino == inode) {
      while (mid > 0 && inode_cache[mid - 1].ino == inode)
	--mid;
      return mid;
    }
    if (inode_cache[mid].ino < inode)
      min = mid;
    else
      max = mid;
  }

  return (inode_cache[min].ino == inode) ? min : -1;
}


/* find_cached_directory_node

   searches for an inode in the dirent cache.  This array is sorted by
   parent inodes, so this search has to be an O(n) scan of the
   array...for now.

*/

const int find_cached_directory_inode (u32 inode)
{
  int max = cDirentCache;
  int i;

  ENTRY (0);

  for (i = 0; i < max; ++i)
    if (dirent_cache[i].ino == inode)
      return i;
  return -1;
}

const int find_cached_parent_inode (u32 inode)
{
  int min = 0;
  int max = cDirentCache;

  ENTRY (0);

  while (min + 1 < max) {
    int mid = (min + max)/2;
    if (dirent_cache[mid].pino == inode) {
      while (mid > 0 && dirent_cache[mid - 1].pino == inode)
	--mid;
      min = mid;
      break;
    }
    if (dirent_cache[mid].pino < inode)
      min = mid;
    else
      max = mid;
  }

  return (dirent_cache[min].pino == inode) ? min : -1;
}

int compare_dirent_cache (const void* _a, const void* _b)
{
  struct dirent_cache* a = (struct dirent_cache*) _a;
  struct dirent_cache* b = (struct dirent_cache*) _b;

  if (a->pino == b->pino) {
    if (a->ino == b->ino)
      return b->version - a->version;
    return a->ino - b->ino;
  }
  return a->pino - b->pino;
}

int compare_inode_cache (const void* _a, const void* _b)
{
  struct inode_cache* a = (struct inode_cache*) _a;
  struct inode_cache* b = (struct inode_cache*) _b;

  if (a->ino == b->ino) {
    if (a->offset == b->offset)
      return b->version - a->version;
    return a->offset - b->offset;
  }
  return a->ino - b->ino;
}


/* summarize_inode

   returns the inode record that is most recent.  This is helpful for
   getting the most current meta-data and extent of the file.

*/

void summarize_inode (u32 inode, union node* node)
{
  int i = find_cached_inode (inode);
  u32 version = 0;
  
  for (; i < cInodeCache; ++i) {
    if (inode_cache[i].ino != inode)
      break;
    if (inode_cache[i].version < version)
      continue;
    version = inode_cache[i].version;
    PRINTF ("summarizing %d ver %d at %d(%x)\n", 
	    inode, version, inode_cache[i].index, inode_cache[i].index);
    read_node (node, inode_cache[i].index, sizeof (struct inode_node));
  }
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
  int cEmpties = 0;		/* Count of consecutive empties */

  ENTRY (0);

  printf ("caching jffs2 filesystem: "); 

  for (ib = 0; ib < jffs2.d.length; ib = (ib + cbNode + 3) & ~3) {

//    PRINTF ("reading node @0x%x\n", ib);

    jffs2.d.driver->seek (&jffs2.d, ib, SEEK_SET);
    jffs2.d.driver->read (&jffs2.d, &node, sizeof (node));

    if (node.u.marker != MARKER_JFFS2 
	|| !verify_header_crc (&node.u)) {
      cbNode = 4;
      if (node.u.marker == MARKER_JFFS2_REV)
	return;			/* endian mismatch */

		/* Check for empty block */
      if (*(unsigned long*) &node == ~0) {
	if (++cEmpties < EMPTY_THRESHOLD)
	  continue;
	cEmpties = 0;
	cbNode = ERASEBLOCK_SIZE - (ib % ERASEBLOCK_SIZE);
      }
      continue;
    }
    
    cEmpties = 0;

    cbNode = (node.u.length + 3) & ~3;

    switch (node.u.node_type) {
    case NODE_DIRENT:
      if (!verify_dirent_crc (&node.d))
	continue;

      dirent_cache[cDirentCache].ino = node.d.ino;
      dirent_cache[cDirentCache].pino = node.d.pino;
      dirent_cache[cDirentCache].version = node.d.version;
      dirent_cache[cDirentCache].index = ib;
      dirent_cache[cDirentCache].nsize = node.d.nsize;
      dirent_cache[cDirentCache].type = node.d.type;
      ++cDirentCache;
      break;

    case NODE_INODE:
      inode_cache[cInodeCache].ino = node.i.ino;
      inode_cache[cInodeCache].offset = node.i.offset;
      inode_cache[cInodeCache].csize = node.i.csize;
      inode_cache[cInodeCache].dsize = node.i.dsize;
      inode_cache[cInodeCache].version = node.i.version;
      inode_cache[cInodeCache].index = ib;
      ++cInodeCache;
      break;;

    default:
      break;
    }
  }

  sort (dirent_cache, cDirentCache, sizeof (struct dirent_cache), 
	compare_dirent_cache, NULL);
  sort (inode_cache,  cInodeCache,  sizeof (struct inode_cache), 
	compare_inode_cache, NULL);

  printf ("%d directory nodes %d inodes nodes\n",
	  cDirentCache, cInodeCache);
}


/* jffs2_find_file

   finds a file given the parent inode number and the path.

*/

static int jffs2_path_to_inode (int inode, struct descriptor_d* d)
{
  int i = d->iRoot;
  //  unsigned long version = 0;
  //  union node nodes[2];
  //  union node* current = &nodes[0];
  //  union node* node = NULL;
  //  int ino;

  ENTRY (0);
  
  if (!inode)
    inode = JFFS2_ROOT_INO;

  for (; i < d->c; ++i) {
    int length = strlen (d->pb[i]);
//    int dotdot = length == 2 && strcmp (d->pb[i], "..") == 0;
    //    size_t ib;
    int index;

    //    printf ("%s: %d %d\n", __FUNCTION__, i, d->c);

    index = find_cached_parent_inode (inode);
    if (index == -1)
      return 0;			/* Path not found */

    //    printf ("%s: index %d\n", __FUNCTION__, index);

    for (; index < cDirentCache; ++index) {
		/* Short-circuit for name length mismatch */
      char rgb[512];
      size_t cbNode;
      struct dirent_node* dirent = (struct dirent_node*) rgb;

      if (dirent_cache[index].nsize != length)
	continue;
      
      cbNode = sizeof (struct dirent_node) + dirent_cache[index].nsize;
      if (cbNode > sizeof (rgb))
	cbNode = sizeof (rgb);

      jffs2.d.driver->seek (&jffs2.d, dirent_cache[index].index,
			    SEEK_SET);
      jffs2.d.driver->read (&jffs2.d, dirent, cbNode);
		/* memcmp OK because we know the strings are the same length */
      if (memcmp (d->pb[i], (const char*) dirent->name, dirent->nsize))
	continue;

      inode = dirent->ino;
      break;
    }

//    inode = dotdot ? node->d.pino : node->d.ino;
  }

  return inode;
}


/* jffs2_identify

   loads the jffs2 cache and makes sure to do it only once.

*/

static int jffs2_identify (void)
{
  int result;
  struct descriptor_d d;

  ENTRY (0);

  if (jffs2.fCached)
    return 0;

  if (   (result = parse_descriptor (szBlockDriver, &d))
      || (result = open_descriptor (&d))) 
    return result;
  
  jffs2.d = d;
  jffs2_load_cache ();
  jffs2.fCached = 1;
  return 0;
}


/* jffs2_decompress_node

   copies the given node, references by inode_cache index, to the
   single node cache which is limited to 4KiB.  

   *** FIXME: we could do some optimizations for when a) the node is
   *** zero and b) when the node is uncompressed.  For the time being,
   *** we'll leave it at this.

*/

static int jffs2_decompress_node (int index)
{
  unsigned char rgb[BLOCK_SIZE_MAX + sizeof (struct inode_node)];
  struct inode_node* node = (struct inode_node*) rgb;
  size_t dsize;
  size_t csize;

  ENTRY (0);

  read_node (rgb, inode_cache[index].offset, 
	     sizeof (struct inode_node) + inode_cache[index].csize);
  dsize = node->dsize;
  if (dsize > BLOCK_SIZE_MAX)
    dsize = BLOCK_SIZE_MAX;
  csize = node->csize;
  if (csize > BLOCK_SIZE_MAX)
    csize = BLOCK_SIZE_MAX;

  PRINTF ("%s: %d of %d  cs %d  ds %d\n", __FUNCTION__, 
	  index, 
	  inode_cache[index].offset, 
	  inode_cache[index].csize, 
	  inode_cache[index].dsize);
  PRINTF ("%s: z %d  of %d  cs %d  ds %d\n", __FUNCTION__, 
	  node->compr, node->offset, node->csize, node->dsize);

  switch (node->compr) {

  case COMPRESSION_NONE:
    memcpy (jffs2.rgbCache, node + 1, dsize);
    jffs2.ibCache = node->offset;
    jffs2.cbCache = dsize;
    break;

  case COMPRESSION_ZERO:
    memset (jffs2.rgbCache, 0, dsize);
    jffs2.ibCache = node->offset;
    jffs2.cbCache = dsize;
    break;

  case COMPRESSION_ZLIB:
    {
      z_stream z;
      int result;
      memset (&z, 0, sizeof (z));
      z.zalloc = zlib_heap_alloc;
      z.zfree = zlib_heap_free;
      zlib_heap_reset ();
      if (inflateInit (&z) != Z_OK)
	return ERROR_FAILURE;
      z.next_in = (Bytef*) (node + 1);
      z.avail_in = csize;
      z.next_out = (Bytef*) jffs2.rgbCache;
      z.avail_out = BLOCK_SIZE_MAX;
      result = inflate (&z, 0);
      jffs2.ibCache = node->offset;
      jffs2.cbCache = (result == 0) ? dsize : 0;
      return (result == 0) ? 0 : ERROR_FAILURE;
    }
    break;

  case COMPRESSION_RTIME:
  case COMPRESSION_RUBINMIPS:
  case COMPRESSION_COPY:
  case COMPRESSION_DYNRUBIN:
  case COMPRESSION_LZO:
  case COMPRESSION_LZARI:
  default:
    jffs2.ibCache = 0;
    jffs2.cbCache = 0;
    return ERROR_UNSUPPORTED;
  }
}


/* decompress a page from the flash, first contains a linked list of references
 * all the nodes of this file, sorted is decending order (newest first). Return
 * the number of total bytes decompressed (not accurate, because some strips
 * overlap, but at least we know we decompressed something) */


#if 0
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
#endif


static int jffs2_open (struct descriptor_d* d)
{
  int result = 0;

  if ((result = jffs2_identify ()))
    return result;

  jffs2.inode = jffs2_path_to_inode (0, d);
  if (jffs2.inode <= 0)
    return ERROR_FILENOTFOUND;

  /* *** FIXME: Need to summarize so we have the correct file extent */

  return 0;
}

static void jffs2_close (struct descriptor_d* d)
{
  ENTRY (0);

//  close_descriptor (&jffs2.d);
  close_helper (d);
}

static ssize_t jffs2_read (struct descriptor_d* d, void* pv, size_t cb)
{
  ENTRY (0);

  /* Need to keep a couple of things in mind here.  If the request is
     within an existing cached block, satisfy it and return.  If the
     request is in another block, scan the inode data for the record
     and either copy it to the cache (if compressed) or read directly
     from source.  That should be it, actually.  */


  while (cb) {
    size_t index = d->start + d->index;
    int i;
    int state = 0;

	/* Read from the cache */
    if (index >= jffs2.ibCache && index < jffs2.ibCache + jffs2.cbCache) {
      size_t offset = index - jffs2.ibCache;
      size_t available = jffs2.cbCache - offset;
      if (available > cb)
	available = cb;
      memcpy (pv, &jffs2.rgbCache[offset], available);
      cb -= available;
      pv += available;
      state = 0;
      continue;
    } 

    for (i = find_cached_inode (jffs2.inode);
	 i < cInodeCache; ++i) {
      if (inode_cache[i].ino != jffs2.inode)
	return 0;		/* Past end of the file */
      if (index < inode_cache[i].offset)
	continue;

      jffs2_decompress_node (i); /* Decompress into the cache block */
      ++state;
      break;
    }

    if (state == 0)
      return ERROR_FAILURE;
  }

  return 0;
}


static void jffs2_info (struct descriptor_d* d)
{
  union node node;
  int inode;
  int i;

  if (jffs2_identify ())
    return;

  inode = jffs2_path_to_inode (0, d);
  if (inode <= 0) {
    printf ("path not found\n"); 
    return;
  }

  //  printf ("inode %d\n", inode);

  if (inode != 1) {

    summarize_inode (inode, &node);

    printf ("mode %08o  s %d\n", 
	    node.i.mode, node.i.isize);
    if (S_ISDIR (node.i.mode)) 
      printf ("    directory\n");
    if (S_ISREG (node.i.mode)) 
      printf ("    regular file\n");
    if (S_ISLNK (node.i.mode)) 
      printf ("    link\n");
    if (S_ISCHR (node.i.mode)) 
      printf ("    char dev\n");
    if (S_ISBLK (node.i.mode)) 
      printf ("    blk dev\n");
  }
  
  if (S_ISDIR (node.i.mode) || inode == 1) {
    i = find_cached_parent_inode (inode);
    for (; i != -1 && i < cDirentCache && dirent_cache[i].pino == inode; ++i) {
      char rgb[512];
      struct dirent_node* dirent = (struct dirent_node*) rgb;

      if (dirent_cache[i].ino == 0)
	continue;

      read_node (rgb, dirent_cache[i].index, 
		 sizeof (struct dirent_node) + dirent_cache[i].nsize);

      printf ("%*.*s", dirent->nsize, dirent->nsize, dirent->name );
      switch (dirent_cache[i].type) {
      case DT_DIR:
	printf ("/");
	break;
      case DT_LNK:
	printf ("@");
	break;
      case DT_REG:
	break;
      default:
	printf ("(%d)", dirent_cache[i].type);
	break;
      }
      printf ("\n");
    }
  }

  return;
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
  .info = jffs2_info,
};

static __service_6 struct service_d jffs2_service = {
//  .init = jffs2_init,
#if !defined (CONFIG_SMALL)
  .report = jffs2_report,
#endif
};
