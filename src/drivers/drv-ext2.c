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
   
   o Thanks to John,
     http://uranus.it.swin.edu.au/~jn/explore2fs/es2fs.htm, for
     compiling much of the information used to write this driver.

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

#define MAGIC_EXT2	0xef53

typedef __signed__ char  __s8;
typedef unsigned   char  __u8;
typedef __signed__ short __s16;
typedef unsigned   short __u16;
typedef __signed__ int	 __s32;
typedef unsigned   int	 __u32;

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
  __u16 bg_used_dirs_count;	/* Group's free directory inode total */
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
  __u32 i_blocks;		/* Blocks count */
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
  EXT2_BAD_INO		= 1,	/* Bad blocks inode */
  EXT2_ROOT_INO		= 2,	/* Root inode */
  EXT2_ACL_IDX_INO 	= 3,	/* ACL inode */
  EXT2_ACL_DATA_INO	= 4, 	/* ACL inode */
  EXT2_BOOT_LOADER_INO	= 5, 	/* Boot loader inode */
  EXT2_UNDEL_DIR_INO    = 6, 	/* Undelete directory inode */
  EXT2_FIRST_INO	= 11,	/* First non reserved inode */
};

#define EXT2_FILENAME_LENGTH_MAX 255

struct directory {
  __u32 inode;
  __u16 rec_len;
  __u8  name_len;
  __u8  file_type;
  char name[EXT2_FILENAME_LENGTH_MAX];
};

