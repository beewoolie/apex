/* crc32.cc

   written by Marc Singer
   6 November 2004

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

   An original reference for information about implementating CRC32 is
   a document written by Ross Williams in 1993, "A Painless Guide to
   CRC Error Detection Algorithms."  Using his nomenclature, the CRC
   algorithm implemented by Ethernet and Zip compression has the
   following features:

     Width  : 32
     Poly   : 04C11DB7
     iPoly  : EDB88320		// reverse bit order
     Init   : FFFFFFFF
     RefIn  : True
     RefOut : True
     XorOut : FFFFFFFF
     Check  : CBF43926

   Width parameter is the length, in bits, of the polynomial.  The
   Poly is a bit mask indicating which terms have non-zero
   coefficients.  In this case,

     g(x) = x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 +
	    x^7 + x^5 + x^4 + x^2 + x + 1

   Init is value used to initialize the CRC function.  RefIn and
   RefOut, being true, indicate that the algorithm interprets the MSB
   of each byte as an MSB.  XorOut is the XOR mask applied to the CRC
   on completion.  Check is the CRC for the sequence "123456789".

   The implementation used herein comes from BLOB and was the work of
   several people, Erik Muow, Jan-Derk Bakker, and Russell King.  It
   was found to be faster than the table algorithm though a little
   larger, ~124 bytes.

*/

#define POLY        (0xedb88320)

#if 0

/* Allegedly faster version, we want the smallest code possible */

unsigned long compute_crc32_x (unsigned long crc, const void* pv, int cb)
{
  unsigned char* pb = (unsigned char*) pv;
  unsigned long poly = POLY;	// Hack to get a register allocated

  crc = crc ^ 0xffffffff;	// Invert because we're continuing

#define DO_CRC\
  if (crc & 1) { \
    crc >>= 1; crc ^= poly; }\
  else { crc >>= 1; }

  while (cb--) {
    crc ^= *pb++;

    DO_CRC;
    DO_CRC;
    DO_CRC;
    DO_CRC;
    DO_CRC;
    DO_CRC;
    DO_CRC;
    DO_CRC;
  }

  return crc ^ 0xffffffff;
}

#endif


/* compute_crc32

   calculates a standard 32 bit CRC checksum over a sequence of bytes.
   This implementation is optimized for the ARM architecture.  Other
   architectures, e.g. i386 will probably not be well served by this
   version as it depends on excellent optimization by the compiler as
   well as a adequate number of registers.

*/

unsigned long compute_crc32 (unsigned long crc, const void *pv, int cb)
{
  const unsigned char* pb = (const unsigned char *) pv;

  crc ^= 0xffffffff;

  while (cb--) {
    int i;
    crc ^= *pb++;

    for (i = 8; i--; ) {
      if (crc & 1) {
	crc >>= 1;
	crc ^= POLY;
      }
      else
	crc >>= 1;
    }
  }

  return crc ^ 0xffffffff;
}
