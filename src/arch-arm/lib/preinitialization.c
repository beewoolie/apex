/* preinitialization.c

   written by Marc Singer
   16 Oct 2006

   Copyright (C) 2006 Marc Singer

   -----------
   DESCRIPTION
   -----------

   NULL stub for preini

*/

#include <config.h>
#include <apex.h>
#include <attributes.h>

void __naked __section (.preinit) preinitialization (void)
{
  __asm volatile ("mov pc, lr");
}
