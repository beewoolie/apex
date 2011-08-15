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

   Board Revision ID
   -----------------

   Board IDs are read from three IO lines, NANDF_CS0, NANDF_CS1, and
   NANDF_RB3 from least significant bit to most significant.  The
   mapping of IDs to board revisions so far, are,

   NANDF_RB3  NANDF_CS1	 NANDF_CS0  Board Revision
   ---------  ---------	 ---------  --------------
       1	  1	     1	         1.1
       1	  1	     0	         1.2
       1	  0	     1           1.3
       1	  0	     0           1.4


   IOMUX of Working SPI Flash
   --------------------------

   The inactive SS is set to GPIO input to, apparently, work around a
   problem with the ECSPI module.


   Initialization Sequence
   -----------------------

// From the kernel

	/ *   AT
	 *  TFR   EV X F   I D LR    S
	 * .EEE ..EE PUI. .T.T 4RVI ZWRS BLDP WCAM
	 * rxxx rrxx xxx0 0101 xxxx xxxx x111 xxxx < forced
	 *    1    0 110       0011 1100 .111 1101 < we want
	 * /
	.type	v7_crval, #object
v7_crval:
	crval	clear=0x0120c302, mmuset=0x10c03c7d, ucset=0x00c01c7c

From kernel, setup of r6 so that we can determine which errata apply:
		see arch/arm/mm/proc-v7.S

	mrc	p15, 0, r0, c0, c0, 0		@ read main ID register
	and	r10, r0, #0xff000000		@ ARM?
	teq	r10, #0x41000000
	bne	3f
	and	r5, r0, #0x00f00000		@ variant
	and	r6, r0, #0x0000000f		@ revision
	orr	r6, r6, r5, lsr #20-4		@ combine variant and revision
	ubfx	r0, r0, #4, #12			@ primary part number


#endif

*/

#include <config.h>
#include <asm/bootstrap.h>
#include <service.h>
#include <sdramboot.h>
#include <asm/cp15.h>
#include <apex.h>       /* *** printf */

#include "mx5-spi.h"

#include "hardware.h"
#include <debug_ll.h>

//static int spi1_slave = SPI1_SS_FLASH;

#define ESDHC_INTERFACE()\
  ((BOARD_ID_MAJOR == 1 && BOARD_ID_MINOR == 1) ? 1 : 0)

static inline void __section (.bootstrap)
  setup_dpll (int idx, u32 op, u32 mfd, u32 mfn)
{
  DPLLx_DP_CTL(idx) = 0x1232;   /* DPLL on */
  DPLLx_DP_CONFIG(idx) = 2;     /* AREN */

  DPLLx_DP_OP(idx) = op;
  DPLLx_DP_HFS_OP(idx) = op;

  DPLLx_DP_MFD(idx) = mfd;
  DPLLx_DP_HFS_MFD(idx) = mfd;

  DPLLx_DP_MFN(idx) = mfn;
  DPLLx_DP_HFS_MFN(idx) = mfn;

  DPLLx_DP_CTL(idx) = 0x1232;
  while (DPLLx_DP_CTL (idx) & 1)
    ;
}


u32 board_revision (void)
{
  static u32 revision;

  if (!revision) {

    /* SOC revision */
    switch (BOOT_ROM_SI_REV) {
    default:
      revision |= CHIP_1_0;
      break;
    case 0x02:
      revision |= CHIP_1_1;
      break;
    case 0x10:
      revision |= (GPIOx_DR(1) && (1<<22)) ? CHIP_2_0 : CHIP_2_5;
      break;
    case 0x20:
      revision |= CHIP_3_0;
      break;
    }

    /* Board revision */
    GPIO_SET           (MX51_PIN_NANDF_CS0);
    GPIO_CONFIG_OUTPUT (MX51_PIN_NANDF_CS0); /* Charge capacitance, rev1.1 */

    GPIO_CONFIG_PAD    (MX51_PIN_NANDF_CS0, GPIO_PAD_PU_100K);
    GPIO_CONFIG_INPUT  (MX51_PIN_NANDF_CS0);
    revision |= GPIO_VALUE (MX51_PIN_NANDF_CS0) ? (1<<BOARD_ID_SHIFT) : 0;

    GPIO_CONFIG_PAD    (MX51_PIN_NANDF_CS1, GPIO_PAD_PU_100K);
    GPIO_CONFIG_INPUT  (MX51_PIN_NANDF_CS1);
    revision |= GPIO_VALUE (MX51_PIN_NANDF_CS1) ? (2<<BOARD_ID_SHIFT) : 0;

    GPIO_CONFIG_PAD    (MX51_PIN_NANDF_RB3, GPIO_PAD_PU_100K);
    GPIO_CONFIG_INPUT  (MX51_PIN_NANDF_RB3);
    revision |= GPIO_VALUE (MX51_PIN_NANDF_RB3) ? (4<<BOARD_ID_SHIFT) : 0;

    revision ^= BOARD_ID_MASK << BOARD_ID_SHIFT; /* Invert board ID bits */
  }

  return revision;
}

