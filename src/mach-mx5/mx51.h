/** @file mx51.h

   written by Marc Singer
   30 Jun 2011

   Copyright (C) 2011 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (MX51_H_INCLUDED)
#    define   MX51_H_INCLUDED

/* ----- Includes */

#include <asm/reg.h>

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#define PHYS_BOOT_ROM		(0x00000000)	/* 36K */
#define PHYS_SCC_RAM		(0x1ffe0000)	/* 128k */
#define PHYS_GRAPHICS_RAM	(0x20000000)	/* 128K */
#define PHYS_GPU		(0x30000000)
#define PHYS_IPUEX		(0x40000000)
#define PHYS_ESDHC1		(0x70004000)
#define PHYS_ESDHC2		(0x70008000)
#define PHYS_UART3		(0x7000C000)
#define PHYS_ECSPI1		(0x70010000)
#define PHYS_SSI2		(0x70040000)
#define PHYS_ESDHC3		(0x70020000)
#define PHYS_ESDHC4		(0x70024000)
#define PHYS_SPDIF		(0x70028000)
#define PHYS_PATA_UDMA		(0x70030000)
#define PHYS_SLM		(0x70034000)
#define PHYS_HSI2C		(0x70038000)
#define PHYS_SPBA		(0x7003c000)
#define PHYS_AIPS1		(0x73f00000)
//#define PHYS_USBOH3             (0x73f80000)
#define PHYS_GPIO1              (0x73f84000)
#define PHYS_GPIO2              (0x73f88000)
#define PHYS_GPIO3              (0x73f8c000)
#define PHYS_GPIO4              (0x73f90000)
#define PHYS_GPIOX(i)		(0x73840000 + ((i) - 1)*0x4000)
#define PHYS_KPP                (0x73f94000)
#define PHYS_WDOG1              (0x73f98000)
#define PHYS_WDOG2              (0x73f9c000)
#define PHYS_GPT                (0x73fa0000)
#define PHYS_SRTC               (0x73fa4000)
#define PHYS_IOMUXC             (0x73fa8000)
#define PHYS_EPIT1              (0x73fac000)
#define PHYS_EPIT2              (0x73fb0000)
#define PHYS_PWM1               (0x73fb4000)
#define PHYS_PWM2               (0x73fb8000)
#define PHYS_UART1              (0x73fbc000)
#define PHYS_UART2              (0x73fc0000)
//#define PHYS_USBOH3             (0x73fc4000)
#define PHYS_SRC                (0x73fd0000)
#define PHYS_CCM                (0x73fd4000)
#define PHYS_GPC                (0x73fd8000)
#define PHYS_AIPS2		(0x83f00000)
#define PHYS_DPLLIP1            (0x83f80000)
#define PHYS_DPLLIP2            (0x83f84000)
#define PHYS_DPLLIP3            (0x83f88000)
#define PHYS_DPLLIPX(i)         (0x83f80000 + ((i) - 1)*0x4000)
#define PHYS_AHBMAX             (0x83f94000)
#define PHYS_IIM                (0x83f98000)
#define PHYS_CSU                (0x83f9c000)
#define PHYS_TIGERP_PLATFORM_NE_32K_256K (0x83fa0000)
#define PHYS_ARM_PLATFORM	(0x83fa0000) /* Lacking documentation */
#define PHYS_OWIRE		(0x83fa4000)
#define PHYS_FIRI               (0x83fa8000)
#define PHYS_eCSPI2             (0x83fac000)
#define PHYS_SDMA               (0x83fb0000)
#define PHYS_SCC                (0x83fb4000)
#define PHYS_ROMCP              (0x83fb8000)
#define PHYS_RTIC               (0x83fbc000)
#define PHYS_CSPI               (0x83fc0000)
#define PHYS_I2C2               (0x83fc4000)
#define PHYS_I2C1               (0x83fc8000)
#define PHYS_SSI1               (0x83fcc000)
#define PHYS_AUDMUX             (0x83fd0000)
#define PHYS_EMI1               (0x83fd8000)
#define PHYS_M4IF		(0x83fd8000)
#define PHYS_ESDCTL		(0x83fd9000)
#define PHYS_PATA_PIO           (0x83fe0000)
#define PHYS_SIM                (0x83fe4000)
#define PHYS_SSI3               (0x83fe8000)
#define PHYS_FEC                (0x83fec000)
#define PHYS_TVE                (0x83ff0000)
#define PHYS_VPU                (0x83ff4000)
#define PHYS_SAHARA             (0x83ff8000)

