/* spinner-nslu2.c
     $Id$

   written by Marc Singer
   11 Feb 2005

   Copyright (C) 2005 Marc Singer

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

#include "hardware.h"

#include <spinner.h>

void nslu2_spinner (unsigned v)
{
  static int step;
  v = v%3;
  if (v == step)
    return;

  step = v;
  switch (step) {
  case 0:
    _L(LED1); break;
  case 1:
    _L(LED4); break;
  case 2:
    _L(LED8); break;
  }
}

static void spinner_init (void)
{
  hook_spinner = nslu2_spinner;
}

static void spinner_release (void)
{
  _L(LED2);
}

static __service_3 struct service_d spinner_service = {
  .init    = spinner_init,
  .release = spinner_release,
};

