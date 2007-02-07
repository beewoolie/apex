/* config.h

   written by Marc Singer
   23 Jun 2006

   Copyright (C) 2006 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__CONFIG_H__)
#    define   __CONFIG_H__

#define PHYS_PARAMS		0x00000100 /* Address for the parameter list */

#define RAM_BANK0_START		0x00000000
#define RAM_BANK0_LENGTH	(32*1024*1024)

//#define RAM_BANK1_START	   0xd0000000
//#define RAM_BANK1_LENGTH   0x10000000

#define INITRD_START		(0x01000000)
#define INITRD_LENGTH		(0x00400000)

#define COMMANDLINE\
 "console=ttyS0,115200 rtc-x1205,probe0,0x6f noirgdebug"

#define MACH_TYPE		   999

/* Uncomment one of these if you need to switch the CPU into a
   specific mode. */
#define FORCE_LITTLEENDIAN
//#define FORCE_BIGENDIAN

#endif  /* __CONFIG_H__ */
