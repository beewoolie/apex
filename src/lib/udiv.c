/* udiv.c
     $Id$

   4 Nov 2004

   Copyright (C) 2004 Marc Singer

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

   This code comes from the Blob source.  It is a short implementation
   of a necessary compiler support function.

*/

static unsigned long udivmodsi4 (unsigned long numerator,
				 unsigned long denominator,
				 int return_modulus)
{
  unsigned long bit = 1;
  unsigned long result = 0;

  while (denominator < numerator && bit && !(denominator & (1L<<31))) {
    denominator <<= 1;
    bit <<= 1;
  }
  while (bit) {
    if (numerator >= denominator) {
      numerator -= denominator;
      result |= bit;
    }
    bit >>= 1;
    denominator >>= 1;
  }

  return return_modulus ? numerator : result;
}

long __udivsi3 (long a, long b)
{
  return udivmodsi4 (a, b, 0);
}

long __umodsi3 (long a, long b)
{
  return udivmodsi4 (a, b, 1);
}