const char* describe_board_revision (void)
{
  static char sz[80];
  u32 revision = board_revision ();
  snprintf (sz, sizeof (sz), "soc %d.%02d  board 1.%d",
            ((revision >> CHIP_ID_SHIFT) & CHIP_ID_MASK) >> 8,
            (((revision >> CHIP_ID_SHIFT) & CHIP_ID_MASK) && 0xff)/10,
            ((revision >> BOARD_ID_SHIFT) & BOARD_ID_MASK) + 1);
  return sz;
}


/** activate the SS line for the given slave on the SPI1 port.
    ***FIXME: need to make sure that this call is idempotent.  We may
    ***need to keep track of the currently selected device. */

void spi_select (const struct mx5_spi* spi)
{
  int slave = spi ? spi->slave : -1;

  if (spi && spi->bus != 1)     /* Only BUS 1 implemented */
    return;

  switch (slave) {

  default:                      /* Disable all */
    GPIO_CONFIG_INPUT (MX51_PIN_CSPI1_SS0);
    GPIO_CONFIG_PAD   (MX51_PIN_CSPI1_SS0,
                       GPIO_PAD_PKE | GPIO_PAD_PKE | GPIO_PAD_PD_100K
                       | GPIO_PAD_DRIVE_HIGH
                       | GPIO_PAD_SLEW_FAST);
    GPIO_CONFIG_INPUT (MX51_PIN_CSPI1_SS1);
    GPIO_CONFIG_PAD   (MX51_PIN_CSPI1_SS1,
                       GPIO_PAD_PKE | GPIO_PAD_PKE | GPIO_PAD_PU_100K
                       | GPIO_PAD_DRIVE_HIGH
                       | GPIO_PAD_SLEW_FAST);
    break;

  case 0:
    GPIO_CONFIG_INPUT (MX51_PIN_CSPI1_SS1);
    GPIO_CONFIG_PAD   (MX51_PIN_CSPI1_SS1,
                       GPIO_PAD_PKE | GPIO_PAD_PKE | GPIO_PAD_PU_100K
                       | GPIO_PAD_DRIVE_HIGH
                       | GPIO_PAD_SLEW_FAST);
    GPIO_CONFIG_FUNC  (MX51_PIN_CSPI1_SS0, 3); /* jolen workaround?  */
    GPIO_CONFIG_FUNC  (MX51_PIN_CSPI1_SS0, 0);
    GPIO_CONFIG_PAD   (MX51_PIN_CSPI1_SS0,
                       GPIO_PAD_HYST_EN | GPIO_PAD_PKE
                       | GPIO_PAD_DRIVE_HIGH | GPIO_PAD_SLEW_FAST);
    break;

  case 1:
    GPIO_CONFIG_INPUT (MX51_PIN_CSPI1_SS0);
    GPIO_CONFIG_PAD   (MX51_PIN_CSPI1_SS0,
                       GPIO_PAD_PKE | GPIO_PAD_PKE | GPIO_PAD_PD_100K
                       | GPIO_PAD_DRIVE_HIGH
                       | GPIO_PAD_SLEW_FAST);
    GPIO_CONFIG_FUNC  (MX51_PIN_CSPI1_SS1, 0);
    GPIO_CONFIG_PAD   (MX51_PIN_CSPI1_SS1,
                       GPIO_PAD_HYST_EN
                       | GPIO_PAD_DRIVE_HIGH | GPIO_PAD_SLEW_FAST);
    break;
  }
}


/** bootstrap iniitialization function.  The iMX5 boot ROM performs
    the SDRAM setup before copying the loader to SDRAM.  The return
    value here is always true because of the loader is always already
    in SDRAM.  When this function is called, caches must be
    disabled. */

