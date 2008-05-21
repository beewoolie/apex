/* sdramboot.h

   written by Marc Singer
   30 Sep 2005

   Copyright (C) 2005 Marc Singer

   This program is licensed under the terms of the GNU General Public
   License as published by the Free Software Foundation.  Please refer
   to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__SDRAMBOOT_H__)
#    define   __SDRAMBOOT_H__

/* ----- Includes */

#include <attributes.h>

/* ----- Types */

/* ----- Globals */

#if defined (CONFIG_SDRAMBOOT_REPORT)
extern int __xbss(init) fSDRAMBoot;
#endif

/* ----- Prototypes */



#endif  /* __SDRAMBOOT_H__ */
