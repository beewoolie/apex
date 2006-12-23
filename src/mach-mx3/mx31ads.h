/* mx31ads.h

   written by Marc Singer
   25 Nov 2006

   Copyright (C) 2006 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__MX31ADS_H__)
#    define   __MX31ADS_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#define CPLD_PHYS	(0xb4000000)
#define CPLD_VERSION	__REG16(CPLD_PHYS + 0x0000)
#define CPLD_STATUS2	__REG16(CPLD_PHYS + 0x0002)
#define CPLD_CTRL1_SET	__REG16(CPLD_PHYS + 0x0004)
#define CPLD_CTRL1_CLR	__REG16(CPLD_PHYS + 0x0006)
#define CPLD_CTRL2_SET	__REG16(CPLD_PHYS + 0x0008)
#define CPLD_CTRL2_CLR	__REG16(CPLD_PHYS + 0x000a)
#define CPLD_CTRL3_SET	__REG16(CPLD_PHYS + 0x000c)
#define CPLD_CTRL3_CLR	__REG16(CPLD_PHYS + 0x000e)
#define CPLD_CTRL4_SET	__REG16(CPLD_PHYS + 0x0010)
#define CPLD_CTRL4_CLR	__REG16(CPLD_PHYS + 0x0012)
#define CPLD_STATUS1	__REG16(CPLD_PHYS + 0x0014)
#define CPLD_ISR	__REG16(CPLD_PHYS + 0x0016)
#define CPLD_ICSR	__REG16(CPLD_PHYS + 0x0018) /* Raw Interrupt Status */
#define CPLD_IMR_SET	__REG16(CPLD_PHYS + 0x001a)
#define CPLD_IMR_CLR	__REG16(CPLD_PHYS + 0x001c)
#define CPLD_UARTA	__REG16(CPLD_PHYS + 0x10000)
#define CPLD_UARTB	__REG16(CPLD_PHYS + 0x10010)
#define CPLD_CS8900_BASE __REG16(CPLD_PHYS + 0x20000)
#define CPLD_CS8900_MEM	__REG16(CPLD_PHYS + 0x21000)
#define CPLD_CS8900_DMA	__REG16(CPLD_PHYS + 0x22000)
#define CPLD_AUDIO	__REG16(CPLD_PHYS + 0x30000)


#endif  /* __MX31ADS_H__ */
