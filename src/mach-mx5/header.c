/** @file header.c

   written by Marc Singer
   1 Jul 2011

   Copyright (C) 2011 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#include <config.h>
#include <attributes.h>
//#include <asm/bootstrap.h>
#include <arch-arm.h>
#include <asm/cp15.h>
#include <mach/memory.h>
#include <sdramboot.h>

void __naked __section (.header.entry) header_entry (void)
{
  __asm volatile ("b header_exit\n\t");
}

void __naked __section (.header) header (void)
{
  __asm volatile (".word 0x12345678");
}

void __naked __section (.header.exit) header_exit (void)
{
}


