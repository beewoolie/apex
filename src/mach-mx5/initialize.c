/* initialize.c

   written by Marc Singer
   22 Jan 2007

   Copyright (C) 2007 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   Hardware initializations.  Some initialization may be left to
   drivers, such as the serial interface initialization.

*/

#include <config.h>
#include <asm/bootstrap.h>
#include <service.h>
#include <sdramboot.h>
#include <asm/cp15.h>

#include "hardware.h"
#include <debug_ll.h>


/** nop bootstrap iniitialization function.  The iMX5 boot ROM
    performs the SDRAM setup before copying the loader to SDRAM.  The
    return value is always true because of the loader is always
    already in SDRAM.

*/

void __naked __section (.bootstrap) initialize_bootstrap (void)
{
  __asm volatile ("mov r0, #-1\n\t");
}


/* target_init

   performs the rest of the hardware initialization that didn't have
   to be performed during the bootstrap phase.

*/

static void target_init (void)
{
#if 0
#if defined (CONFIG_MACH_EXBIBLIO_ROSENCRANTZ)
  IOMUX_PIN_CONFIG_GPIO  (PIN_ILLUMINATION_EN1);
  GPIO_PIN_CONFIG_OUTPUT (PIN_ILLUMINATION_EN1);
  GPIO_PIN_CLEAR	 (PIN_ILLUMINATION_EN1);

  IOMUX_PIN_CONFIG_GPIO  (PIN_ILLUMINATION_EN2);
  GPIO_PIN_CONFIG_OUTPUT (PIN_ILLUMINATION_EN2);
  GPIO_PIN_CLEAR	 (PIN_ILLUMINATION_EN2);

  IOMUX_PIN_CONFIG_GPIO  (PIN_BOARD_ID1);
  GPIO_PIN_CONFIG_INPUT  (PIN_BOARD_ID1);
  IOMUX_PIN_CONFIG_GPIO  (PIN_BOARD_ID2);
  GPIO_PIN_CONFIG_INPUT  (PIN_BOARD_ID2);

#endif

#if !defined (NO_INIT)
  /* Initialize AIPS (AHB to IP bus) */
  AIPS1_MPR1 = 0x77777777;
  AIPS1_MPR2 = 0x77777777;
  AIPS2_MPR1 = 0x77777777;
  AIPS2_MPR2 = 0x77777777;
  AIPS1_OPACR1 = 0;
  AIPS1_OPACR2 = 0;
  AIPS1_OPACR3 = 0;
  AIPS1_OPACR4 = 0;
  AIPS1_OPACR5 &= ~0xff000000;
  AIPS2_OPACR1 = 0;
  AIPS2_OPACR2 = 0;
  AIPS2_OPACR3 = 0;
  AIPS2_OPACR4 = 0;
  AIPS2_OPACR5 &= ~0xff000000;

  /* Initialize MAX (Multi-layer AHB Crossbar Switch) */
  MAX_MPR0 = 0x00302154;
  MAX_MPR1 = 0x00302154;
  MAX_MPR2 = 0x00302154;
  MAX_MPR3 = 0x00302154;
  MAX_MPR4 = 0x00302154;
  MAX_SGPCR0 = 0x10;
  MAX_SGPCR1 = 0x10;
  MAX_SGPCR2 = 0x10;
  MAX_SGPCR3 = 0x10;
  MAX_SGPCR4 = 0x10;
  MAX_MGPCR0 = 0;
  MAX_MGPCR1 = 0;
  MAX_MGPCR2 = 0;
  MAX_MGPCR3 = 0;
  MAX_MGPCR4 = 0;

  /* Initialize M3IF (Multi-Master Memory Interface) */
  M3IF_CTL = (1<<M3IF_M_IPU1);
#endif

#if defined (CONFIG_USES_DM9000)

	/* Configure the DM9000 for Asynchronous level sensitive DTACK mode */
  WEIM_UCR(DM_WEIM_CS) = 0
    | (3<<14)                   /* CNC, AHB clock width minimum CS pulse */
    | (3<<8)                    /* WSC */
    | (1<<7)                    /* EW, DTACK level sensitive */
    | (2<<4)                    /* WWS, two extra clocks for write setup */
	/* DM9000 needs 2 clocks @20ns each to recover from a transaction */
    | (6<<0)                    /* EDC, 6 cycles between transactions */
    ;

  WEIM_LCR(DM_WEIM_CS) = 0
	/* DM9000 needs a slight delay within CS for nOE */
    | (4<<28)                   /* OEA, 1/2 AHB clock delay til assert OE */
    | (4<<24)                   /* OEN, 1/2 AHB clock delay til deassert OE */
    | (5<<8)                    /* DSZ, 16 bit bus width */
    | (1<<0)                    /* CSEN, enable CS */
    ;

  WEIM_ACR(DM_WEIM_CS) = 0
	/* DM9000 needs a slight delay within CS for R_Wn */
	/* Karma needs a delay so that the address, which passes
           through a buffer, is ready before the R_Wn signal is
           asserted.  Rosencrantz doesn't need this, but it doesn't
           really hurt. */
    | (4<<20)                   /* RWA, 1/2 AHB clock delay til assert RW */
    | (4<<16)                   /* RWN, 1/2 AHB clock delay til deassert RW */
    | (3<<4)                    /* DCT, AHB clock delay til nDTACK check */
    ;
#endif
#endif
}


static void target_release (void)
{
}

static __service_0 struct service_d mx3x_target_service = {
  .init    = target_init,
  .release = target_release,
};
