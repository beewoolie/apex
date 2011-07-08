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

   NANDF_RB3	NANDF_CS1	NANDF_CS0	Board Revision
   ---------	---------	---------	--------------
       1	    1		    1		     1.1
       1	    1		    0		     1.2
       1	    0		    1		     1.3
       1	    0		    0		     1.4


   Initialization Sequence
   -----------------------

   arch_cpu_init,          / * basic arch cpu dependent setup * /
     icache_enable
     dcache_enable
//     l2_cache_disable
     l2_cache_enable  // 0, c1, c1, 1 |= 2

#endif
x  board_init,             / * basic board dependent setup * /
#if defined(CONFIG_USE_IRQ)
   interrupt_init,         / * set up exceptions * /
#endif
   timer_init,             / * initialize timer * /
   env_init,               / * initialize environment * /
   init_baudrate,          / * initialze baudrate settings * /
   serial_init,            / * serial communications setup * /
   console_init_f,         / * stage 1 init of console * /
   display_banner,         / * say that we are here * /
#if defined(CONFIG_DISPLAY_CPUINFO)
   print_cpuinfo,          / * display cpu info (and speed) * /
#endif
#if defined(CONFIG_DISPLAY_BOARDINFO)
   checkboard,             / * display board info * /
#endif
#if defined(CONFIG_HARD_I2C) || defined(CONFIG_SOFT_I2C)
   init_func_i2c,
#endif
   dram_init,              / * configure available RAM banks * /
#if defined(CONFIG_CMD_PCI) || defined (CONFIG_PCI)
   arm_pci_init,
#endif
   display_dram_config,

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

int board_id;
//static int spi1_slave = SPI1_SS_FLASH;

#define BOARD_ID_MAJOR() (1)
#define BOARD_ID_MINOR() ((board_id ^ 0x7) + 1)
#define ESDHC_INTERFACE()\
  ((BOARD_ID_MAJOR() == 1 && BOARD_ID_MINOR() == 1) ? 1 : 0)

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


/** activate the SS line for the given slave on the SPI1 port.
    ***FIXME: need to make sure that this call is idempotent.  We may
    ***need to keep track of the currently selected device. */

void spi1_select (int slave)
{
  switch (slave) {

  case 0:
    GPIO_CONFIG_INPUT (MX51_PIN_CSPI1_SS1);
    GPIO_CONFIG_PAD   (MX51_PIN_CSPI1_SS1,
                       GPIO_PAD_PKE | GPIO_PAD_DRIVE_HIGH | GPIO_PAD_SLEW_FAST);
    GPIO_CONFIG_FUNC  (MX51_PIN_CSPI1_SS0, 0);
    GPIO_CONFIG_PAD   (MX51_PIN_CSPI1_SS0,
                       GPIO_PAD_HYST_EN | GPIO_PAD_PKE
                       | GPIO_PAD_DRIVE_HIGH | GPIO_PAD_SLEW_FAST);
    break;

  case 1:
    GPIO_CONFIG_INPUT (MX51_PIN_CSPI1_SS0);
    GPIO_CONFIG_PAD   (MX51_PIN_CSPI1_SS0,
                       GPIO_PAD_PKE | GPIO_PAD_DRIVE_HIGH | GPIO_PAD_SLEW_FAST);
    GPIO_CONFIG_FUNC  (MX51_PIN_CSPI1_SS1, 0);
    GPIO_CONFIG_PAD   (MX51_PIN_CSPI1_SS1,
                       GPIO_PAD_HYST_EN
                       | GPIO_PAD_DRIVE_HIGH | GPIO_PAD_SLEW_FAST);
    break;
  }
}


/** bootstrap iniitialization function.  The iMX5 boot ROM performs
    the SDRAM setup before copying the loader to SDRAM.  The return
    value is always true because of the loader is always already in
    SDRAM. */

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

  /* Drive GPIO1.23 high, LED perhaps? */
  GPIO_SET           (MX51_PIN_UART3_TXD);
  GPIO_CONFIG_OUTPUT (MX51_PIN_UART3_TXD);
//  GPIOX_DR(1)   |= (1<<23);
//  GPIOX_GDIR(1) |= (1<<23);

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

  __asm volatile ("mov r0, #-1\t\n"
		  "bx %0" : : "r" (lr));
}


/** initializes the I2C IO lines.  This code should perhaps be part of
    a driver, though it may simply be necessary to boot the kernel. */

