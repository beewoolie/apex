/* drv-ext2.c
     $Id$

   written by Marc Singer
   22 Feb 2005

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
   
   NOTES
   -----

   o Thanks to John,
     http://uranus.it.swin.edu.au/~jn/explore2fs/es2fs.htm, for
     compiling some of the information used to write this driver.
     Unfortunately, some of his work is incorrect. 
   o Thanks to the GRUB project for implementing an ext2fs reader.

   o Partitions.  The partition support is redundant here.  We may
     want to implement a translation driver to handle the partition
     stuff.  Later.

   o There is no inode 0, so no room is left for it in the on-disk
     data structures.  
   o The filesystem image starts with the superblock and is
     immediately followed by an array of block_group structures.
     These data are replicated in each block group.
   o The first superblock begins in the first sector, or in the third,
     depending on how the filesystem was initialized.  This skipped
     1KiB only occurs for the first superblock and is never accounted
     for any place else.
   o Filesystem blocks are counted from zero starting at the beginning
     of the filesystem partition.
   o The bitmaps for the inodes and the used blocks are not
     replicated, but are specific to each block group. 
   o Thus, a file read is <DIR_MODE>inode = root; block_group; inode;
     <FILE_MODE> data_block[n]; search for path element; resolve; inode =
     new_inode; repeat; <DIR_MODE> if more path elements, <DATA_MODE>
     if file found. 

   o Need to make sure that the constant BLOCK_SIZE_MAX isn't smaller
     than the block size on disk.

   o Endianness.  We are careless about endianness.  We should make
     sure to use the appropriate macros before release.

   o Verified direct, indirect, and double-indirect inode blocks

   o How do we handle files larger than 2^32?
   o What do we know that the directory contains file type info? Is
     this standard in all ext2 implementations?

   o A path starting with ?i is interpreted as a direct request for an
     inode.  The number following the ?i prefix is parsed as a decimal
     number and is used as the inode to open.  e.g. ext2://2/?i

   o Symlinks
     o Are chased with recursive calls.  This means that the
       code could fail with a very deep linking path.  Be warned. 
     o Very long links are not properly ready.  We only read links
       that fit in the inode.  Links that require a block read are not
       followed.  The main reason for this is that is must be tested
       and I don't have a > 60 (15*4) character link to test.  

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
//#define TALK_DIR

#if defined TALK
# define PRINTF(v...)	printf (v)
#else
# define PRINTF(v...)	do {} while (0)
#endif

#define ENTRY(l) PRINTF ("%s\n", __FUNCTION__)

#define DRIVER_NAME	"ext2fs"

#define MAGIC_EXT2	0xef53

#define SECTOR_SIZE	512
#define BLOCK_SIZE	1024	/* Of questionable value */

#define BLOCK_SIZE_MAX	4096	/* Largest block buffer we support */

#if 0
/* Defined in kernel headers */
typedef __signed__ char  __s8;
typedef unsigned   char  __u8;
typedef __signed__ short __s16;
typedef unsigned   short __u16;
typedef __signed__ int	 __s32;
typedef unsigned   int	 __u32;
#endif

struct partition {
  unsigned char boot;
  unsigned char begin_chs[3];
  unsigned char type;
  unsigned char end_chs[3];
  unsigned long start;
  unsigned long length;
};

struct superblock {
  __u32 s_inodes_count;		/* Total inodes */
  __u32 s_blocks_count;		/* Total blocks */
  __u32	s_r_blocks_count;	/* Count of reserved blocks */
  __u32 s_free_blocks_count;	/* Total free blocks */
  __u32 s_free_inodes_count;	/* Total of free inodes */
  __u32 s_first_data_block;	/* Index of first data block (0|1) */
  __u32 s_log_block_size;	/* 2^(10+n) count of bytes in a block */
  __s32 s_log_frag_size;	/* Size of the fragments ?? */
  __u32 s_blocks_per_group;	/* Number of blocks in a block group */
  __u32 s_frags_per_group;	/* Number of fragments in a block group */
  __u32 s_inodes_per_group;	/* Number of inodes inn a block group */
  __u32 s_mtime;		/* Last filesystem modification time */
  __u32 s_wtime;		/* Last filesystem write time */
  __u16 s_mnt_count;		/* Count of filesystem mounts */
  __s16 s_max_mnt_count;	/* Mount limit before forcing a check */
  __u16 s_magic;		/* EXT2 magic number (0xef53) */
  __u16 s_state;		/* Filesystem state */
  __u16 s_errors;		/* Error reporting mode */
  __u16 s_pad;			/* Padding */
  __u32 s_lastcheck;		/* Last filesystem check time */
  __u32 s_checkinterval;	/* Maximum interval between checks */
  __u32 s_creator_os;		/* OS that created filesystem */
  __u32 s_rev_level;		/* Filesystem revision number */
  __u16 s_def_resuid;		/* Default uid for reserved blocks */
  __u16 s_def_resgid;		/* Default gid for reserved blocks */
  __u32 s_reserved[235];	/* Padding */
};

