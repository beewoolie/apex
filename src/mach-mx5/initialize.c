/* initialize.c

   written by Marc Singer
   2 Jul 2011

   Copyright (C) 2011 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   Hardware initializations.  Some initialization may be left to
   drivers, such as the serial interface initialization.

   L2 cache
   --------

   We're not enabling the L2 cache until we find documentation
   explaining the registers.

*/

#include <config.h>
#include <asm/bootstrap.h>
#include <service.h>
#include <sdramboot.h>
#include <asm/cp15.h>

#include "hardware.h"
#include <debug_ll.h>


static inline void __section (.bootstrap)
  setup_dpll (int idx, u32 op, u32 mfd, u32 mfn)
{
  DPLLX_DP_CTL(idx) = 0x1232;   /* DPLL on */
  DPLLX_DP_CONFIG(idx) = 2;     /* AREN */

  DPLLX_DP_OP(idx) = op;
  DPLLX_DP_HFS_OP(idx) = op;

  DPLLX_DP_MFD(idx) = mfd;
  DPLLX_DP_HFS_MFD(idx) = mfd;

  DPLLX_DP_MFN(idx) = mfn;
  DPLLX_DP_HFS_MFN(idx) = mfn;

  DPLLX_DP_CTL(idx) = 0x1232;
  while (DPLLX_DP_CTL (idx) & 1)
    ;
}

/** nop bootstrap iniitialization function.  The iMX5 boot ROM
    performs the SDRAM setup before copying the loader to SDRAM.  The
    return value is always true because of the loader is always
    already in SDRAM.

*/

void __naked __section (.bootstrap) initialize_bootstrap (void)
{
  // Let's do the l2cc, aips, m4if, clock, and
  // debug setup here.

  // Scratch that.  Let's do anything that makes RAM faster but we can
  // relegate peripherap setups to the target_init code once we're all
  // in SDRAM.  So, l2cc, clocks, m4if?

  /* Drive GPIO1.23 high, LED perhaps? */
  GPIOX_DR(1)   |= (1<<23);
  GPIOX_GDIR(1) |= (1<<23);

  /* ARM errata ID #468414 */
  {
    u32 l;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 1\n\t" /* ACTLR */
		    "orr %0, %0, #(1<<5)\n\t" /* Enable L1NEON */
		    "mcr p15, 0, %0, c1, c0, 1\n\t" : "=&r" (l)
		    );
  }

    /* Gate peripheral clocks */
  CCM_CCGR0 = 0x3FFFFFFF;
  CCM_CCGR1 = 0;
  CCM_CCGR2 = 0;
  CCM_CCGR3 = 0;
  CCM_CCGR4 = 0x00030000;
  CCM_CCGR5 = 0x00fff030;
  CCM_CCGR6 = 0x00000300;

  /* Disable IPU and HSC dividers */
  CCM_CCDR  = 0x00060000;

  /* Switch source for DDR away from PLL1 */
  CCM_CBCDR = 0x19239145;

  /* Wait for handshake to complete */
  while (CCM_CDHIPR)
    ;

  /* Switch ARM to step clock */
  CCM_CCSR = 4;

  setup_dpll (1, DPLL_OP_800, DPLL_MFD_800, DPLL_MFN_800);
  setup_dpll (3, DPLL_OP_665, DPLL_MFD_665, DPLL_MFN_665);

  /* Switch peripherals to PLL3 */
  CCM_CBCMR = 0x000010c0;
  CCM_CBCDR = 0x13239145;

  setup_dpll (2, DPLL_OP_665, DPLL_MFD_665, DPLL_MFN_665);

  /* Switch peripherals to PLL2 */
  CCM_CBCDR = 0x19239145;
  CCM_CBCMR = 0x000020c0;

  setup_dpll (3, DPLL_OP_216, DPLL_MFD_216, DPLL_MFN_216);

  ARM_PLATFORM_ICGC = 0x725;    /* Lacking documentation */

  /* Run TO 3.0 at Full speed, for other TO's wait till we increase VDDGP */
  if (BOOT_ROM_SI_REV < 0x10)
    CCM_CACRR = 0x1;
  else
    CCM_CACRR = 0;

  /* Switch ARM to PLL1 */
  CCM_CCSR = 0;

  CCM_CBCMR = 0x20c2;           /* perclk <= lp_apm (24MHz) */
  CCM_CBCDR = 0x59E35100;       /* DDR clk <= pll1, perclk div <= 1 */

  CCM_CCGR0 = 0xffffffff;
  CCM_CCGR1 = 0xffffffff;
  CCM_CCGR2 = 0xffffffff;
  CCM_CCGR3 = 0xffffffff;
  CCM_CCGR4 = 0xffffffff;
  CCM_CCGR5 = 0xffffffff;
  CCM_CCGR6 = 0xffffffff;

  CCM_CSCMR1 = 0xA5A2A020;      /* UART clk <= PLL2, divid to 66.5 MHz */
  CCM_CSCDR1 = 0x00C30321;

  /* Wait for handshake to complete */
  while (CCM_CDHIPR)
    ;

  CCM_CCDR = 0;

  CCM_CCOSR = 0x000a0000 + 0xf0;  /* cko <= for ARM /8 */


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
