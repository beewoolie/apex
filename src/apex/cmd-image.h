/* image.h

   written by Marc Singer
   12 Sep 2008

   Copyright (C) 2008 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__IMAGE_H__)
#    define   __IMAGE_H__

/* ----- Includes */

#include <driver.h>

/* ----- Types */

typedef int (*pfn_handle_image) (int, struct descriptor_d*, bool);

/* ----- Globals */

/* ----- Prototypes */

pfn_handle_image is_apex_image (const char*, size_t);
pfn_handle_image is_uboot_image (const char*, size_t);



#endif  /* __IMAGE_H__ */
