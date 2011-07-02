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

/ *
 * L2CC Cache setup/invalidation/disable
 * /
.macro init_l2cc
	/ * explicitly disable L2 cache * /
        mrc 15, 0, r0, c1, c0, 1
        bic r0, r0, #0x2
        mcr 15, 0, r0, c1, c0, 1

        / * reconfigure L2 cache aux control reg * /
        mov r0, #0xC0                   / * tag RAM * /
        add r0, r0, #0x4                / * data RAM * /
        orr r0, r0, #(1 << 24)          / * disable write allocate delay * /
        orr r0, r0, #(1 << 23)          / * disable write allocate combine * /
        orr r0, r0, #(1 << 22)          / * disable write allocate * /

	ldr r1, =0x00000000
	ldr r3, [r1, #ROM_SI_REV]
	cmp r3, #0x10    / * r3 contains the silicon rev * /
	orrls r0, r0, #(1 << 25)    / * disable write combine for TO 2 and lower revs * /

	mcr 15, 1, r0, c9, c0, 2
.endm / * init_l2cc * /

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

  CCM_CSCMR1 = 0xA5A2A020;      /* UART clk <= PLL2, divided to 66.5 MHz */
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
  /* MPROTx set to non-bufferable, trusted for r/w and not forced to
     user-mode */
  __REG(PHYS_AIPS1 + 0) = 0x77777777;
  __REG(PHYS_AIPS1 + 4) = 0x77777777;
  __REG(PHYS_AIPS2 + 0) = 0x77777777;
  __REG(PHYS_AIPS2 + 4) = 0x77777777;

  /* VPU and IPU given higher priority (0x4) IPU accesses with ID=0x1
     given highest priority (=0xA) */
  M4IF_FBPM0 = 0x203;
  __REG (PHYS_M4IF + 0x44) = 0;
  __REG (PHYS_M4IF + 0x9c) = 0x00120125;
  __REG (PHYS_M4IF + 0x48) = 0x001901A3;
}


static void target_release (void)
{
}

static __service_0 struct service_d mx3x_target_service = {
  .init    = target_init,
  .release = target_release,
};