void __naked __section (.bootstrap) initialize_bootstrap (void)
{
  unsigned long lr;
  __asm volatile ("mov %0, lr" : "=r" (lr));

	/* Tune memory timings *** FIXME: verify*/
  __REG (ESDCTL_ESDMISC_) = 0xcaaaf6d0;

  // *** FIXME: poll the SOC and board revisions
  // *** FIXME: disable watchdog?  Probably we don't need to do so.

  // Let's do the l2cc, aips, m4if, clock, and
  // debug setup here.

  // Scratch that.  Let's do anything that makes RAM faster but we can
  // relegate peripherap setups to the target_init code once we're all
  // in SDRAM.  So, l2cc, clocks, m4if?

  /* Drive GPIO1.23 high, LED perhaps? ...I don't believe so... */
  GPIO_SET           (MX51_PIN_UART3_TXD);
  GPIO_CONFIG_OUTPUT (MX51_PIN_UART3_TXD);
//  GPIOx_DR(1)   |= (1<<23);
//  GPIOx_GDIR(1) |= (1<<23);

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

  {                             /* Configure L2 cache */
    unsigned long l = (0
                       | ( 4<<0)  /* Data RAM latency 3 clocks */
                       | ( 3<<6)  /* Tag RAM latency 4 clocks */
                       | (1<<22)  /* Write allocate disable (ARM Errata 460075?) */
                       | (1<<23)  /* Write allocate combine disable */
                       | (1<<24)  /* Write allocate delay disable */
                       );
    if (ROM_SI_REV <= 0x10)
      l |= (1<<25);        	  /* Write combine disable (TO 2 and older) */
    __asm volatile ("mcr p15, 1, %0, c9, c0, 2" :: "r" (l));
  }
  {                             /* Enable L2 cache, data cache disabled in control register */
    unsigned long l;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 1\n\t"
		    "orr %0, %0, #(1<<1)\n\t"
		    "mcr p15, 0, %0, c1, c0, 1\n\t"
		    : "=&r" (l));
  }

  __asm volatile ("mov r0, #-1\t\n"
		  "bx %0" : : "r" (lr));
}



static void target_init_mprot (void)
{
  /* MPROTx set to non-bufferable, trusted for r/w and not forced to
     user-mode */
  __REG(PHYS_AIPS1 + 0) = 0x77777777;
  __REG(PHYS_AIPS1 + 4) = 0x77777777;
  __REG(PHYS_AIPS2 + 0) = 0x77777777;
  __REG(PHYS_AIPS2 + 4) = 0x77777777;
}

static void target_init_m4if (void)
{
  /* VPU and IPU given higher priority (0x4) IPU accesses with ID=0x1
     given highest priority (=0xA) */
  M4IF_FBPM0 = 0x203;
  __REG (PHYS_M4IF + 0x44) = 0;
  __REG (PHYS_M4IF + 0x9c) = 0x00120125;
  __REG (PHYS_M4IF + 0x48) = 0x001901A3;
}


