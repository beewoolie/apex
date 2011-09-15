/** @file header.c

   written by Marc Singer
   1 Jul 2011

   Copyright (C) 2011 Marc Singer

   -----------
   DESCRIPTION
   -----------

   The DCD is necessary to coax the mx5x boot ROM to recognize the
   image and execute it.  (*** FIXME: is this really true?  Can we use
   vanilla images if we want to?)  The layout for the whole of Apex is
   as follows

     Loader Image
   +-----------------+
   | 1KiB Padding    |
   +-----------------+
   | DCD Header      |
   +-----------------+
   | DCD Array       |
   +-----------------+
   | DCD Trailer     |
   +-----------------+
   | Loader Segments |
   +-----------------+

   Most Loader Images place a jump at the start of the 1K padding that
   moves the PC to Loader entry point.  It doesn't appear to be
   necessary.

   The header structure is defined below to the best of our
   understanding.  It seems to be the case that the image_size is the
   size of the entire Loader Image that needs to be loaded into
   RAM, including the padding, and DCD data.

   The trailer has a single field, the size of the Loader Image.  The
   boot ROM copies this number of bytes from the boot source medium to
   the application base address defined in the DCD header.

   The padding may server to leave room for partition tables when the
   loader is being read from a medium that supports partitions.

   Only one type of DCD entry is being used, #4.  This appears to be a
   word write of value to the address in the DCD entry.

*/

#include <config.h>
#include <attributes.h>
//#include <asm/bootstrap.h>
#include <arch-arm.h>
#include <asm/cp15.h>
#include <mach/memory.h>
#include <sdramboot.h>

struct dcd_header {
  void (*application_entry) (void);		 /* Loader initial PC */
  u32                barker;                     /* Always 0xb1 */
  u32                app_code_csf;               /* Always 0 */
  const void* const* dcd_pointer_pointer;        /* Address to dcd_pointer */
  u32                super_root_key;             /* Always 0 */
  const void*        dcd_pointer;                /* Pointer to dcd entries */
  const void*        app_base_address;           /* VMA of loader (see NOTE) */
  u32                magic;                      /* Always 0xb17219e9 */
  u32                dcd_size;                   /* Size of dcd entry array */
};

struct dcd_entry {
  u32		     type;
  u32 		     address;
  u32 		     value;
};

struct dcd_trailer {
  u32		     image_size; 		/* Size of Loader Image */
};

/** probably unnecessary jump to the start of the loader that preceeds
    the header.  The boot ROM copies this data to RAM but it doesn't
    need it since it is given an entry point in the DCD header.  */
void __naked __section (.header.entry) header_entry (void)
{
  __asm volatile ("b reset\n\t");
}

void header_exit (void);

