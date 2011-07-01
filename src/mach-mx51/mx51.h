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
#define PHYS_USBOH3             (0x73f80000)
#define PHYS_GPIO1              (0x73f84000)
#define PHYS_GPIO2              (0x73f88000)
#define PHYS_GPIO3              (0x73f8c000)
#define PHYS_GPIO4              (0x73f90000)
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
#define PHYS_USBOH3             (0x73fc4000)
#define PHYS_SRC                (0x73fd0000)
#define PHYS_CCM                (0x73fd4000)
#define PHYS_GPC                (0x73fd8000)
#define PHYS_DPLLIP1            (0x83f80000)
#define PHYS_DPLLIP2            (0x83f84000)
#define PHYS_DPLLIP3            (0x83f88000)
#define PHYS_AHBMAX             (0x83f94000)
#define PHYS_IIM                (0x83f98000)
#define PHYS_CSU                (0x83f9c000)
#define PHYS_TIGERP_PLATFORM_NE_32K_256K (0x83fa0000)
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

#define SDRAM_BANK0_PHYS	(PHYS_CSD0)
#define SDRAM_BANK1_PHYS	(PHYS_CSD1)
#define SDRAM_BANK_SIZE		(0x10000000)


#endif  /* MX51_H_INCLUDED */