static void target_init_gpio (void)
{
  /* initialize GPIO1 */

  GPIO_CLEAR         (MX51_PIN_GPIO1_5);
  GPIO_CONFIG_OUTPUT (MX51_PIN_GPIO1_5);
  GPIO_CONFIG_FUNC   (MX51_PIN_GPIO1_7, GPIO_PIN_FUNC_SION | 6);

  /* initialize GPIO2 */

  GPIO_CLEAR (MX51_PIN_EIM_D27);    /* GPIO2_9 */
  GPIO_CLEAR (MX51_PIN_EIM_A16);    /* GPIO2_10 */
  GPIO_CLEAR (MX51_PIN_EIM_A17);    /* GPIO2_11 */
  GPIO_SET   (MX51_PIN_EIM_A18);    /* GPIO2_12 */
  GPIO_SET   (MX51_PIN_EIM_A20);    /* GPIO2_14 */
  GPIO_CLEAR (MX51_PIN_EIM_A22);    /* GPIO2_16 */
  GPIO_SET   (MX51_PIN_EIM_A23);    /* GPIO2_17 */
  GPIO_SET   (MX51_PIN_EIM_OE);     /* GPIO2_24 */

  GPIO_CONFIG_OUTPUT (MX51_PIN_EIM_D27);    /* GPIO2_9 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_EIM_A16);    /* GPIO2_10 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_EIM_A17);    /* GPIO2_11 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_EIM_A18);    /* GPIO2_12 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_EIM_A20);    /* GPIO2_14 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_EIM_A22);    /* GPIO2_16 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_EIM_A23);    /* GPIO2_17 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_EIM_OE);     /* GPIO2_24 */
  GPIO_CONFIG_INPUT  (PIN_PWR_SW_REQ);      /* GPIO2_31 */

  /* Not sure what these do. */
  //  GPIO_CONFIG_INPUT (MX51_PIN_EIM_CS0);
//  GPIO_CONFIG_INPUT (MX51_PIN_EIM_CS1);
//  __REG(PHYS_IOMUXC + 0x980) = 1;

  /* Deduced from reading IOMUXC */
#if 0

   Differences between APEX bare and APEX booted after uboot *and* SPI
   flash works:

  MX51_PIN_EIM_CS0        0x0e0: 0x00000000 -> 0x00000001 /* GPIO2_25 */
  MX51_PIN_EIM_CS1        0x0e4: 0x00000000 -> 0x00000001 /* GPIO2_26 */
  MX51_PIN_CSI1_VSYNC     0x1c4: 0x00000000 -> 0x00000003 /* GPIO3_14 */
  MX51_PIN_CSI1_HSYNC     0x1c8: 0x00000000 -> 0x00000003 /* GPIO3_15 */

                          0x3e4: 0x00000016 -> 0x00000000
                          0x3e8: 0x00000016 -> 0x00000000
                          0x4ac: 0x000000e3 -> 0x000000e5
                          0x4b0: 0x000000e3 -> 0x000000e5
                          0x4b4: 0x000000e3 -> 0x000000e5
                          0x4cc: 0x000000e3 -> 0x000000e5
                          0x50c: 0x000020c3 -> 0x000020c5
                          0x510: 0x000020c3 -> 0x000020c5

  MX51_PIN_CSPI1_SS0      0x608: 0x00000085 -> 0x00000185 /* ignore */
  MX51_PIN_EIM_DA0        0x7a8: 0x000020d6 -> 0x000020d4
  MX51_PIN_EIM_DA4        0x7ac: 0x000020d6 -> 0x000020d4
  MX51_PIN_EIM_DA8        0x7b0: 0x000020d6 -> 0x000020c4
  MX51_PIN_EIM_DA12       0x7bc: 0x000020d6 -> 0x000020dc

                          0x83c: 0x00000002 -> 0x00000004
                          0x848: 0x00000002 -> 0x00000004
                          0x8a0: 0x00000200 -> 0x00000000
                          0x9b0: 0x00000001 -> 0x00000000
                          0x9b4: 0x00000001 -> 0x00000000
#endif

  /* initialize GPIO3 */

  IOMUXC_GPIO3_IPP_IND_G_IN_8_SELECT_INPUT  = 1;
  IOMUXC_GPIO3_IPP_IND_G_IN_5_SELECT_INPUT  = 1;
  IOMUXC_GPIO3_IPP_IND_G_IN_12_SELECT_INPUT = 1;
  IOMUXC_GPIO3_IPP_IND_G_IN_1_SELECT_INPUT  = 1;
  IOMUXC_GPIO3_IPP_IND_G_IN_6_SELECT_INPUT  = 1;

  GPIO_SET   (MX51_PIN_DI1_PIN13);
  GPIO_SET   (MX51_PIN_DISPB2_SER_DIN);
  GPIO_CLEAR (MX51_PIN_DISPB2_SER_DIO);
  GPIO_CLEAR (MX51_PIN_DISPB2_SER_RS);
  GPIO_CLEAR (MX51_PIN_CSI1_D8);
  GPIO_CLEAR (MX51_PIN_CSI1_D9);
  GPIO_CLEAR (MX51_PIN_NANDF_D15);

  GPIO_CONFIG_INPUT  (MX51_PIN_DI1_PIN11);      /* GPIO3_0 */
  GPIO_CONFIG_INPUT  (MX51_PIN_DI1_PIN12);      /* GPIO3_1 */
  GPIO_CONFIG_OUTPUT (PIN_WDG);                 /* GPIO3_2 */
  GPIO_CONFIG_INPUT  (PIN_AC_ADAP_INS);         /* GPIO3_3 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_DISPB2_SER_DIN); /* GPIO3_5 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_DISPB2_SER_DIO); /* GPIO3_6 */
  GPIO_CONFIG_INPUT  (MX51_PIN_DISPB2_SER_RS);  /* GPIO3_8 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_CSI1_D8);        /* GPIO3_12 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_CSI1_D9);        /* GPIO3_13 */
  GPIO_CONFIG_INPUT  (MX51_PIN_NANDF_CS1);      /* GPIO3_17 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_NANDF_D15);      /* GPIO3_25 */

  /* initialize GPIO group4 */

  GPIO_CLEAR (MX51_PIN_CSI2_D13);
  GPIO_CLEAR (MX51_PIN_CSI2_D18);
  GPIO_SET   (MX51_PIN_CSI2_D19);
  GPIO_CLEAR (PIN_SYS_PWROFF);
  GPIO_CLEAR (MX51_PIN_CSI2_HSYNC);

  GPIO_CONFIG_OUTPUT (MX51_PIN_CSI2_D13);	/* GPIO4_10 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_CSI2_D18);       /* GPIO4_11 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_CSI2_D19);	/* GPIO4_12 */
  GPIO_CONFIG_OUTPUT (PIN_SYS_PWROFF);          /* GPIO4_13 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_CSI2_HSYNC);     /* GPIO4_14 */
  GPIO_CONFIG_INPUT  (MX51_PIN_CSI2_PIXCLK);    /* GPIO4_15 */

  GPIO_CONFIG_FUNC (MX51_PIN_GPIO1_9, 4); /* CLKO from CCM */

  GPIO_CONFIG_PAD  (MX51_PIN_CSPI1_MOSI,
                    GPIO_PAD_SLEW_FAST | GPIO_PAD_DRIVE_HIGH
                    | GPIO_PAD_HYST_EN);
  GPIO_CONFIG_FUNC (MX51_PIN_CSPI1_MOSI, 0);

  GPIO_CONFIG_PAD  (MX51_PIN_CSPI1_MISO,
                    GPIO_PAD_SLEW_FAST | GPIO_PAD_DRIVE_HIGH
                    | GPIO_PAD_HYST_EN);
  GPIO_CONFIG_FUNC (MX51_PIN_CSPI1_MISO, 0);

  GPIO_CONFIG_PAD  (MX51_PIN_CSPI1_RDY, GPIO_PAD_PKE | GPIO_PAD_HYST_EN);
  GPIO_CONFIG_FUNC (MX51_PIN_CSPI1_RDY, 0);

  GPIO_CONFIG_PAD  (MX51_PIN_CSPI1_SCLK,
                    GPIO_PAD_SLEW_FAST | GPIO_PAD_DRIVE_HIGH
                    | GPIO_PAD_HYST_EN);
  GPIO_CONFIG_FUNC (MX51_PIN_CSPI1_SCLK, 0);

  /* workaround for ss0 */
  spi_select (NULL);

  /* PWR_SW_REQ# <= EIM_DTACK */
  GPIO_CONFIG_INPUT (PIN_PWR_SW_REQ);
  GPIO_CONFIG_PAD   (PIN_PWR_SW_REQ,
                     GPIO_PAD_PKE | GPIO_PAD_PU_100K);

  /* PMIC_IRQ <= GPIO1_6 */
  GPIO_CONFIG_INPUT (PIN_PMIC_IRQ);
  GPIO_CONFIG_FUNC  (PIN_PMIC_IRQ,
                    _PIN_GPIO_A(PIN_PMIC_IRQ) | GPIO_PIN_FUNC_SION);
  GPIO_CONFIG_PAD   (PIN_PMIC_IRQ,
                     GPIO_PAD_SLEW_SLOW
                     | GPIO_PAD_DRIVE_MED
                     | GPIO_PAD_PU_100K
                     | GPIO_PAD_HYST_EN
                     | GPIO_PAD_DRIVE_HIGHVOLT
                     );

  /* Output 'magic' value on GPIO1 and GPIO2 */
  GPIOx_DR(2) = 0x01025200;
  GPIOx_DR(1) = 0x00000020;
}