#define PHYS_CSD0		(0x90000000)
#define PHYS_CSD1		(0xa0000000)
#define PHYS_CS0		(0xb0000000)
#define PHYS_CS1		(0xb8000000)
#define PHYS_CS2		(0xc0000000)
#define PHYS_CS3		(0xc8000000)
#define PHYS_CS4		(0xcc000000)
#define PHYS_CS5		(0xce000000)
#define PHYS_NAND_BUFFER	(0xcfff0000)
#define PHYS_GPU2S		(0xd0000000)
#define PHYS_TZIC		(0xe0000000)

#define BOOT_ROM_SI_REV		__REG (PHYS_BOOT_ROM + 0x48)

// page 226
#define CCM_CCR			__REG (PHYS_CCM + 0x00)
#define CCM_CCDR		__REG (PHYS_CCM + 0x04)
#define CCM_CSR			__REG (PHYS_CCM + 0x08)
#define CCM_CCSR		__REG (PHYS_CCM + 0x0c)
#define CCM_CACRR		__REG (PHYS_CCM + 0x10)
#define CCM_CBCDR		__REG (PHYS_CCM + 0x14)
#define CCM_CBCMR		__REG (PHYS_CCM + 0x18)
#define CCM_CSCMR1		__REG (PHYS_CCM + 0x1c)
#define CCM_CSCMR2		__REG (PHYS_CCM + 0x20)
#define CCM_CSCDR1		__REG (PHYS_CCM + 0x24)
#define CCM_CDHIPR		__REG (PHYS_CCM + 0x48)
#define CCM_CCOSR		__REG (PHYS_CCM + 0x60)
#define CCM_CCGR0		__REG (PHYS_CCM + 0x68)
#define CCM_CCGR1		__REG (PHYS_CCM + 0x6c)
#define CCM_CCGR2		__REG (PHYS_CCM + 0x70)
#define CCM_CCGR3		__REG (PHYS_CCM + 0x74)
#define CCM_CCGR4		__REG (PHYS_CCM + 0x78)
#define CCM_CCGR5		__REG (PHYS_CCM + 0x7c)
#define CCM_CCGR6		__REG (PHYS_CCM + 0x80)

#define CCM_CCGR_OFF		(0x00)
#define CCM_CCGR_RUN		(0x01)
#define CCM_CCGR_RUNWAIT	(0x02)
#define CCM_CCGR_ALL		(0x03)
#define CCM_CCGR_MASK           (0x03)
#define CCM_CCGR1_UART1_IPG_CLK_SH (3*2)
#define CCM_CCGR1_UART1_PER_CLK_SH (4*2)
#define CCM_CCGR2_GPT_IPG_CLK_SH   (9*2)

#define ARM_PLATFORM_ICGC	__REG (PHYS_ARM_PLATFORM + 0x14)

#define DPLLX_DP_CTL(i)		__REG (PHYS_DPLLIPX(i) + 0x00)
#define DPLLX_DP_CONFIG(i)	__REG (PHYS_DPLLIPX(i) + 0x04)
#define DPLLX_DP_OP(i)		__REG (PHYS_DPLLIPX(i) + 0x08)
#define DPLLX_DP_MFD(i)		__REG (PHYS_DPLLIPX(i) + 0x0c)
#define DPLLX_DP_MFN(i)		__REG (PHYS_DPLLIPX(i) + 0x10)
#define DPLLX_DP_HFS_OP(i)	__REG (PHYS_DPLLIPX(i) + 0x1c)
#define DPLLX_DP_HFS_MFD(i)	__REG (PHYS_DPLLIPX(i) + 0x20)
#define DPLLX_DP_HFS_MFN(i)	__REG (PHYS_DPLLIPX(i) + 0x24)