static void target_init_i2c (void)
{
  GPIO_CONFIG_FUNC (MX51_PIN_I2C1_CLK, 0 | GPIO_PIN_FUNC_SION);
  GPIO_CONFIG_PAD  (MX51_PIN_I2C1_CLK, 0x1e4);
  GPIO_CONFIG_FUNC (MX51_PIN_I2C1_DAT, 0 | GPIO_PIN_FUNC_SION);
  GPIO_CONFIG_PAD  (MX51_PIN_I2C1_CLK, 0x1e4);

  GPIO_CONFIG_FUNC (MX51_PIN_KEY_COL5, 3 | GPIO_PIN_FUNC_SION); /* I2C2 SDA */
  IOMUXC_I2C2_IPP_SDA_IN_SELECT_INPUT = 1;                      /* KEY_COL5 */
  GPIO_CONFIG_PAD  (MX51_PIN_KEY_COL5,
                    GPIO_PAD_SLEW_FAST | GPIO_PAD_OPEN_DRAIN
                    | GPIO_PAD_DRIVE_HIGH | GPIO_PAD_PU_100K
                    | GPIO_PAD_HYST_EN);

  GPIO_CONFIG_FUNC (MX51_PIN_KEY_COL4, 4 | GPIO_PIN_FUNC_SION);
  IOMUXC_I2C2_IPP_SCL_IN_SELECT_INPUT = 1;			/* KEY_COL4 */
  GPIO_CONFIG_PAD  (MX51_PIN_KEY_COL4,
                    GPIO_PAD_SLEW_FAST | GPIO_PAD_OPEN_DRAIN
                    | GPIO_PAD_DRIVE_HIGH | GPIO_PAD_PU_100K
                    | GPIO_PAD_HYST_EN);
}


/** performs remaining hardware initialization that didn't have to be
   performed during the bootstrap phase and isn't done in a driver. */

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

  CCM_CCDR = 0x00060000;        /* Mask IPU clock handshake */

  /* Disable IPU and HSC dividers */
  /* Clock Controller Module (CCM) page6.6 */

//  writel(0x60000, CCM_BASE_ADDR + CLKCTL_CCDR);

  /* initialize GPIO1 */

  GPIO_CLEAR         (MX51_PIN_GPIO1_5);
  GPIO_CONFIG_OUTPUT (MX51_PIN_GPIO1_5);
  GPIO_CONFIG_INPUT  (PIN_PMIC_IRQ);

//  writel(0x00000020, 0x73f84004);
//  writel(0x00000000, 0x73f84000);
//  writel(0x00, IOMUXC_BASE_ADDR + 0x3dc);
//  writel(0x00, IOMUXC_BASE_ADDR + 0x3e0);


  /* initialize GPIO2 */

  GPIO_CLEAR (MX51_PIN_EIM_D27);	/* GPIO2_9 */
  GPIO_CLEAR (MX51_PIN_EIM_A16);    /* GPIO2_10 */
  GPIO_CLEAR (MX51_PIN_EIM_A17);    /* GPIO2_11 */
  GPIO_SET   (MX51_PIN_EIM_A18);    /* GPIO2_12 */
  GPIO_SET   (MX51_PIN_EIM_A20);    /* GPIO2_14 */
  GPIO_CLEAR (MX51_PIN_EIM_A22);    /* GPIO2_16 */
  GPIO_SET   (MX51_PIN_EIM_A23);    /* GPIO2_17 */
  GPIO_SET   (MX51_PIN_EIM_OE);     /* GPIO2_24 */

  GPIO_CONFIG_OUTPUT (MX51_PIN_EIM_D27);	/* GPIO2_9 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_EIM_A16);    /* GPIO2_10 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_EIM_A17);    /* GPIO2_11 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_EIM_A18);    /* GPIO2_12 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_EIM_A20);    /* GPIO2_14 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_EIM_A22);    /* GPIO2_16 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_EIM_A23);    /* GPIO2_17 */
  GPIO_CONFIG_OUTPUT (MX51_PIN_EIM_OE);     /* GPIO2_24 */
  GPIO_CONFIG_INPUT  (PIN_PWR_SW_REQ);      /* GPIO2_31 */

//  writel(0x01035e00, 0x73f88004); /* DIR   9,10,11,12,14,16,17,24 */
//  writel(0x01025000, 0x73f88000); /* DATA          12,14,   17,24 */
//  writel(0x01, IOMUXC_BASE_ADDR + 0x9c);
//  writel(0x01, IOMUXC_BASE_ADDR + 0xa0);
//  writel(0x01, IOMUXC_BASE_ADDR + 0xa4);
//  writel(0x01, IOMUXC_BASE_ADDR + 0xac);
//  writel(0x01, IOMUXC_BASE_ADDR + 0xdc);
//  writel(0x01, IOMUXC_BASE_ADDR + 0xb4);
//  writel(0x01, IOMUXC_BASE_ADDR + 0xb8);
//  writel(0x01, IOMUXC_BASE_ADDR + 0xf8);
//  writel(0x01, IOMUXC_BASE_ADDR + 0x88);

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

