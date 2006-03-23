/* exception_vectors.c
     $Id$

   written by Marc Singer
   15 Nov 2004

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

   These exception vectors are the defaults for the platform.  A
   machine implementation may override them by defining the
   exception_vectors() function.

   It turns out that we don't need or want vectors at the top of the
   loader.  This is something copied from others and it has no value.
   For the time being, this vestige remains as a NOP.  Should there be
   a need for an exception vector table, it will be generated at
   run-time.

   Also note that a platform can still override this function to
   create exception vectors of its own.  It's just that part of the
   arm setup is in the way, the ATAGS, so we cannot execute at the
   bottom of memory even if we wanted to.

*/

#include <config.h>
#include <asm/bootstrap.h>

void __naked __section (.entry) entry (void)
{
  /* Fallthrough to reset section.  Exception vectors are handled
     differently.  This entry point exists for two reasons.  First,
     it's used to tell the linker where to enter.  We use the symbol
     entry() and the program entry point.  Second, this function
     allows a target to override the first executed instruction in
     case there is something it has to do.  The replacement function
     should be naked and fall-through to the .reset section. */
}