/** initializes the I2C IO lines.  This code should perhaps be part of
    a driver, though it may simply be necessary to boot the kernel. */

static void target_init_i2c (void)
{
  GPIO_CONFIG_FUNC (MX51_PIN_I2C1_CLK, 0 | GPIO_PIN_FUNC_SION);
  GPIO_CONFIG_PAD  (MX51_PIN_I2C1_CLK, 0x1e4);
  GPIO_CONFIG_FUNC (MX51_PIN_I2C1_DAT, 0 | GPIO_PIN_FUNC_SION);
  GPIO_CONFIG_PAD  (MX51_PIN_I2C1_DAT, 0x1a4);

  GPIO_CONFIG_FUNC (MX51_PIN_KEY_COL5, 3 | GPIO_PIN_FUNC_SION); /* I2C2 SDA */
  IOMUXC_I2C2_IPP_SDA_IN_SELECT_INPUT = 1;                      /* KEY_COL5 */
  GPIO_CONFIG_PAD  (MX51_PIN_KEY_COL5,
                    GPIO_PAD_SLEW_FAST | GPIO_PAD_OPEN_DRAIN
                    | GPIO_PAD_DRIVE_HIGH | GPIO_PAD_PU_100K
                    | GPIO_PAD_HYST_EN);

  GPIO_CONFIG_FUNC (MX51_PIN_KEY_COL4, 3 | GPIO_PIN_FUNC_SION);
  IOMUXC_I2C2_IPP_SCL_IN_SELECT_INPUT = 1;			/* KEY_COL4 */
  GPIO_CONFIG_PAD  (MX51_PIN_KEY_COL4,
                    GPIO_PAD_SLEW_FAST | GPIO_PAD_OPEN_DRAIN
                    | GPIO_PAD_DRIVE_HIGH | GPIO_PAD_PU_100K
                    | GPIO_PAD_HYST_EN);
}