//  writel(0x02003064, 0x73f8c004); /* DIR  2,5,6,12,13,25  */
//  writel(0x00000024, 0x73f8c000); /* DATA 2,5 */
//  writel(0x01, IOMUXC_BASE_ADDR + 0x994);
//  writel(0x04, IOMUXC_BASE_ADDR + 0x2c8); /* 3.8 */

//  writel(0x03, IOMUXC_BASE_ADDR + 0x134); /* 3.17 */
//  writel(0x04, IOMUXC_BASE_ADDR + 0x2a8); /* 3.0 */
//  writel(0x04, IOMUXC_BASE_ADDR + 0x2b0); /* 3.2 */
//  writel(0x04, IOMUXC_BASE_ADDR + 0x2b4); /* 3.3 */

//  writel(0x01, IOMUXC_BASE_ADDR + 0x988);
//  writel(0x04, IOMUXC_BASE_ADDR + 0x2bc); /* 3.5 */

//  writel(0x01, IOMUXC_BASE_ADDR + 0x998);
//  writel(0x03, IOMUXC_BASE_ADDR + 0x194); /* 3.12 */
//  writel(0x03, IOMUXC_BASE_ADDR + 0x198); /* 3.13 */
//  writel(0x03, IOMUXC_BASE_ADDR + 0x154); /* 3.25 */

//  writel(0x01, IOMUXC_BASE_ADDR + 0x978);
//  writel(0x04, IOMUXC_BASE_ADDR + 0x2ac); /* 3.1 */

//  writel(0x01, IOMUXC_BASE_ADDR + 0x98c);
//  writel(0x04, IOMUXC_BASE_ADDR + 0x2c0); /* 3.6 */

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

//  writel(0x00007c00, 0x73f90004); /* DIR  10,11,12,13,14 */
//  writel(0x00001000, 0x73f90000); /* DATA       12 */
//  writel(0x03, IOMUXC_BASE_ADDR + 0x1d0); /* 4.10 */
//  writel(0x03, IOMUXC_BASE_ADDR + 0x1e4); /* 4.11 */
//  writel(0x03, IOMUXC_BASE_ADDR + 0x1e8); /* 4.12 */
//  writel(0x03, IOMUXC_BASE_ADDR + 0x1ec); /* 4.13 */
//  writel(0x03, IOMUXC_BASE_ADDR + 0x1f0); /* 4.14 */
//  writel(0x03, IOMUXC_BASE_ADDR + 0x1f4); /* 4.15 */

  GPIO_CONFIG_PAD (PIN_PWR_SW_REQ, GPIO_PAD_PKE | GPIO_PAD_PU_100K);
  GPIO_CONFIG_PAD (PIN_PMIC_IRQ,
                   GPIO_PAD_SLEW_SLOW
                   | GPIO_PAD_HYST_EN
                   | GPIO_PAD_DRIVE_MED      /* ***FIXME: input? */
                   | GPIO_PAD_DRIVE_HIGHVOLT /* ***FIXME: input? */
                   );

  /* Partially claimed pin */
//  mxc_request_iomux(MX51_PIN_GPIO1_9, IOMUX_CONFIG_ALT4);

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

  /* Output 'magic' value on GPIO1 and GPIO2 */
  GPIOX_DR(2) = 0x01025200;
  GPIOX_DR(1) = 0x00000020;

  /* Read board revision */

  board_id = 0;

  GPIO_SET           (MX51_PIN_NANDF_CS0);
  GPIO_CONFIG_OUTPUT (MX51_PIN_NANDF_CS0); /* Charge capacitance, rev1.1 */

  GPIO_CONFIG_PAD    (MX51_PIN_NANDF_CS0, GPIO_PAD_PU_100K);
  GPIO_CONFIG_INPUT  (MX51_PIN_NANDF_CS0);
  board_id |= GPIO_VALUE (MX51_PIN_NANDF_CS0) ? (1<<0) : 0;

  GPIO_CONFIG_PAD    (MX51_PIN_NANDF_CS1, GPIO_PAD_PU_100K);
  GPIO_CONFIG_INPUT  (MX51_PIN_NANDF_CS1);
  board_id |= GPIO_VALUE (MX51_PIN_NANDF_CS1) ? (1<<1) : 0;

  GPIO_CONFIG_PAD    (MX51_PIN_NANDF_RB3, GPIO_PAD_PU_100K);
  GPIO_CONFIG_INPUT  (MX51_PIN_NANDF_RB3);
  board_id |= GPIO_VALUE (MX51_PIN_NANDF_RB3) ? (1<<2) : 0;

  /* Select a device for SPI1  */
  spi1_select (SPI1_SS_FLASH);

  target_init_i2c ();
}


static void target_release (void)
{
}

static __service_0 struct service_d mx3x_target_service = {
  .init    = target_init,
  .release = target_release,
};
