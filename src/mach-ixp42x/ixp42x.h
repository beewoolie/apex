/* ixp42x.h
     $Id$

   written by Marc Singer
   14 Jan 2005

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

#if !defined (__IXP42X_H__)
#    define   __IXP42X_H__

/* ----- Includes */

#include <asm/reg.h>

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

	/* Registers */

#define SDR_PHYS	(0xcc000000)
#define SDR_CONFIG	__REG(SDR_PHYS + 0x00)
#define SDR_REFRESH	__REG(SDR_PHYS + 0x04)	/* Refresh register */
#define SDR_IR		__REG(SDR_PHYS + 0x08)	/* Instruction register */

#define SDR_IR_MODE_CAS2	0x0
#define SDR_IR_MODE_CAS3	0x1
#define SDR_IR_PRECHARGE_ALL	0x2
#define SDR_IR_NOP		0x3
#define SDR_IR_AUTO_REFRESH	0x4
#define SDR_IR_BURST_TERMINATE	0x5
#define SDR_IR_NORMAL		0x6 /* *** undocumented? *** */

#define SDR_CONFIG_64MIB	(1<<5) /* 64 Mib chip enable */
#define SDR_CONFIG_CAS3		(1<<3) /* Enable CAS3, CAS2 default */
#define SDR_CONFIG_2x8Mx16       0
#define SDR_CONFIG_4x8Mx16       1
#define SDR_CONFIG_2x16Mx16      2
#define SDR_CONFIG_4x16Mx16      3
#define SDR_CONFIG_2x32Mx16      4
#define SDR_CONFIG_4x32Mx16      5

#define EXP_PHYS	(0xc4000000)
#define EXP_TIMING_CS0	__REG(EXP_PHYS + 0x00)
#define EXP_TIMING_CS1	__REG(EXP_PHYS + 0x04)
#define EXP_TIMING_CS2	__REG(EXP_PHYS + 0x08)
#define EXP_TIMING_CS3	__REG(EXP_PHYS + 0x0c)
#define EXP_TIMING_CS4	__REG(EXP_PHYS + 0x10)
#define EXP_TIMING_CS5	__REG(EXP_PHYS + 0x14)
#define EXP_TIMING_CS6	__REG(EXP_PHYS + 0x18)
#define EXP_TIMING_CS7	__REG(EXP_PHYS + 0x1c)
#define EXP_CNFG0	__REG(EXP_PHYS + 0x20)
#define EXP_CNFG1	__REG(EXP_PHYS + 0x24)

#define EXP_CS_EN	       (1<<31)	/* Enable chip select */
#define EXP_BYTE_EN	       (1<<0)	/* Eight-bit device */
#define EXP_WR_EN	       (1<<1)	/* Enable writes */
#define EXP_SPLIT_EN	       (1<<3)	/* Enable split transfers */
#define EXP_MUX_EN	       (1<<4)	/* Multiplexed A/D */
#define EXP_HRDY_POL	       (1<<4)	/* HRDY polarity */
#define EXP_BYTE_RD16	       (1<<4)	/* Byte access to 16 bit device */
#define EXP_CNFG_SHIFT	       (10)	/* Device addressing range */
#define EXP_CNFG_MASK	       (0xf)
#define EXP_CYC_TYPE_SHIFT     (14)	/* Cycle type */
#define EXP_CYC_TYPE_MASK      (0x3)
#define EXP_T1_SHIFT	       (28)	/* Address timing */
#define EXP_T1_MASK	       (0x3)
#define EXP_T2_SHIFT	       (26)	/* Setup/CS timing */
#define EXP_T2_MASK	       (0x3)
#define EXP_T3_SHIFT	       (22)	/* Strobe timing */
#define EXP_T3_MASK	       (0xf)
#define EXP_T4_SHIFT	       (20)	/* Hold timing */
#define EXP_T4_MASK	       (0x3)
#define EXP_T5_SHIFT	       (20)	/* Recovery timing */
#define EXP_T5_MASK	       (0xf)

#define EXP_CNFG0_MEM_MAP      (1<<31) /* Boot mode mapping of EXP_CS0 */

#define OST_PHYS	(0xc8005000)
#define OST_TS		__REG(OST_PHYS + 0x00) /* Time-stamp timer */
#define OST_TS_PHYS	(OST_PHYS + 0x00)
#define OST_WDOG	__REG(OST_PHYS + 0x14) /* Watchdog timer */
#define OST_WDOG_ENAB	__REG(OST_PHYS + 0x18) /* Watchdog enable */
#define OST_WDOG_KEY	__REG(OST_PHYS + 0x1c) /* Watchdog enable */

#define UART0_PHYS	(0xc8000000)
#define UART1_PHYS	(0xc8001000)
#define UART_PHYS	(UART1_PHYS) /* Console */

#endif  /* __IXP42X_H__ */