struct block_group {
  __u32 bg_block_bitmap;	/* Group's block bitmap block address */
  __u32 bg_inode_bitmap;	/* Group's inode bitmap block address */
  __u32 bg_inode_table;		/* Group's inode table block address */
  __u16 bg_free_blocks_count;	/* Group's free block total */
  __u16 bg_free_inodes_count;	/* Group's free inode total */
  __u16 bg_used_dirs_count;	/* Group's used directory total */
  __u16 bg_pad;			/* Padding */
  __u32 bg_reserved[3];		/* Padding */
}; 

struct inode {
  __u16 i_mode;			/* File mode */
  __u16 i_uid;			/* Owner Uid */
  __u32 i_size;			/* Size in bytes */
  __u32 i_atime;		/* Access time */
  __u32 i_ctime;		/* Creation time */
  __u32 i_mtime;		/* Modification time */
  __u32 i_dtime;		/* Deletion Time */
  __u16 i_gid;			/* Group Id */
  __u16 i_links_count;		/* Links count */
  __u32 i_blocks;		/* Blocks count (512b) */
  __u32 i_flags;		/* File flags */
  union {
    struct {
      __u32 l_i_reserved1;
    } linux1;
    struct {
      __u32 h_i_translator;
    } hurd1;
    struct {
      __u32 m_i_reserved1;
    } masix1;
  } osd1;			/* OS dependent 1 */
  __u32 i_block[15];		/* Pointers to blocks */
  __u32 i_version;		/* File version (for NFS) */
  __u32 i_file_acl;		/* File ACL */
  __u32 i_dir_acl;		/* Directory ACL */
  __u32 i_faddr;		/* Fragment address */

  union {
    struct {
      __u8 l_i_frag;		/* Fragment number */
      __u8 l_i_fsize;		/* Fragment size */
      __u16 i_pad1;
      __u32 l_i_reserved2[2];
    } linux2;
    struct {
      __u8 h_i_frag;		/* Fragment number */
      __u8 h_i_fsize;		/* Fragment size */
      __u16 h_i_mode_high;
      __u16 h_i_uid_high;
      __u16 h_i_gid_high;
      __u32 h_i_author;
    } hurd2;
    struct {
      __u8 m_i_frag;		/* Fragment number */
      __u8 m_i_fsize;		/* Fragment size */
      __u16 m_pad1;
      __u32 m_i_reserved2[2];
    } masix2;
  } osd2;			/* OS dependent 2 */
  //  __u8   i_frag;		/* Fragment number */
  //  __u8   i_fsize;		/* Fragment size */
  //  __u16 i_pad1;		/* Padding */
  //  __u32  i_reserved2[2];	/* Padding */
};

enum {
  EXT2_NULL_INO		= 0,	/* Invalid inode number */
  EXT2_BAD_INO		= 1,	/* Bad blocks inode */
  EXT2_ROOT_INO		= 2,	/* Root inode */
  EXT2_ACL_IDX_INO 	= 3,	/* ACL inode */
  EXT2_ACL_DATA_INO	= 4, 	/* ACL inode */
  EXT2_BOOT_LOADER_INO	= 5, 	/* Boot loader inode */
  EXT2_UNDEL_DIR_INO    = 6, 	/* Undelete directory inode */
  EXT2_FIRST_INO	= 11,	/* First non reserved inode */
};

enum {
  EXT2_FT_UNKNOWN	= 0,
  EXT2_FT_REG_FILE	= 1,
  EXT2_FT_DIR		= 2,
  EXT2_FT_CHRDEV	= 3,
  EXT2_FT_BLKDEV	= 4,
  EXT2_FT_FIFO		= 5,
  EXT2_FT_SOCK		= 6,
  EXT2_FT_SYMLINK	= 7,
  EXT2_FT_MAX
};

