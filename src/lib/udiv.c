/* udiv.c
     $Id$

   written by Marc Singer
   4 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

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

