/* stacklimit.c

   written by Marc Singer
   10 July 2006

   Copyright (C) 2006 Marc Singer

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

*/

#include <config.h>
#include <apex.h>
#include <service.h>

extern char APEX_VMA_STACKS_START;
extern char APEX_VMA_STACK_START;

/* stacklimit_report

   probes the stack to determine how much of the allocated stack space
   has been used.

*/

void stacklimit_report (void)
{
  int i;
  unsigned max = &APEX_VMA_STACK_START - &APEX_VMA_STACKS_START;
  for (i = 4; i < max; i += 4)
    if (((unsigned long*)&APEX_VMA_STACKS_START)[i] != 0xe5e5e5e5)
      break;

  printf ("   stack: %d free (%d) %p %p\n",
	  i, max, &APEX_VMA_STACKS_START, &APEX_VMA_STACK_START);
}

static __service_0 struct service_d stacklimit_service = {
  .report = stacklimit_report,
};