enum {
  S_IFMT		= 0170000,
  S_IFLNK		= 0120000,
  S_IFREG		= 0100000,
  S_IFDIR		= 0040000,
};

#define S_ISLNK(m)      (((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)

#define EXT2_FILENAME_LENGTH_MAX 255

struct directory {
  __u32 inode;			/* File inode */
  __u16 rec_len;		/* Size of whole directory record */
  __u8  name_len;		/* Size of filename */
  __u8  file_type;
  char name[EXT2_FILENAME_LENGTH_MAX];
};

struct ext2_info {
  struct descriptor_d d;	/* Descriptor for underlying driver */

  int fOK;
  struct partition partition[4];
  struct superblock superblock;
  int block_size;
  int rg_blocking[3];		/* Tier block counts */

  struct inode inode;		/* Current inode */
  int inode_number;		/* Number of current inode */
  int blockCache;		/* Number of first block in cache */
  int cCache;			/* Count of cached block numbers */
  char rgbCache[BLOCK_SIZE_MAX]; /* Cache of block numbers from inode */
};

static struct ext2_info ext2;
static const char szBlockDriver[] /* Underlying block driver */
  = CONFIG_DRIVER_EXT2_BLOCKDEVICE;

inline int block_groups (struct ext2_info* ext2)
{
  return (ext2->superblock.s_blocks_count
	  + ext2->superblock.s_blocks_per_group - 1)
    /ext2->superblock.s_blocks_per_group;
}

inline int group_from_inode (struct ext2_info* ext2, int inode)
{
  return (inode - 1)/ext2->superblock.s_inodes_per_group;
}


static inline unsigned long read_block_number (int i)
{
  char* pb = &ext2.rgbCache[i*sizeof (long)];
  return  ((unsigned long) pb[0])
       + (((unsigned long) pb[1]) <<  8)
       + (((unsigned long) pb[2]) << 16)
       + (((unsigned long) pb[3]) << 24);
}


static int ext2_block_read (int block, void* pv, size_t cb)
{
  PRINTF ("%s: %d\n", __FUNCTION__, block);
  ext2.d.driver->seek (&ext2.d, ext2.block_size*block, SEEK_SET);  
  return ext2.d.driver->read (&ext2.d, pv, cb) != cb;
}


/* ext2_update_block_cache

   makes sure that the block number cache contains the given block
   number.  This function is fast if the cache already contains the
   data.  It return 0 on success.  A non-zero result means that there
   was a read error.  Requesting a block beyond the extent of the
   inode is *not* detected.

   This is a core function of the driver.  It traverses the inode
   block list and retrieves up to a block's worth of block numbers for
   the file.

*/

static int ext2_update_block_cache (int block_index)
{
  int block_base = 0;

  if (block_index >= ext2.blockCache 
      && block_index < ext2.blockCache + ext2.cCache) {
//    PRINTF ("ubc: %d %d %d\n", block_index, ext2.blockCache, ext2.cCache);
    return 0;			/* Already cached */
  }

  ENTRY (0);

  if (block_index < ext2.rg_blocking[0]) {		/* Direct */
    PRINTF ("  direct\n");
    ext2.blockCache = 0;
    ext2.cCache = ext2.rg_blocking[0];
    memcpy (ext2.rgbCache, &ext2.inode.i_block[0],
	    ext2.rg_blocking[0]*sizeof (long));
    return 0;
  }

  block_base += ext2.rg_blocking[0];
  ext2.cCache = ext2.rg_blocking[1]; /* One block of block numbers */

  if (block_index < block_base + ext2.rg_blocking[1]) {	/* Indirect */
    PRINTF ("  indirect\n");
    if (ext2_block_read (ext2.inode.i_block[12], 
			 ext2.rgbCache, ext2.block_size))
      return -1;
    ext2.blockCache = block_base;
    PRINTF ("  @ %d\n", ext2.blockCache);
    return 0;
  }

  block_base += ext2.rg_blocking[1];

  if (block_index < block_base + ext2.rg_blocking[2]) {	/* Double indirect */
    int offset = (block_index - block_base)/ext2.rg_blocking[1];
    PRINTF ("  offset %d\n", offset);

    if (ext2_block_read (ext2.inode.i_block[13], 
			 ext2.rgbCache, ext2.block_size))
      return -1;
    if (ext2_block_read (read_block_number (offset),
			 ext2.rgbCache, ext2.block_size))
      return -1;

    ext2.blockCache = block_base + offset*ext2.rg_blocking[1];

    PRINTF ("  double indirect %d\n", ext2.blockCache);

    return 0;
  }

  block_base += ext2.rg_blocking[2];
							/* Triple indirect */
  {
    int offset = (block_index - block_base)/ext2.rg_blocking[2];
    PRINTF ("  offset %d\n", offset);

    if (ext2_block_read (ext2.inode.i_block[14], 
			 ext2.rgbCache, ext2.block_size))
      return -1;
    if (ext2_block_read (read_block_number (offset),
			 ext2.rgbCache, ext2.block_size))
      return -1;

    block_base += offset*ext2.rg_blocking[2];
    offset = (block_index - block_base)/ext2.rg_blocking[1];
    PRINTF ("  block_base %d  offset %d\n", block_base, offset);

    if (ext2_block_read (read_block_number (offset),
			 ext2.rgbCache, ext2.block_size))
      return -1;

    ext2.blockCache = block_base + offset*ext2.rg_blocking[1];

    PRINTF ("  triple indirect %d\n", ext2.blockCache);

    return 0;
  }
}

