/* spinner-efikamx.c

   written by Marc Singer
   16 Sep 2011

   Copyright (C) 2011 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

*/

#include <config.h>
#include <apex.h>
#include <service.h>

#include <mach/hardware.h>

#include <spinner.h>

void efikamx_spinner (unsigned v)
{
  static int step;
  static int value;

  if (v == ~0) {		/* Clear */
    value = 0;
    step = 0;
    GPIO_SET (LED_RED);
    return;
  }

  v = v/128;
  if (v == value)
    return;

  value = v;
  switch (step) {
  case 0:
    GPIO_CLEAR (LED_RED); break;
  case 1:
    GPIO_SET (LED_RED); break;
  }
  ++step;
  if (step > 1)
    step = 0;
}

static void spinner_init (void)
{
  hook_spinner = efikamx_spinner;
  GPIO_SET (LED_RED);
}

static void spinner_release (void)
{
  GPIO_SET (LED_RED);
}

static __service_7 struct service_d spinner_service = {
  .init    = spinner_init,
  .release = spinner_release,
};
