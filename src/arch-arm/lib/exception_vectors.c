/* exception_vectors.c
     $Id$

   written by Marc Singer
   15 Nov 2004

   Copyright (C) 2004 Marc Singer

   -----------
   DESCRIPTION
   -----------

   These exception vectors are the defaults for the platform.  A
   machine implementation may override them by defining the
   exception_vectors() function.

*/

#include <config.h>
#include <asm/bootstrap.h>

extern void exception_error (void);

void __naked __section(entry) exception_vectors (void)
{
  __asm ("b reset\n\t"
	 "b exception_error\n\t"
	 "b exception_error\n\t"
	 "b exception_error\n\t"
	 "b exception_error\n\t"
	 "b exception_error\n\t"
	 "b exception_error\n\t"
	 "b exception_error\n\t");
}

void __naked __section (bootstrap) exception_error (void)
{
  while (1)
    ;
}