static void target_init_esdhc (void)
{
  GPIO_CONFIG_INPUT (PIN_SDHC1_CD);
  GPIO_CONFIG_FUNC  (PIN_SDHC1_CD, 0 | GPIO_PIN_FUNC_SION);
  GPIO_CONFIG_PAD   (PIN_SDHC1_CD,
                     GPIO_PAD_DRIVE_HIGH | GPIO_PAD_PKE
                     | GPIO_PAD_HYST_EN | GPIO_PAD_PU_100K
                     | GPIO_PAD_SLEW_FAST);

  GPIO_CONFIG_FUNC (PIN_SDHC1_WP, 0 | GPIO_PIN_FUNC_SION);
  GPIO_CONFIG_PAD  (PIN_SDHC1_WP,
                    GPIO_PAD_DRIVE_HIGH
                    | GPIO_PAD_HYST_EN | GPIO_PAD_PU_100K
                    | GPIO_PAD_SLEW_FAST);

  GPIO_CONFIG_FUNC (MX51_PIN_SD1_CMD,   0 | GPIO_PIN_FUNC_SION);
  GPIO_CONFIG_FUNC (MX51_PIN_SD1_CLK,   0 | GPIO_PIN_FUNC_SION);
  GPIO_CONFIG_FUNC (MX51_PIN_SD1_DATA0, 0 | GPIO_PIN_FUNC_SION);
  GPIO_CONFIG_FUNC (MX51_PIN_SD1_DATA1, 0 | GPIO_PIN_FUNC_SION);
  GPIO_CONFIG_FUNC (MX51_PIN_SD1_DATA2, 0 | GPIO_PIN_FUNC_SION);
  GPIO_CONFIG_FUNC (MX51_PIN_SD1_DATA3, 0 | GPIO_PIN_FUNC_SION);

  GPIO_CONFIG_PAD  (MX51_PIN_SD1_CMD,
                    GPIO_PAD_DRIVE_MAX | GPIO_PAD_DRIVE_HIGHVOLT
                    | GPIO_PAD_HYST_EN | GPIO_PAD_PU_47K
                    | GPIO_PAD_PUE | GPIO_PAD_PKE | GPIO_PAD_SLEW_FAST
                    );
  GPIO_CONFIG_PAD  (MX51_PIN_SD1_CLK,
                    GPIO_PAD_DRIVE_MAX | GPIO_PAD_DRIVE_HIGHVOLT
                    | GPIO_PAD_PU_47K
                    | GPIO_PAD_PUE | GPIO_PAD_PKE | GPIO_PAD_SLEW_FAST);
  GPIO_CONFIG_PAD  (MX51_PIN_SD1_DATA0,
                    GPIO_PAD_DRIVE_MAX | GPIO_PAD_DRIVE_HIGHVOLT
                    | GPIO_PAD_HYST_EN | GPIO_PAD_PU_47K
                    | GPIO_PAD_PUE | GPIO_PAD_PKE | GPIO_PAD_SLEW_FAST);
  GPIO_CONFIG_PAD  (MX51_PIN_SD1_DATA1,
                    GPIO_PAD_DRIVE_MAX | GPIO_PAD_DRIVE_HIGHVOLT
                    | GPIO_PAD_HYST_EN | GPIO_PAD_PU_47K
                    | GPIO_PAD_PUE | GPIO_PAD_PKE | GPIO_PAD_SLEW_FAST);
  GPIO_CONFIG_PAD  (MX51_PIN_SD1_DATA2,
                    GPIO_PAD_DRIVE_MAX | GPIO_PAD_DRIVE_HIGHVOLT
                    | GPIO_PAD_HYST_EN | GPIO_PAD_PU_47K
                    | GPIO_PAD_PUE | GPIO_PAD_PKE | GPIO_PAD_SLEW_FAST);
  GPIO_CONFIG_PAD  (MX51_PIN_SD1_DATA3,
                    GPIO_PAD_DRIVE_MAX | GPIO_PAD_DRIVE_HIGHVOLT
                    | GPIO_PAD_HYST_EN | GPIO_PAD_PU_47K
                    | GPIO_PAD_PUE | GPIO_PAD_PKE | GPIO_PAD_SLEW_FAST);

  GPIO_CONFIG_INPUT (PIN_SDHC2_CD);
  GPIO_CONFIG_FUNC (PIN_SDHC2_CD, 6 | GPIO_PIN_FUNC_SION);
  GPIO_CONFIG_PAD  (PIN_SDHC2_CD,
                    GPIO_PAD_DRIVE_HIGH | GPIO_PAD_PKE
                    | GPIO_PAD_PU_100K
                    | GPIO_PAD_SLEW_FAST);

  GPIO_CONFIG_FUNC (PIN_SDHC2_WP, 6 | GPIO_PIN_FUNC_SION);
  GPIO_CONFIG_PAD  (PIN_SDHC2_WP,
                    GPIO_PAD_DRIVE_HIGH | GPIO_PAD_PKE
                    | GPIO_PAD_PU_100K
                    | GPIO_PAD_SLEW_FAST);
  GPIO_CONFIG_FUNC (MX51_PIN_SD2_CMD,   0);
  GPIO_CONFIG_FUNC (MX51_PIN_SD2_CLK,   0);
  GPIO_CONFIG_FUNC (MX51_PIN_SD2_DATA0, 0);
  GPIO_CONFIG_FUNC (MX51_PIN_SD2_DATA1, 0);
  GPIO_CONFIG_FUNC (MX51_PIN_SD2_DATA2, 0);
  GPIO_CONFIG_FUNC (MX51_PIN_SD2_DATA3, 0);

  GPIO_CONFIG_PAD  (MX51_PIN_SD2_CMD,
                    GPIO_PAD_DRIVE_MAX | GPIO_PAD_DRIVE_HIGHVOLT
                    | GPIO_PAD_HYST_EN | GPIO_PAD_PU_47K
                    | GPIO_PAD_PUE | GPIO_PAD_PKE | GPIO_PAD_SLEW_FAST);
  GPIO_CONFIG_PAD  (MX51_PIN_SD2_CLK,
                    GPIO_PAD_DRIVE_MAX | GPIO_PAD_DRIVE_HIGHVOLT
                    | GPIO_PAD_PU_47K
                    | GPIO_PAD_PUE | GPIO_PAD_PKE | GPIO_PAD_SLEW_FAST);
  GPIO_CONFIG_PAD  (MX51_PIN_SD2_DATA0,
                    GPIO_PAD_DRIVE_MAX | GPIO_PAD_DRIVE_HIGHVOLT
                    | GPIO_PAD_HYST_EN | GPIO_PAD_PU_47K
                    | GPIO_PAD_PUE | GPIO_PAD_PKE | GPIO_PAD_SLEW_FAST);
  GPIO_CONFIG_PAD  (MX51_PIN_SD2_DATA1,
                    GPIO_PAD_DRIVE_MAX | GPIO_PAD_DRIVE_HIGHVOLT
                    | GPIO_PAD_HYST_EN | GPIO_PAD_PU_47K
                    | GPIO_PAD_PUE | GPIO_PAD_PKE | GPIO_PAD_SLEW_FAST);
  GPIO_CONFIG_PAD  (MX51_PIN_SD2_DATA2,
                    GPIO_PAD_DRIVE_MAX | GPIO_PAD_DRIVE_HIGHVOLT
                    | GPIO_PAD_HYST_EN | GPIO_PAD_PU_47K
                    | GPIO_PAD_PUE | GPIO_PAD_PKE | GPIO_PAD_SLEW_FAST);
  GPIO_CONFIG_PAD  (MX51_PIN_SD2_DATA3,
                    GPIO_PAD_DRIVE_MAX | GPIO_PAD_DRIVE_HIGHVOLT
                    | GPIO_PAD_HYST_EN | GPIO_PAD_PD_100K
                    | GPIO_PAD_PUE | GPIO_PAD_PKE | GPIO_PAD_SLEW_FAST);
}

