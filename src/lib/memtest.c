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


/* memory_test_0

   is a data bus/address bus memory verification test for verifying
   that memory is OK before we copy APEX into SDRAM.

*/

int __naked __section (.bootstrap) memory_test_0 (unsigned long address,
						  unsigned long c)
{
  const unsigned long pattern_a = 0xaaaaaaaa;
  const unsigned long pattern_b = 0x55555555;

  volatile unsigned long* p = (volatile unsigned long*) address;
  unsigned long offset;
  unsigned long mark;

  __asm volatile ("mov fp, lr");

  c /= 4;			/* Count of words */

		/* Walking data bit */
  for (mark = 1; mark; mark <<= 1) {
    *(volatile unsigned long*) address = mark;
    if (*(volatile unsigned long*) address != mark)
      __asm volatile ("mov r0, #1\n\t"
		      "mov pc, fp");
  }

		/* Walking address bits */
  for (offset = 1; offset < c; offset <<= 1)
    p[offset] = pattern_a;

  p[0] = pattern_b;

  for (offset = 1; offset < c; offset <<= 1)
    if (p[offset] != pattern_a)
	__asm volatile ("mov r0, #2\n\t"
			"mov pc, fp");

  p[0] = pattern_a;

  for (mark = 1; mark < c; mark <<= 1) {
    p[mark] = pattern_b;
    if (p[0] != pattern_a)
	__asm volatile ("mov r0, #3\n\t"
			"mov pc, fp");

    for (offset = 1; offset < c; offset <<= 1)
      if (p[offset] != pattern_a && offset != mark)
	__asm volatile ("mov r0, #4\n\t"
			"mov pc, fp");

    p[mark] = pattern_a;
  }

		/* Full memory test */
  for (offset = 0; offset < c; ++offset)
    p[offset] = offset + 1;

  for (offset = 0; offset < c; ++offset) {
    if (p[offset] != offset + 1)
      __asm volatile ("mov r0, %0\n\t"
		      "mov pc, fp" :: "r" (offset*4));
    p[offset] = ~(offset + 1);
  }
  for (offset = 0; offset < c; ++offset)
    if (p[offset] != ~(offset + 1))
      __asm volatile ("mov r0, %0\n\t"
		      "mov pc, fp" :: "r" (offset*4));

  __asm volatile ("mov r0, #0\n\t"
		  "mov pc, fp");
  return 0;			/* Redundant, but calms the compiler */
}
