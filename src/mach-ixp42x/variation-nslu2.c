/* variation-nslu2.c

   written by Marc Singer
   6 Jan 2007

   Copyright (C) 2007 Marc Singer

   -----------
   DESCRIPTION
   -----------

   Service to support variable variations on the nslu2.  Essetially,
   this is the interface with the user to select the variation at
   boot-time.

*/

#include <config.h>
#include <alias.h>
#include <service.h>
#include <apex.h>

#include "hardware.h"

static void variation_init (void)
{
  int reset_pressed = (GPIO_INR & (1<<GPIO_I_RESETBUTTON)) == 0;

  if (reset_pressed)
    alias_set ("variation", CONFIG_VARIATION_SUFFIX);

  MASK_AND_SET (GPIO_OUTR, 1<<GPIO_I_LEDRED,
		reset_pressed ? (1<<GPIO_I_LEDRED): 0);
}

static __service_9 struct service_d variation_service = {
  .init = variation_init,
};