/* ext2_find_inode

   reads an inode into the current inode structure.  The return value
   is zero on success, non-zero on error. 

*/

int ext2_find_inode (int inode)
{
  struct block_group group;

  if (inode == ext2.inode_number)	/* Short circuit */
    return 0;

  memset (&ext2.inode, 0, sizeof (ext2.inode));
  ext2.cCache = 0;			/* Flush block cache */
  ext2.inode_number = 0;

	/* Fetch block_group structure for the inode  */
  ext2.d.driver->seek (&ext2.d, 
		       2*BLOCK_SIZE
		       + (sizeof (struct block_group)
			  *((inode - 1)/ext2.superblock.s_inodes_per_group)), 
		       SEEK_SET);
  if (ext2.d.driver->read (&ext2.d, &group, sizeof (group))
      != sizeof (struct block_group))
    return 1;

	/* Fetch the inode  */
  ext2.d.driver->seek (&ext2.d, 
		       ext2.block_size*group.bg_inode_table
		       + (sizeof (struct inode)
			  *((inode - 1)%ext2.superblock.s_inodes_per_group)),
		       SEEK_SET);  
//  PRINTF ("%s: seeked to 0x%x\n", __FUNCTION__, ext2.d.index);
  if (ext2.d.driver->read (&ext2.d, &ext2.inode, sizeof (struct inode))
      != sizeof (struct inode))
    return 1;

  ext2.inode_number = inode;

  PRINTF ("inode %d: mode %07o  flags %x  size %d (0x%x)\n", 
	  ext2.inode_number,
	  ext2.inode.i_mode, ext2.inode.i_flags, 
	  ext2.inode.i_size, ext2.inode.i_size); 

  return 0;
}


/* ext2_identify

   performs an initial read on the device to get partition
   information.

*/

static int ext2_identify (void)
{
  int result;
  char sz[128];
  struct descriptor_d d;

  snprintf (sz, sizeof (sz), "%s:+1s", szBlockDriver);
  if (   (result = parse_descriptor (sz, &d))
      || (result = open_descriptor (&d))) 
    return result;

	/* Check for signature */
  {
    unsigned char rgb[2];

    d.driver->seek (&d, SECTOR_SIZE - 2, SEEK_SET);
    if (d.driver->read (&d, &rgb, 2) != 2
	|| rgb[0] != 0x55
	|| rgb[1] != 0xaa) {
      PRINTF ("sig is %x %x\n", rgb[0], rgb[1]);
      close_descriptor (&d);
      return -1;
    }
  }

  d.driver->seek (&d, SECTOR_SIZE - 66, SEEK_SET);
  d.driver->read (&d, ext2.partition, 16*4);

  close_descriptor (&d);

  return 0;
}

/* ext2_enum_directory

   enumerates the entries in a directory, returning the next directory
   structure for each successive call.  The return value is NULL at
   the end of the list.

   The directory recursed should be read into the current inode.  If
   not, this call will load the root directory inode and enumerate
   from there.

   Note that the handle is only 0 at the start of the enumeration.
   Moreover, the returned value points to the next entry that will be
   enumerated.  If the last entry in a block has a reclen that puts
   the next record at the beginning of the next block, the initial
   check for a null inode will really be looking at the first entry of
   the block.  Fortunately, this is innocuous.  It may be better to
   perform that fixup just before returning.

*/