#define GPT_CR			__REG (PHYS_GPT + 0x00)
#define GPT_PR			__REG (PHYS_GPT + 0x04)
#define GPT_SR			__REG (PHYS_GPT + 0x08)
#define GPT_IR			__REG (PHYS_GPT + 0x0c)
#define GPT_0CR1		__REG (PHYS_GPT + 0x10)
#define GPT_0CR2		__REG (PHYS_GPT + 0x14)
#define GPT_0CR3		__REG (PHYS_GPT + 0x18)
#define GPT_ICR1		__REG (PHYS_GPT + 0x1c)
#define GPT_ICR2		__REG (PHYS_GPT + 0x20)
#define GPT_CNT			__REG (PHYS_GPT + 0x24)

#define GPT_CR_EN		(1<<0)
#define GPT_CR_CLKSRC_SH	(6)
#define GPT_CR_CLKSRC_MSK	(0x7<<6)
#define GPT_CR_CLKSRC_32K	(0x4<<6)
#define GPT_CR_CLKSRC_HIGH	(0x2<<6)
#define GPT_CR_CLKSRC_LOW	(0x1<<6)
#define GPT_CR_FREERUN		(1<<9)

#define GPT_SR_ROV		(1<<5) /* Roll-over */

#define SDRAM_BANK0_PHYS	(PHYS_CSD0)
#define SDRAM_BANK1_PHYS	(PHYS_CSD1)
#define SDRAM_BANK_SIZE		(0x10000000)

#define ESDCTL_ESDCTL0_		(PHYS_ESDCTL + 0x00)
#define ESDCTL_ESDCFG0_		(PHYS_ESDCTL + 0x04)
#define ESDCTL_ESDCTL1_		(PHYS_ESDCTL + 0x08)
#define ESDCTL_ESDCFG1_		(PHYS_ESDCTL + 0x0c)
#define ESDCTL_ESDMISC_		(PHYS_ESDCTL + 0x10)
#define ESDCTL_ESDSCR_		(PHYS_ESDCTL + 0x14)
#define ESDCTL_ESDCDLY1_	(PHYS_ESDCTL + 0x20)
#define ESDCTL_ESDCDLY2_	(PHYS_ESDCTL + 0x24)
#define ESDCTL_ESDCDLY3_	(PHYS_ESDCTL + 0x28)
#define ESDCTL_ESDCDLY4_	(PHYS_ESDCTL + 0x2c)
#define ESDCTL_ESDCDLY5_	(PHYS_ESDCTL + 0x30)
#define ESDCTL_ESDGPR_		(PHYS_ESDCTL + 0x34)
#define ESDCTL_ESDPRCT0_	(PHYS_ESDCTL + 0x38)
#define ESDCTL_ESDPRCT1_	(PHYS_ESDCTL + 0x3c)


/* Assuming 24MHz input clock with doubler ON */
/*                            MFI         PDF */
#define DPLL_OP_850       ((8 << 4) + ((1 - 1)  << 0))
#define DPLL_MFD_850      (48 - 1)
#define DPLL_MFN_850      41

#define DPLL_OP_800       ((8 << 4) + ((1 - 1)  << 0))
#define DPLL_MFD_800      (3 - 1)
#define DPLL_MFN_800      1

#define DPLL_OP_700       ((7 << 4) + ((1 - 1)  << 0))
#define DPLL_MFD_700      (24 - 1)
#define DPLL_MFN_700      7

#define DPLL_OP_665       ((6 << 4) + ((1 - 1)  << 0))
#define DPLL_MFD_665      (96 - 1)
#define DPLL_MFN_665      89

#define DPLL_OP_532       ((5 << 4) + ((1 - 1)  << 0))
#define DPLL_MFD_532      (24 - 1)
#define DPLL_MFN_532      13

#define DPLL_OP_400       ((8 << 4) + ((2 - 1)  << 0))
#define DPLL_MFD_400      (3 - 1)
#define DPLL_MFN_400      1

#define DPLL_OP_216       ((6 << 4) + ((3 - 1)  << 0))
#define DPLL_MFD_216      (4 - 1)
#define DPLL_MFN_216      3

#define GPIOX_DR(i)	__REG (PHYS_GPIOX(i) + 0x00)
#define GPIOX_GDIR(i)	__REG (PHYS_GPIOX(i) + 0x04)
#define GPIOX_PSR(i)	__REG (PHYS_GPIOX(i) + 0x08)


#define M4IF_FBPM0	__REG (PHYS_M4IF + 0x40)
#define M4IF_FIDBP	__REG (PHYS_M4IF + 0x48)

#endif  /* MX51_H_INCLUDED */
