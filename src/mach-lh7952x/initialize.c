/* initialize.c
     $Id$

   written by Marc Singer
   28 Oct 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

   Hardware initializations.  Some initializationmay be left to
   drivers, such as the serial interface initialization.

*/

#if defined (HAVE_CONFIG_H)
# include <blob/config.h>
#endif

//#include <blob/arch.h>
#include "lh79524.h"

//#define USE_SLOW

#if defined (USE_SLOW)
# define EMC_RASCAS_V	((3<<0)|(3<<8))	// RAS3, CAS3
#else
# define EMC_RASCAS_V	((3<<0)|(2<<8))	// RAS3, CAS2
#endif

#if defined (USE_SLOW)
# define SDRAM_CHIP_MODE	(0x32<<11)	// CAS3 BURST4
#else
# define SDRAM_CHIP_MODE	(0x22<<11)	// CAS2 BURST4
#endif

	// SRAM devices
#define BCR0_MODE		(0x200039af)	// Bootflash
#define BCR6_MODE		(0x1000fbe0)	// CompactFlash
#define BCR7_MODE		(0x1000b2c2)	// CPLD & Ethernet

	// SDRAM
#define SDRAM_RASCAS		EMC_RASCAS_V
#define SDRAM_CFG_SETUP		((1<<14)|(1<<12)|(3<<9)|(1<<7)) /*32LP;16Mx16*/
#define SDRAM_CFG		(SDRAM_CFG_SETUP | (1<<19))

#if 1
#define __naked __attribute__((naked))
#define __start __attribute__((section(".start")))

typedef struct { volatile unsigned long  offset[4096]; } __regbase;
# define __REG(x)	((__regbase   *)((x)&~4095))->offset[((x)&4095)>>2]
typedef struct { volatile unsigned short offset[4096]; } __regbase16;
# define __REG16(x)	((__regbase16 *)((x)&~4095))->offset[((x)&4095)>>1]
typedef struct { volatile unsigned char  offset[4096]; } __regbase8;
# define __REG8(x)	((__regbase8  *)((x)&~4095))->offset[(x)&4095]
#endif


/* usdelay

   this function accepts a count of microseconds and will wait at
   least that long before returning.  It depends on the timer being
   setup.

   When in C, this function was one instruction longer than the
   hand-coded assembler.  For some reason, the compiler would reload
   the TIMER1_PHYS at the top of the while loop.

 */

static void __attribute__((naked, section(".bootstrap"))) 
     usdelay (unsigned long us)
{
  __asm ("str %1, [%0, #0]\n\t"
	 "str %2, [%0, #0xc]\n\t"
	 "str %3, [%0, #0]\n\t"
      "0: ldr %2, [%0, #0xc]\n\t"
	 "tst %2, %4\n\t"
	 "beq 0b\n\t"
	 "mov pc, lr"
	 :
	 : "r" (TIMER1_PHYS), 
	   "r" (0),
	   "r" (us),
	   "r" ((1<<1)|(5<<2)),
	   "i" (0x8000)
	 );
}


/* initialize_bootstrap

   performs vital SDRAM initialization as well as some other memory
   controller initializations.  It will perform no work if we are
   already running from SDRAM.

   The assembly output of this implementation of the initialization is
   more dense than the assembler version using a table of
   initializations.  This is primarily due to the compiler's ability
   to keep track of the register set offsets and value being output.

*/