static void* ext2_enum_directory (void* h, struct directory** pdir)
{
  static int block_number;
  static char rgb[BLOCK_SIZE_MAX];
  size_t ib = (size_t) h;
  int block_index;
  struct directory* dir;

//  ENTRY (0);

  if (ib == 0 && !ext2.inode_number && ext2_find_inode (EXT2_ROOT_INO))
    return NULL;

  if (!S_ISDIR (ext2.inode.i_mode))
    return NULL;

  if (ib >= ext2.inode.i_size)
    return NULL;
  dir = (struct directory*) ((void*) rgb + (ib & (ext2.block_size - 1)));
  if (ib && dir->inode == 0)	/* This may never be invoked */
    ib = (ib & ~(ext2.block_size - 1)) + ext2.block_size;
  if (ib >= ext2.inode.i_size)
    return NULL;

  block_index = ib/ext2.block_size;
  ext2_update_block_cache (block_index);
  if (read_block_number (block_index - ext2.blockCache) != block_number) {
    block_number = read_block_number (block_index - ext2.blockCache);
    if (ext2_block_read (block_number, rgb, ext2.block_size))
      return NULL;
  }
  dir = (struct directory*) ((void*) rgb + (ib & (ext2.block_size - 1)));
  *pdir = dir;
  return (void*) ib + dir->rec_len;
}


/* ext2_path_to_inode

   follows a path, opening inodes along the way, and returns the inode
   of the directory with the final inode of the last path element.
   The passed inode is the starting node for the search.

   This might seem like a peculiar way to traverse a path.  It is done
   this way to simplify symlink traversal.  

   This is a core function of the driver.  It is what enabled the use
   of pathnames to access files in the filesystem.  

*/

static int ext2_path_to_inode (int inode, struct descriptor_d* d)
{
  int i = d->iRoot;
  void* h;
  struct directory* dir = NULL;

  ENTRY (0);

  if (!inode)
    inode = EXT2_ROOT_INO;

  PRINTF ("%s: start, reading inode %d\n", __FUNCTION__, inode);

  if (ext2_find_inode (inode))
    return EXT2_NULL_INO;

  PRINTF ("%s: searching\n", __FUNCTION__);

  for (; i < d->c; ++i) {
    int length = strlen (d->pb[i]);
    h = NULL;
    PRINTF ("%s: enumerating on inode %d\n", __FUNCTION__, ext2.inode_number);
    inode = ext2.inode_number;
    while ((h = ext2_enum_directory (h, &dir))) {
      PRINTF ("  '%s' '%*.*s'\n", d->pb[i], 
	      dir->name_len, dir->name_len, dir->name);
      if (length != dir->name_len)
	continue;
      if (memcmp (d->pb[i], dir->name, length))
	continue;

      if (   dir->file_type != EXT2_FT_DIR
	  && dir->file_type != EXT2_FT_SYMLINK
	  && dir->file_type != EXT2_FT_REG_FILE)
	return EXT2_NULL_INO;		/* Limited inode handling  */

      if (ext2_find_inode (dir->inode))
	return EXT2_NULL_INO;

		/* Recurse into directory */
      if (S_ISDIR (ext2.inode.i_mode)) {
	PRINTF ("%s: following directory %d\n",
		__FUNCTION__, ext2.inode_number);
	break;
      }

		/* Chase symlink */
      if (S_ISLNK (ext2.inode.i_mode)) {
	PRINTF ("%s: chasing symlink %d\n", __FUNCTION__, ext2.inode_number);
	while (1) {
	  int cb = ext2.inode.i_size;
	  int cbDriver;
	  char sz[3 + cb];
	  struct descriptor_d d2;
		 /* Kindofa dumb hack to coerce the descriptor parser
		    into parsing the symlink path */
	  strcpy (sz, DRIVER_NAME);
	  cbDriver = strlen (sz); sz[cbDriver++] = ':';
	  memcpy (sz + cbDriver, (void*) &ext2.inode.i_block[0], cb);
	  sz[cbDriver + cb] = 0;
	  PRINTF ("%s: chasing '%s'\n", __FUNCTION__, sz);
	  if (parse_descriptor (sz, &d2))
	    return EXT2_NULL_INO;
	  inode = ext2_path_to_inode (inode, &d2);
	  if (S_ISLNK (ext2.inode.i_mode))
	    continue;		/* chase again */
	  PRINTF ("%s: end of symlink %d\n", __FUNCTION__, ext2.inode_number);
	  break;
	}
	break;
      }

		/* Detect filename within path */
      if (i + 1 < d->c)
	return EXT2_NULL_INO;

      break;
    }
    if (h == NULL)		/* file not found */
      return EXT2_NULL_INO;
  }

  PRINTF ("%s: returning inode %d finding %d\n",
	  __FUNCTION__, inode, ext2.inode_number); 

  return inode;
}