static void target_init_ata (void)
{
#define __(p) ({ GPIO_CONFIG_FUNC ((p), 1);                 \
                 GPIO_CONFIG_PAD  ((p), GPIO_PAD_DRIVE_HIGH \
                                      | GPIO_PAD_DRIVE_HIGHVOLT); })

  __(MX51_PIN_NANDF_ALE);
  __(MX51_PIN_NANDF_CS2);
  __(MX51_PIN_NANDF_CS3);
  __(MX51_PIN_NANDF_CS4);
  __(MX51_PIN_NANDF_CS5);
  __(MX51_PIN_NANDF_CS6);
  __(MX51_PIN_NANDF_RE_B);
  __(MX51_PIN_NANDF_WE_B);
  __(MX51_PIN_NANDF_CLE);
  __(MX51_PIN_NANDF_RB0);
  __(MX51_PIN_NANDF_WP_B);
  __(MX51_PIN_GPIO_NAND);
  __(MX51_PIN_NANDF_RB1);
  __(MX51_PIN_NANDF_D0);
  __(MX51_PIN_NANDF_D1);
  __(MX51_PIN_NANDF_D2);
  __(MX51_PIN_NANDF_D3);
  __(MX51_PIN_NANDF_D4);
  __(MX51_PIN_NANDF_D5);
  __(MX51_PIN_NANDF_D6);
  __(MX51_PIN_NANDF_D7);
  __(MX51_PIN_NANDF_D8);
  __(MX51_PIN_NANDF_D9);
  __(MX51_PIN_NANDF_D10);
  __(MX51_PIN_NANDF_D11);
  __(MX51_PIN_NANDF_D12);
  __(MX51_PIN_NANDF_D13);
  __(MX51_PIN_NANDF_D14);
  __(MX51_PIN_NANDF_D15);