void  __attribute__((naked, section(".text"))) initialize_bootstrap (void)
{
  unsigned long lr;
  __asm volatile ("mov %0, lr\n\t"
//	 "tst %0, #0xf0000000\n\t"
//	 "beq 1f\n\t"
	 "cmp %0, %1\n\t" //");
	 "movge pc, lr\n\t"
	 "1:" : "=r" (lr): "i" (SDRAM_BANK0_PHYS));

	/* Setup HCLK, FCLK and peripheral clocks */
  __REG (RCPC_PHYS | RCPC_CTRL)       = RCPC_CTRL_UNLOCK;

  __REG (RCPC_PHYS | RCPC_AHBCLKCTRL) = RCPC_AHBCLKCTRL_V;
  __REG (RCPC_PHYS | RCPC_PCLKCTRL0)  = RCPC_PCLKCTRL0_V;
  __REG (RCPC_PHYS | RCPC_PCLKCTRL1)  = RCPC_PCLKCTRL1_V;
  __REG (RCPC_PHYS | RCPC_PCLKSEL0)   = RCPC_PCLKSEL0_V;

  __REG (RCPC_PHYS | RCPC_CORECONFIG) = RCPC_CORECONFIG_FASTBUS;
  __REG (RCPC_PHYS | RCPC_SYSPLLCNTL) = RCPC_SYSPLLCNTL_V;
  __REG (RCPC_PHYS | RCPC_SYSCLKPRE)  = RCPC_SYSCLKPRE_V;
  __REG (RCPC_PHYS | RCPC_CPUCLKPRE)  = RCPC_CPUCLKPRE_V;
  __REG (RCPC_PHYS | RCPC_CORECONFIG) = RCPC_CORECONFIG_V;

  __REG (RCPC_PHYS | RCPC_CTRL)       = RCPC_CTRL_LOCK;

	/* Setup IO pin multiplexers */
  __REG (IOCON_PHYS | IOCON_MUXCTL5)  = IOCON_MUXCTL5_V;
  __REG (IOCON_PHYS | IOCON_MUXCTL6)  = IOCON_MUXCTL6_V;
  __REG (IOCON_PHYS | IOCON_MUXCTL7)  = IOCON_MUXCTL7_V;
  __REG (IOCON_PHYS | IOCON_MUXCTL10) = IOCON_MUXCTL10_V;
  __REG (IOCON_PHYS | IOCON_MUXCTL11) = IOCON_MUXCTL11_V;
  __REG (IOCON_PHYS | IOCON_MUXCTL12) = IOCON_MUXCTL12_V;
  __REG (IOCON_PHYS | IOCON_MUXCTL19) = IOCON_MUXCTL19_V;
  __REG (IOCON_PHYS | IOCON_MUXCTL20) = IOCON_MUXCTL20_V;

	/* NAND flash, 8 bit */
  __REG (EMC_PHYS | EMC_SCONFIG0)    = 0x80;
  __REG (EMC_PHYS | EMC_SWAITWEN0)   = 1;
  __REG (EMC_PHYS | EMC_SWAITOEN0)   = 1;
  __REG (EMC_PHYS | EMC_SWAITRD0)    = 2;
  __REG (EMC_PHYS | EMC_SWAITPAGE0)  = 2;
  __REG (EMC_PHYS | EMC_SWAITWR0)    = 2;
  __REG (EMC_PHYS | EMC_STURN0)      = 2;

	/* NOR flash, 16 bit */
  __REG (EMC_PHYS | EMC_SCONFIG1)    = 0x81;
  __REG (EMC_PHYS | EMC_SWAITWEN1)   = 1;
  __REG (EMC_PHYS | EMC_SWAITOEN1)   = 1;
  __REG (EMC_PHYS | EMC_SWAITRD1)    = 6;
  __REG (EMC_PHYS | EMC_SWAITPAGE1)  = 2;
  __REG (EMC_PHYS | EMC_SWAITWR1)    = 6;
  __REG (EMC_PHYS | EMC_STURN1)      = 1;


	/* CPLD, 16 bit */
  __REG (EMC_PHYS | EMC_SCONFIG3)    = 0x81;
  __REG (EMC_PHYS | EMC_SWAITWEN3)   = 2;
  __REG (EMC_PHYS | EMC_SWAITOEN3)   = 2;
  __REG (EMC_PHYS | EMC_SWAITRD3)    = 5;
  __REG (EMC_PHYS | EMC_SWAITPAGE3)  = 2;
  __REG (EMC_PHYS | EMC_SWAITWR3)    = 5;
  __REG (EMC_PHYS | EMC_STURN3)      = 2;

	/* SDRAM */
  __REG (EMC_PHYS | EMC_READCONFIG)  = EMC_READCONFIG_CMDDELAY;
  __REG (EMC_PHYS | EMC_DYNCFG0)     = SDRAM_CFG_SETUP;
  __REG (EMC_PHYS | EMC_DYNRASCAS0)  = SDRAM_RASCAS;
  __REG (EMC_PHYS | EMC_DYNCFG1)     = SDRAM_CFG_SETUP;
  __REG (EMC_PHYS | EMC_DYNRASCAS1)  = SDRAM_RASCAS;

  __REG (EMC_PHYS | EMC_PRECHARGE)   = NS_TO_HCLK(20);
  __REG (EMC_PHYS | EMC_DYNM2PRE)    = NS_TO_HCLK(60);
  __REG (EMC_PHYS | EMC_REFEXIT)     = NS_TO_HCLK(120);
  __REG (EMC_PHYS | EMC_DOACTIVE)    = NS_TO_HCLK(120);
  __REG (EMC_PHYS | EMC_DIACTIVE)    = NS_TO_HCLK(120);
  __REG (EMC_PHYS | EMC_DWRT)	     = NS_TO_HCLK(40);
  __REG (EMC_PHYS | EMC_DYNACTCMD)   = NS_TO_HCLK(120);
  __REG (EMC_PHYS | EMC_DYNAUTO)     = NS_TO_HCLK(120);
  __REG (EMC_PHYS | EMC_DYNREFEXIT)  = NS_TO_HCLK(120);
  __REG (EMC_PHYS | EMC_DYNACTIVEAB) = NS_TO_HCLK(40);
  __REG (EMC_PHYS | EMC_DYNAMICTMRD) = NS_TO_HCLK(40);

  __REG (EMC_PHYS | EMC_DYNMCTRL)    = ((1<<1)|(1<<0));
  usdelay (200);
  __REG (EMC_PHYS | EMC_DYNMCTRL)    = (1<<1)|(1<<0)|(3<<7); /* NOP command */
  usdelay (200);
  __REG (EMC_PHYS | EMC_DYNMCTRL)    = (1<<1)|(1<<0)|(2<<7); /* PRECHARGE ALL*/
  __REG (EMC_PHYS | EMC_DYNMREF)     = NS_TO_HCLK(100)/16 + 1;
  usdelay (250);
  __REG (EMC_PHYS | EMC_DYNMREF)     = NS_TO_HCLK(7812)/16;
  __REG (EMC_PHYS | EMC_DYNMCTRL)    = (1<<1)|(1<<0)|(1<<7); /* MODE command */

  __REG (SDRAM_BANK0_PHYS | SDRAM_CHIP_MODE);
  __REG (SDRAM_BANK1_PHYS | SDRAM_CHIP_MODE);

  __REG (EMC_PHYS | EMC_DYNMCTRL)    = (1<<1)|(1<<0)|(0<<7); /* NORMAL */
  __REG (EMC_PHYS | EMC_DYNCFG0)     = SDRAM_CFG;
  __REG (EMC_PHYS | EMC_DYNCFG1)     = SDRAM_CFG;
  
  __asm volatile ("mov pc, %0" : : "r" (lr));
}


/* initialize

   performs the rest of the hardware initialization that didn't have
   to be performed during the bootstrap phase.

*/

void  __attribute__((naked, section(".bootstrap"))) initialize_target (void)
{
	/* CompactFlash, 16 bit */
  __REG (EMC_PHYS | EMC_SCONFIG2)    = 0x81;
  __REG (EMC_PHYS | EMC_SWAITWEN2)   = 2;
  __REG (EMC_PHYS | EMC_SWAITOEN2)   = 2;
  __REG (EMC_PHYS | EMC_SWAITRD2)    = 6;
  __REG (EMC_PHYS | EMC_SWAITPAGE2)  = 2;
  __REG (EMC_PHYS | EMC_SWAITWR2)    = 6;
  __REG (EMC_PHYS | EMC_STURN2)      = 1;
}