static int ext2_open (struct descriptor_d* d)
{
  int result = 0;
  char sz[128];

  ENTRY (0);

  ext2.fOK = 0;

  if ((result = ext2_identify ()))
    return result;
  ext2.fOK = 1;

#if defined (TALK)
  PRINTF ("descript %d %d\n", d->c, d->iRoot);
  {
    int i;
    for (i = 0; i < d->c; ++i)
      PRINTF ("  %d: (%s)\n", i, d->pb[i]);
  }
#endif

  {
    int partition = 0;
    if (d->iRoot > 0 && d->c)
      partition = simple_strtoul (d->pb[0], NULL, 10) - 1;
    if (partition < 0 || partition > 3 
		      || ext2.partition[partition].length == 0)
      ERROR_RETURN (ERROR_BADPARTITION, "invalid partition"); 

    snprintf (sz, sizeof (sz), "%s:%lds+%lds", 
	      szBlockDriver, 
	      ext2.partition[partition].start, 
	      ext2.partition[partition].length);
  }

  PRINTF ("  opening %s\n", sz);
#if defined (TALK)
  PRINTF ("descript %d %d\n", d->c, d->iRoot);
  {
    int i;
    for (i = 0; i < d->c; ++i)
      PRINTF ("  %d: (%s)\n", i, d->pb[i]);
  }
#endif

	/* Open descriptor for the partition */
  if (   (result = parse_descriptor (sz, &ext2.d))
      || (result = open_descriptor (&ext2.d))) 
    return result;

	/* Default for superblock is 1 1KiB block into the partition */
  ext2.d.driver->seek (&ext2.d, BLOCK_SIZE, SEEK_SET);
  if (ext2.d.driver->read (&ext2.d, &ext2.superblock, 
			   sizeof (ext2.superblock)) 
      != sizeof (ext2.superblock)
      || ext2.superblock.s_magic != MAGIC_EXT2) {
    close_descriptor (&ext2.d); 
    return -1;
  }
  
	/* Precompute constants based on block size */
  ext2.block_size = 1 << (ext2.superblock.s_log_block_size + 10);
  ext2.rg_blocking[0] = 12;
  ext2.rg_blocking[1] = ext2.block_size/sizeof (long);
  ext2.rg_blocking[2] = ext2.rg_blocking[1]*(ext2.block_size/sizeof (long));

#if 0
	/* Read block group control structures */
  ext2.d.driver->seek (&ext2.d, 2*BLOCK_SIZE, SEEK_SET);
  if (ext2.d.driver->read (&ext2.d, &ext2.block_group, 
			   sizeof (ext2.block_group)) 
      != sizeof (ext2.block_group)) {
    close_descriptor (&ext2.d); 
    return -1;
  }    
#endif

	/* Parse an inode number */
  if (*d->pb[d->iRoot] == '?' && (d->pb[d->iRoot])[1] == 'i') {
    int inode_target = simple_strtoul (d->pb[d->iRoot] + 2, NULL, 10);
    if (ext2_find_inode (inode_target)) {
      close_descriptor (&ext2.d); 
      return ERROR_FILENOTFOUND;
    }
  }
  else {
    if (ext2_path_to_inode (0, d) == EXT2_NULL_INO) {
      close_descriptor (&ext2.d);
      return ERROR_FILENOTFOUND;
    }
  }

  if (d->length == 0)		/* Defaulting to length of whole file */
    d->length = ext2.inode.i_size;

  if (d->start > ext2.inode.i_size)
    d->start = ext2.inode.i_size;
  if (d->start + d->length > ext2.inode.i_size)
    d->length = ext2.inode.i_size - d->start;

#if defined (TALK_DIR)
  /* Dump it as a dir if it is one */
  {
    void* h = NULL;
    struct directory* dir;
    while ((h = ext2_enum_directory (h, &dir))) {
      printf ("%5d: 0x%x %*.*s\n", 
	      dir->inode, dir->file_type, 
	      dir->name_len, dir->name_len, dir->name);
    }
  }      
#endif

#if 0
  dump ((void*) &ext2.inode, 0x80, 0);

  PRINTF ("inode: size %d  blocks %d\n", 
	  ext2.inode.i_size, ext2.inode.i_blocks);
  PRINTF ("  [%d %d %d %d %d %d %d %d]\n", 
	  ext2.inode.i_block[0], ext2.inode.i_block[1],
	  ext2.inode.i_block[2], ext2.inode.i_block[3],
	  ext2.inode.i_block[4], ext2.inode.i_block[5],
	  ext2.inode.i_block[6], ext2.inode.i_block[7]);
#endif

  return 0;
}

