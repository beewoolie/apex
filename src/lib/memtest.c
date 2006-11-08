/* memtest.c

   written by Marc Singer
   7 Nov 2006

   Copyright (C) 2006 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#include <config.h>
#include <attributes.h>

#include <memtest.h>

#include <debug_ll.h>

/* memory_test

   peforms a full-spectrum (eventually?) memory test of the given
   region.  The return value is zero for success and non-zero on
   error.

*/


/* memory_test_0

   is a data bus memory verification test for verifying that memory is
   OK before we copy APEX into SDRAM.

*/

int __naked __section (.bootstrap) memory_test_0 (unsigned long address,
					  unsigned long cb)
{
  unsigned long v;

  __asm volatile ("mov fp, lr");

		/* Walking data bit */
  for (v = 1; v; v <<= 1) {
    *(volatile unsigned long*) address = v;
    if (*(volatile unsigned long*) address != v)
      __asm volatile ("mov r0, #1\n\t"
		      "mov pc, fp");
  }

  __asm volatile ("mov r0, #0\n\t"
		  "mov pc, fp");
  return 0;			/* Redundant, but calms the compiler */
}


/* memory_test_1

   address bus test.

 */

int __naked __section (.bootstrap) memory_test_1 (unsigned long address,
						  unsigned long cb)
{
  const unsigned long pattern_a = 0xaaaaaaaa;
  const unsigned long pattern_b = 0x55555555;

  volatile unsigned long* p = (volatile unsigned long*) address;
  unsigned long offset;
  unsigned long mark;

  __asm volatile ("mov fp, lr");

  for (offset = 1; offset < cb; offset <<= 1)
    p[offset] = pattern_a;

  p[0] = pattern_b;

  for (offset = 1; offset < cb; offset <<= 1)
    if (p[offset] != pattern_a)
	__asm volatile ("mov r0, #2\n\t"
			"mov pc, fp");

  p[0] = pattern_a;

  for (mark = 1; mark < cb; mark <<= 1) {
    p[mark] = pattern_b;
    if (p[0] != pattern_a)
	__asm volatile ("mov r0, #3\n\t"
			"mov pc, fp");

    for (offset = 1; offset < cb; offset <<= 1)
      if (p[offset] != pattern_a && offset != mark)
	__asm volatile ("mov r0, #4\n\t"
			"mov pc, fp");

    p[mark] = pattern_a;
  }

  __asm volatile ("mov r0, #0\n\t"
		  "mov pc, fp");
  return 0;			/* Redundant, but calms the compiler */
}
