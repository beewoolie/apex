/* bsign.h						-*- C++ -*-
   $Id: bsign.h,v 1.16 2003/08/21 07:13:07 elf Exp $
   
   written by Marc Singer (aka Oscar Levi)
   1 Dec 1998

   This file is part of the project BSIGN.  See the file README for
   more information.

   Copyright (c) 1998,2003 Marc Singer

   This program is free software; you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__BSIGN_H__)
#    define   __BSIGN_H__

/* ----- Includes */

#include "exitstatus.h"

/* ----- Types */

struct SigInfo {
  int version;			// Version of signature block
  size_t ibSection;		// Origin and extent of signature section
  size_t cbSection;
//size_t ibIdentifier;		// Origin and extent of ID string
//size_t cbIdentifier;
  size_t ibHash;		// Origin and extent of hash
  size_t cbHash;
  size_t ibSignature;		// Origin and extent of digital signature
  size_t cbSignature;
};

/* ----- Globals */

bool is_elf (char* pb, size_t cb);
bool is_elf_header (const char* pb, size_t cb);
bool is_elf_signed (char* pb, size_t cb, SigInfo* pInfo);
bool is_elf_rewritten (char* pb, size_t cb, SigInfo* pInfo);
eExitStatus rewrite_elf (char* pb, size_t cb, int fhNew, SigInfo* pInfo);
eExitStatus hash_elf (char* pb, size_t cb, int fhNew, SigInfo* pInfo, 
		      bool fSign);
eExitStatus check_elf (char* pb, size_t cb, bool fExpectSignature, 
		       SigInfo* pInfo);
int size_elf_header (void);

#endif  /* __BSIGN_H__ */
