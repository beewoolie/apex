/* rfc2440.cc
     $Id: rfc2440.cc,v 1.3 2003/08/11 08:26:23 elf Exp $

   written by Marc Singer
   10 Aug 2003

   Copyright (c) 2003 Marc Singer 

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA

   -----------
   DESCRIPTION
   -----------

   Routines to handle decode of RFC2440 (OpenPGP) packets.  

   See <http://www.ietf.org/rfc/rfc2440.txt>.

   SUMMARY
   -------

   Here is a sample.

000000  88 3f 03 05 00 36 72 19  23 57 e8 ce 07 2e ae 5d  .?...6r.#W.....]
000010  2d 11 02 61 97 00 9f 45  ff ac ff 3f 82 31 0a 30  -..a...E...?.1.0
000020  59 f7 ec c4 9e 58 31 ef  d3 78 f2 00 9f 44 d2 77  Y....X1..x...D.w
000030  e9 b9 03 0f e8 df 9d 4f  0c 5f 86 09 8a 69 1f 01  .......O._...i..
000040  5d

   Byte 0: 	the content type and length-type, an old format,
		signature packet (type 2, bits 5-2), with a one byte
		length (bits 1-0).  
   Byte 1:	the length of the following data 0x3f (63). 
   Byte 2:	this is a version 3 signature packet.
   Byte 3:	length which must be 5.
   Byte 4:	this a signature of a binary document (0).
   Bytes 5-8:	unix time stamp in MSB order.
   Bytes 9-16:	key ID of the signer.  
   Byte 17:	the public-key algorithm is DSA (17).
   Byte 18:	the hash algorithm is SHA1 (2).
   Bytes 19-20:	the most-significant 16 bits of the hash.

*/


#include "standard.h"
#include "rfc2440.h"
#include <time.h>

size_t rfc2440::size (void)
{
  switch (m_rgb[0] & 0xc3) {
  case 0x80:			// One byte length
    return m_rgb[1];
  case 0x81:			// Two byte length
  case 0x82:			// Three byte length
  case 0x83:			// Four byte length
  default:
    return 0;
  }
}

bool rfc2440::is_signature (void)
{
  if (!m_rgb ||  m_cb < 2)
    return false;
  return ((m_rgb[0] & 0x3c) == 2);
}


time_t rfc2440::timestamp (void)
{
  if (m_cb < 9)
    return 0;
  unsigned char* pb = (unsigned char*) &m_rgb[5];
  return (pb[0] << 24) | (pb[1] << 16) | (pb[2] << 8) | pb[3];
}


const char* rfc2440::timestring (void)
{
  if (m_cb < 9)
    return NULL;
  time_t ts = timestamp ();
  struct tm* ptm = localtime (&ts);
  static char sz[80];
  strftime (sz, sizeof (sz), "%d %b %Y %H:%M:%S", ptm);
  return sz;
}


const char* rfc2440::describe_id (void)
{
  if (m_cb < 17)
    return NULL;
  static char sz[17];
  unsigned char* pb = (unsigned char*) &m_rgb[9];
  sprintf (sz, "%02X%02X%02X%02X%02X%02X%02X%02X",
	   pb[0], pb[1], pb[2], pb[3], pb[4], pb[5], pb[6], pb[7]);
  return sz;
}