const __section (.header.rodata.1) struct dcd_entry dcd_entries[] = {

  { 4, PHYS_IOMUXC + 0x3d8, 0x0002 }, /* Activate power-down watchdog*/

  /* Configure IOMUX for DDR2 */
  { 4, PHYS_IOMUXC + 0x8a0, 0x0200 }, /* 0: 0, CMOS; 0x200 DDR */
  { 4, PHYS_IOMUXC + 0x50c, 0x20c3 }, /* 20c5: 0x2 med drive, 0x4 high drive */
  { 4, PHYS_IOMUXC + 0x510, 0x20c3 }, /* 20c5 */
  { 4, PHYS_IOMUXC + 0x83c, 0x0002 }, /* 5 */
  { 4, PHYS_IOMUXC + 0x848, 0x0002 }, /* 5 */
  { 4, PHYS_IOMUXC + 0x4b8, 0x00e7 },
  { 4, PHYS_IOMUXC + 0x4bc, 0x0045 },
  { 4, PHYS_IOMUXC + 0x4c0, 0x0045 },
  { 4, PHYS_IOMUXC + 0x4c4, 0x0045 },
  { 4, PHYS_IOMUXC + 0x4c8, 0x0045 },
  { 4, PHYS_IOMUXC + 0x820, 0x0000 },
  { 4, PHYS_IOMUXC + 0x4a4, 0x0005 },
  { 4, PHYS_IOMUXC + 0x4a8, 0x0005 },
  { 4, PHYS_IOMUXC + 0x4ac, 0x00e3 }, /* e5 */
  { 4, PHYS_IOMUXC + 0x4b0, 0x00e3 }, /* e5 */
  { 4, PHYS_IOMUXC + 0x4b4, 0x00e3 }, /* e5 */
  { 4, PHYS_IOMUXC + 0x4cc, 0x00e3 }, /* e5 */
  { 4, PHYS_IOMUXC + 0x4d0, 0x00e4 }, // DRAM_CS1, drive strength e2 -> e4

  /* Configure IO line drive strength to maximum */
  { 4, PHYS_IOMUXC + 0x82c, 0x0004 },
  { 4, PHYS_IOMUXC + 0x8a4, 0x0004 },
  { 4, PHYS_IOMUXC + 0x8ac, 0x0004 },
  { 4, PHYS_IOMUXC + 0x8b8, 0x0004 },

  /* SDRAM init 13 rows, 10 columns, 32 bit, SREF=4, Micron, CAS3, BL=4 */
  { 4, ESDCTL_ESDCTL0_, 0x82a20000 },
  { 4, ESDCTL_ESDCTL1_, 0x82a20000 },
  { 4, ESDCTL_ESDMISC_, 0xcaaaf6d0 },
  //  { 4, ESDCTL_ESDMISC_, 0x000ad0d0 },
  { 4, ESDCTL_ESDCFG0_, 0x333574aa }, /* 3f3574aa: slower exit of self-refr */
  { 4, ESDCTL_ESDCFG1_, 0x333574aa }, /* 3f3574aa */

  /* Configure SDRAM bank 0 */
  { 4, ESDCTL_ESDSCR_,  0x04008008 },
  { 4, ESDCTL_ESDSCR_,  0x0000801a },
  { 4, ESDCTL_ESDSCR_,  0x0000801b },
  { 4, ESDCTL_ESDSCR_,  0x00448019 },
  { 4, ESDCTL_ESDSCR_,  0x07328018 },
  { 4, ESDCTL_ESDSCR_,  0x04008008 },
  { 4, ESDCTL_ESDSCR_,  0x00008010 },
  { 4, ESDCTL_ESDSCR_,  0x00008010 },
  { 4, ESDCTL_ESDSCR_,  0x06328018 },
  { 4, ESDCTL_ESDSCR_,  0x03808019 },
  { 4, ESDCTL_ESDSCR_,  0x00408019 },
  { 4, ESDCTL_ESDSCR_,  0x00008000 },

  /* Configure SDRAM bank 1 */
  { 4, ESDCTL_ESDSCR_,  0x0400800c },
  { 4, ESDCTL_ESDSCR_,  0x0000801e },
  { 4, ESDCTL_ESDSCR_,  0x0000801f },
  { 4, ESDCTL_ESDSCR_,  0x0000801d },
  { 4, ESDCTL_ESDSCR_,  0x0732801c },
  { 4, ESDCTL_ESDSCR_,  0x0400800c },
  { 4, ESDCTL_ESDSCR_,  0x00008014 },
  { 4, ESDCTL_ESDSCR_,  0x00008014 },
  { 4, ESDCTL_ESDSCR_,  0x0632801c },
  { 4, ESDCTL_ESDSCR_,  0x0380801d },
  { 4, ESDCTL_ESDSCR_,  0x0040801d },
  { 4, ESDCTL_ESDSCR_,  0x00008004 },

  { 4, ESDCTL_ESDCTL0_, 0xb2a20000 },
  { 4, ESDCTL_ESDCTL1_, 0xb2a20000 },
  { 4, ESDCTL_ESDMISC_, 0x000ad6d0 },
  { 4, ESDCTL_ESDGPR_,  0x90000000 },
  { 4, ESDCTL_ESDSCR_,  0x00000000 },
};

extern char APEX_VMA_START[];
extern char APEX_VMA_COPY_SIZE[];
void reset (void);

const __section (.header.rodata.0) struct dcd_header header_ = {
  reset,
  .barker = 0xb1,
  0,
  &header_.dcd_pointer,
  0,
  &header_.magic,
  APEX_VMA_START,
  0xb17219e9,
  sizeof (dcd_entries)
};

const __section (.header.rodata.2) struct dcd_trailer trailer = {
  (u32) APEX_VMA_COPY_SIZE,
};
