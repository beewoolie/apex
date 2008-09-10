/* region-checksum.h

   written by Marc Singer
   22 Aug 2008

   Copyright (C) 2008 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__REGION_CHECKSUM_H__)
#    define   __REGION_CHECKSUM_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

enum {
  regionChecksumSpinner	= (1<<0),
  regionChecksumLength	= (1<<1), /* Add non-zero length bytes, LSB first */
};

int region_checksum (struct descriptor_d* dout, unsigned flags,
                     unsigned long* crc_result);


#endif  /* __REGION_CHECKSUM_H__ */
