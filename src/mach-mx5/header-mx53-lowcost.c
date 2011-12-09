/** @file header-mx53-lowcost.c

   written by Marc Singer
   16 Dec 2011

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

  { 4, 0x53fa8554, 0x00300000 },
  { 4, 0x53fa8558, 0x00300040 },
  { 4, 0x53fa8560, 0x00300000 },
  { 4, 0x53fa8564, 0x00300040 },
  { 4, 0x53fa8568, 0x00300040 },
  { 4, 0x53fa8570, 0x00300000 },
  { 4, 0x53fa8574, 0x00300000 },
  { 4, 0x53fa8578, 0x00300000 },
  { 4, 0x53fa857c, 0x00300040 },
  { 4, 0x53fa8580, 0x00300040 },
  { 4, 0x53fa8584, 0x00300000 },
  { 4, 0x53fa8588, 0x00300000 },
  { 4, 0x53fa8590, 0x00300040 },
  { 4, 0x53fa8594, 0x00300000 },
  { 4, 0x53fa86f0, 0x00300000 },
  { 4, 0x53fa86f4, 0x00000000 },
  { 4, 0x53fa86fc, 0x00000000 },
  { 4, 0x53fa8714, 0x00000000 },
  { 4, 0x53fa8718, 0x00300000 },
  { 4, 0x53fa871c, 0x00300000 },
  { 4, 0x53fa8720, 0x00300000 },
  { 4, 0x53fa8724, 0x04000000 },
  { 4, 0x53fa8728, 0x00300000 },
  { 4, 0x53fa872c, 0x00300000 },
  { 4, 0x63fd9088, 0x35343535 },
  { 4, 0x63fd9090, 0x4d444c44 },
  { 4, 0x63fd907c, 0x01370138 },
  { 4, 0x63fd9080, 0x013b013c },
  { 4, 0x63fd9018, 0x00011740 },
  { 4, 0x63fd9000, 0xc3190000 },
  { 4, 0x63fd900c, 0x9f5152e3 },
  { 4, 0x63fd9010, 0xb68e8a63 },
  { 4, 0x63fd9014, 0x01ff00db },
  { 4, 0x63fd902c, 0x000026d2 },
  { 4, 0x63fd9030, 0x009f0e21 },
  { 4, 0x63fd9008, 0x12273030 },
  { 4, 0x63fd9004, 0x0002002d },
  { 4, 0x63fd901c, 0x00008032 },
  { 4, 0x63fd901c, 0x00008033 },
  { 4, 0x63fd901c, 0x00028031 },
  { 4, 0x63fd901c, 0x052080b0 },
  { 4, 0x63fd901c, 0x04008040 },
  { 4, 0x63fd901c, 0x0000803a },
  { 4, 0x63fd901c, 0x0000803b },
  { 4, 0x63fd901c, 0x00028039 },
  { 4, 0x63fd901c, 0x05208138 },
  { 4, 0x63fd901c, 0x04008048 },
  { 4, 0x63fd9020, 0x00005800 },
  { 4, 0x63fd9040, 0x04b80003 },
  { 4, 0x63fd9058, 0x00022227 },
  { 4, 0x63fd901c, 0x00000000 },
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