static void ext2_close (struct descriptor_d* d)
{
  ENTRY (0);

  close_descriptor (&ext2.d);
  close_helper (d);
}

static ssize_t ext2_read (struct descriptor_d* d, void* pv, size_t cb)
{
  ssize_t cbRead = 0;

  //  ENTRY (0);
  PRINTF ("%s: inode %d\n", __FUNCTION__, ext2.inode_number);

  while (cb) {
    size_t available = cb;
    size_t index = d->start + d->index;
    size_t offset = (index & (ext2.block_size - 1));
    size_t remain = ext2.block_size - offset;
    int block_index;
    int block;

    /* *** FIXME: this code doesn't properly bound the read on the
       extent of the descriptor. */

    if (index >= ext2.inode.i_size)
      break;
    if (index + available > ext2.inode.i_size)
      available = ext2.inode.i_size - index;
    if (available > remain)
      available = remain;

    block_index = index/ext2.block_size;
//    PRINTF ("%s: index %d  block_index %d  offset %d  available %d\n",
//	    __FUNCTION__, index, block_index, offset, available);
    if (ext2_update_block_cache (block_index))
      break;

    block = read_block_number (block_index - ext2.blockCache);
    ext2.d.driver->seek (&ext2.d, ext2.block_size*block + offset, SEEK_SET);  

    {
      ssize_t cbThis = ext2.d.driver->read (&ext2.d, pv, available);
      if (cbThis <= 0)
	break;
      d->index += cbThis;
      cb -= cbThis;
      cbRead += cbThis;
      pv += cbThis;
    }
  }

  return cbRead;
}

#if !defined (CONFIG_SMALL)
static void ext2_report (void)
{
  int i;

  if (!ext2.fOK) {
    if (ext2_identify ())
      return;
    ext2.fOK = 1;
  }

  printf ("  ext2:\n"); 
  for (i = 0; i < 4; ++i)
    if (ext2.partition[i].type)
      printf ("          partition %d: %c %02x 0x%08lx 0x%08lx\n", i, 
	      ext2.partition[i].boot ? '*' : ' ',
	      ext2.partition[i].type,
	      ext2.partition[i].start,
	      ext2.partition[i].length);

  if (ext2.block_size) {
    printf ("          total (i/b) %d/%d  free %d/%d  group %d/%d\n", 
	    ext2.superblock.s_inodes_count,
	    ext2.superblock.s_blocks_count,
	    ext2.superblock.s_free_inodes_count,
	    ext2.superblock.s_free_blocks_count,
	    ext2.superblock.s_inodes_per_group,
	    ext2.superblock.s_blocks_per_group
	    );
    printf ("          first_data %d  block_size %d  groups %d\n", 
	    ext2.superblock.s_first_data_block, 
	    ext2.block_size,
	    block_groups (&ext2));
#if 0
    printf ("          group 0: inode block %d  free %d/%d\n", 
	    ext2.block_group[0].bg_inode_table,
	    ext2.block_group[0].bg_free_inodes_count,
	    ext2.block_group[0].bg_free_blocks_count);
#endif
  }
}
#endif

static __driver_6 struct driver_d ext2_driver = {
  .name = DRIVER_NAME,
  .description = "Ext2 filesystem driver",
  .flags = DRIVER_DESCRIP_FS,
  .open = ext2_open,
  .close = ext2_close,
  .read = ext2_read,
//  .write = cf_write,
//  .erase = cf_erase,
  .seek = seek_helper,
};

static __service_6 struct service_d ext2_service = {
#if !defined (CONFIG_SMALL)
  .report = ext2_report,
#endif
};