#undef __
}


/** performs remaining hardware initialization that didn't have to be
    performed during the bootstrap phase and isn't done in a driver. */

static void target_init (void)
{
  target_init_mprot ();
  target_init_m4if ();

  CCM_CCDR = 0x00060000;        /* Mask IPU clock handshake */

  target_init_gpio ();

  board_revision ();

  /* Select a device for SPI1  */
//  spi1_select (SPI1_SS_FLASH);

  target_init_i2c ();
  target_init_esdhc ();
  target_init_ata ();

  /* Clear power-down watchdog */
  WDOGx_WMCR(1) = 0;
  WDOGx_WMCR(2) = 0;

  /* ***FIXME: should enable a system managed timeout at this point if
     we want to keep using a watchdog timer at all. */
}


static void target_release (void)
{
  /* workaround for ENGcm09397 - Fix SPI NOR reset issue*/
#if 0
  /* de-select SS0 of instance: eCSPI1 */
  __REG (PHYS_IOMUXC + 0x218) = 3;
//  writel(0x3, IOMUXC_BASE_ADDR + 0x218);
  __REG (PHYS_IOMUXC + 0x608) = 0x85;
//  writel(0x85, IOMUXC_BASE_ADDR + 0x608);
  /* de-select SS1 of instance: ecspi1 */
  __REG (PHYS_IOMUXC + 0x21c) = 3;
//  writel(0x3, IOMUXC_BASE_ADDR + 0x21C);
  __REG (PHYS_IOMUXC + 0x60c) = 0x85;
//  writel(0x85, IOMUXC_BASE_ADDR + 0x60C);
#endif

  spi_select (NULL);
}

static __service_0 struct service_d mx3x_target_service = {
  .init    = target_init,
  .release = target_release,
};
