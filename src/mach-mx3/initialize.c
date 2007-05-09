/* initialize.c

   written by Marc Singer
   22 Jan 2007

   Copyright (C) 2007 Marc Singer

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

   Hardware initializations.  Some initialization may be left to
   drivers, such as the serial interface initialization.

   o Size.  The setup for the MX31 is substantially more verbose than
     other platforms we've handled.  With OneNAND boot, there is only
     1K of code available before we can relocate to SDRAM (unless we
     do some other heroics that I'd prefer to avoid.  So there is some
     pressure on the initialize_bootstrap() and relocate() functions
     to be brief.

*/

#include <config.h>
#include <asm/bootstrap.h>
#include <service.h>
#include <sdramboot.h>
#include <asm/cp15.h>

#include "hardware.h"
#include <debug_ll.h>

#define ESDCTL_CTL0_V 0\
	|(1<<31)		/* SDE - enable */\
	|(2<<24)		/* ROW - 13 rows */\
/*	|(1<<20)		/* COL - 9 columns */\
	|(2<<20)		/* COL - 10 columns */\
/*	|(1<<16)		/* DSIZ - 16 bit */\
	|(2<<16)		/* DSIZ - 32 bit */\
	|(3<<13)		/* SREFR - 7.81us refresh */\
	|(1<<7)			/* BL - burst length of 8 */


#if 0
static void __naked __used __bootstrap_2 target_preinit (void)
{
  STORE_REMAP_PERIPHERAL_PORT (0x40000000 | 0x15); /* 1GiB @ 1GiB */
}
#endif


/* usleep

   this function accepts a count of microseconds and will wait at
   least that long before returning.  It depends on the timer being
   setup.

   Note that this function is neither __naked nor static.  It is
   available to the rest of the application as is.

   The source for the timer is really the IPG clock and not the high
   clock regardless of the setting for CLKSRC as either HIGH or IPG.
   In both cases, the CPU uses the IPG clock.  The value for IPGCLK
   comes from the main PLL and is available in a constant.  We divide
   down to get a 1us interval.

 */

void __section (.bootstrap) usleep (unsigned long us)
{
  unsigned long mode =
    EPT_CR_CLKSRC_IPG
    | EPT_CR_IOVW
    | ((IPGCLK/1000000 - 1) << EPT_CR_PRESCALE_SH)
    | EPT_CR_DBGEN
    | EPT_CR_SWR
    ;
  __asm volatile ("str %1, [%2, #0]\n\t"	/* EPITCR <- mode w/reset */
		  "orr %1, %1, #1\n\t"		/* |=  EPT_CR_EN */
		  "bic %1, %1, #0x10000\n\t"	/* &= ~EPT_CR_SWR  */
		  "str %1, [%2, #0]\n\t"	/* EPITCR <- mode w/EN */
		  "str %0, [%2, #8]\n\t"	/* EPITLR <- count */
	       "0: ldr %0, [%2, #4]\n\t"	/* EPITSR */
		  "tst %0, #1\n\t"
		  "beq 0b\n\t"
		  : "+r" (us)
		  :  "r" (mode),
		     "r" (PHYS_EPIT1)
		  : "cc");
}


/* initialize_bootstrap

   performs vital SDRAM initialization as well as some other memory
   controller initializations.  It will perform no work if we are
   already running from SDRAM.

   The return value is true if SDRAM has been initialized and false if
   this initialization has already been performed.  Note that the
   non-SDRAM initializations are performed regardless of whether or
   not we're running in SDRAM.

*/

void __naked __section (.bootstrap) initialize_bootstrap (void)
{
  unsigned long lr;
  __asm volatile ("mov %0, lr" : "=r" (lr));

  STORE_REMAP_PERIPHERAL_PORT (0x40000000 | 0x15); /* 1GiB @ 1GiB */

  __REG (PHYS_L2CC + 0x08) = 0; /* Disable L2CC, should be redundant */

  //  __REG(PHYS_IPU + 0x00) |= 0x40; /* Enable DI?  From Redboot. */

  /* *** FIXME: Changing this timer while the system is running from
     SDRAM may have adverse affects.  In fact, we may want to defer
     all of this setup when we are not in flash. */
#if 1
  /* Reset clock controls.  This seems to make APEX behave better when
     we're being executed after the clocks and memory have been
     initialized. *** FIXME: we need to determine exactly what needs
     to be done here so that we can remove cruft. */
//  CCM_COSR  = 0x00000280;
//  CCM_PDR1  = 0x49fcfe7f;

//  CCM_MPCTL = 0x04001800;
//  CCM_PDR0  = 0xff870b48;

//  if (1 || (CCM_MPCTL != CCM_MPCTL_V && 0)) {
//  CCM_CCMR  |=  (1<<7);		/* Bypass PLL clock */
  CCM_CCMR  &= ~(1<<3);				/* Disable PLL */
  CCM_CCMR   = 0x074b0bf5;			/* Source CKIH; MCU bypass */
  { int i = 0x1000; while (i--) ; }		/* Delay */
  CCM_CCMR  |=  (1<<3);				/* Enable PLL */
  CCM_CCMR  &= ~(1<<7);				/* MCU from PLL */
  CCM_PDR0   = CCM_PDR0_V;
  CCM_MPCTL  = CCM_MPCTL_V;
  CCM_PDR1   = 0x49fcfe7f;			/* Default value */
  CCM_UPCTL  = CCM_UPCTL_V;
  CCM_COSR   = CCM_COSR_V;
//  }
#endif

  //;WM32  0xb8002050 0x0000dcf6            ; Configure PSRAM on CS5
  //;WM32  0xb8002054 0x444a4541
  //;WM32  0xb8002058 0x44443302
  //;WM32  0xB6000000 0xCAFECAFE

  CCM_CGR0 |= CCM_CGR_RUN << CCM_CGR0_EPIT1_SH;	/* Enable EPIT1 clock  */

  /* NOR flash initialization, though this shouldn't be necessary
     unless we're going to reduce timing latencies.  */
  //  WEIM_UCR(0) = 0x0000CC03; // ; Start 16 bit NorFlash on CS0
  //  WEIM_LCR(0) = 0xa0330D01; //
  //  WEIM_ACR(0) = 0x00220800; //

#if defined (CONFIG_MACH_MX31ADS)
  WEIM_UCR(4) = 0x0000DCF6; // ; Configure CPLD on CS4
  WEIM_LCR(4) = 0x444A4541; //
  WEIM_ACR(4) = 0x44443302; //
#endif

#if defined (CONFIG_STARTUP_UART)

  INITIALIZE_CONSOLE_UART;

  PUTC('A');

#endif

#if defined LED_ON
  LED_ON (0);
#endif

  __asm volatile ("cmp %0, %1\n\t"
		  "bls 1f\n\t"
		  "cmp %0, %2\n\t"
		  "bhi 1f\n\t"
#if defined (CONFIG_SDRAMBOOT_REPORT)
		  "mov r0, #1\n\t"
		  "str r0, [%3]\n\t"
#endif
		  "mov r0, #0\n\t"
		  "mov pc, %0\n\t"
		  "1:"
		  :: "r" (lr),
		     "I" (SDRAM_BANK0_PHYS),
		     "I" (SDRAM_END_PHYS)
#if defined (CONFIG_SDRAMBOOT_REPORT)
		  ,  "r" (&fSDRAMBoot)
#endif
		  : "cc");

  PUTC ('S');

		/* Initialize IOMUX for SDRAM */
  {
    /* Initialize IOMUX from 0x43fac26c to 0x43fac2dc */
    int i;
    for (i = 0; i < 29; ++i) {
      if (i == (0x43fac27c - 0x43fac26c)/4) /* Skip CS2 */
	continue;
      __REG (0x43fac26c + i*4) = 0;
    }
  }
  __REG (0x43fac27c) = 0x1000; // ; CS2 (CSD0)

	/* SDRAM initialization */
  ESDCTL_CTL0 = 0;
  ESDCTL_CFG0 = 0x0075e73a;
  ESDCTL_MISC = ESDCTL_MISC_RST;	/* Reset */
  ESDCTL_MISC = ESDCTL_MISC_MDDREN;	/* Enable DDR */
  usleep (1);			/* > 200ns */
  ESDCTL_CTL0 = 0x92100000;
  __REG (0x80000f00) = 0;	/* DDR */
  ESDCTL_CTL0 = 0xa2100000;
  __REG (0x80000000) = 0;
  ESDCTL_CTL0 = 0xb2100000;
  __REG8 (0x80000000 + 0x33) = 0;	/* Burst mode */
  __REG8 (0x81000000) = 0xff;
  ESDCTL_CTL0 = ESDCTL_CTL0_V;
  __REG (0x80000000) = 0;
  /* *** FIXME: we should check for DDR here.  we can test CTL0_V */
  ESDCTL_MISC = ESDCTL_MISC_RST | ESDCTL_MISC_MDDREN;
  __REG (0x80000000) = 0x55555555;
  __REG (0x80000004) = 0xaaaaaaaa;

#if 0				/* BDI */
//  ESDCTL_CFG0 = 0x0075e73a;
// ESDCTL_CFG0 = 0x0076eb3a; // Reset value
  ESDCTL_CFG0 = 0x006ac73a; // BDI value

  ESDCTL_MISC = ESDCTL_MISC_RST;	/* Reset */
  ESDCTL_MISC = ESDCTL_MISC_MDDREN;	/* Enable DDR */
  usleep (200);

  ESDCTL_CTL0 = 0x92100000;
  __REG (0x80000f00) = 0;		/* DDR */
  ESDCTL_CTL0 = 0xa2100000;
  __REG (0x80000000) = 0;
  __REG (0x80000000) = 0;
  ESDCTL_CTL0 = 0xb2100000;
  __REG8 (0x80000000 + 0x33) = 0;	/* Burst mode */
  __REG8 (0x81000000) = 0xff;

  ESDCTL_CTL0 = 0x82116080
    + 0x00100000			/* DDR */
    + 0x00010000			/* x32 */
    ;
  __REG (0x80000000) = 0;
  ESDCTL_MISC = 0xc;		/* DDR and delay line reset */
  __REG (0x80000000) = 0x55555555;
  __REG (0x80000004) = 0xaaaaaaaa;

#endif

#if 0
+    .macro setup_sdram, name, bus_width, mode, full_page
+        /* It sets the "Z" flag in the CPSR at the end of the macro */
+        ldr r0, ESDCTL_BASE_W
+        mov r2, #SDRAM_BASE_ADDR
+        ldr r1, SDRAM_0x0075E73A
+        str r1, [r0, #0x4]
+        ldr r1, =0x2            // reset
+        str r1, [r0, #0x10]
+        ldr r1, SDRAM_PARAM1_\mode
+        str r1, [r0, #0x10]
+        // Hold for more than 200ns
+        ldr r1, =0x10000
+1:
+        subs r1, r1, #0x1
+        bne 1b
+
+        ldr r1, SDRAM_0x92100000
+        str r1, [r0]
+        ldr r1, =0x0
+        ldr r12, SDRAM_PARAM2_\mode
+        str r1, [r12]
+        ldr r1, SDRAM_0xA2100000
+        str r1, [r0]
+        ldr r1, =0x0
+        str r1, [r2]
+        ldr r1, SDRAM_0xB2100000
+        str r1, [r0]
+
+        ldr r1, =0x0
+        .if \full_page
+        strb r1, [r2, #SDRAM_FULL_PAGE_MODE]
+        .else
+        strb r1, [r2, #SDRAM_BURST_MODE]
+        .endif
+
+        ldr r1, =0xFF
+        ldr r12, =0x81000000
+        strb r1, [r12]
+        ldr r3, SDRAM_0x82116080
+        ldr r4, SDRAM_PARAM3_\mode
+        add r3, r3, r4
+        ldr r4, SDRAM_PARAM4_\bus_width
+        add r3, r3, r4
+        .if \full_page
+        add r3, r3, #0x100   /* Force to full page mode */
+        .endif
+
+        str r3, [r0]
+        ldr r1, =0x0
+        str r1, [r2]
+        /* Below only for DDR */
+        ldr r1, [r0, #0x10]
+        ands r1, r1, #0x4
+        ldrne r1, =0x0000000C
+        strne r1, [r0, #0x10]
+        /* Testing if it is truly DDR */
+        ldr r1, SDRAM_0x55555555
+        ldr r0, =SDRAM_BASE_ADDR
+        str r1, [r0]
+        ldr r2, SDRAM_0xAAAAAAAA
+        str r2, [r0, #0x4]
+        ldr r2, [r0]
+        cmp r1, r2
+    .endm
+
#endif

  PUTC ('s');

#if defined (CONFIG_SDRAMBOOT_REPORT)
  barrier ();
  fSDRAMBoot = 0;
#endif

  __asm volatile ("mov r0, #-1\t\n"
		  "mov pc, %0" : : "r" (lr));
}


/* target_init

   performs the rest of the hardware initialization that didn't have
   to be performed during the bootstrap phase.

*/

static void target_init (void)
{
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

#if defined (CONFIG_MACH_EXBIBLIO_ROSENCRANTZ)
  /* *** Unoptimized access times */
  WEIM_UCR(4) = 0x0000DCF6; // ; Configure DM9000 on CS4
  WEIM_LCR(4) = 0x444A4541; //
  WEIM_ACR(4) = 0x44443302; //
#endif
}


static void target_release (void)
{
}

static __service_0 struct service_d mx3x_target_service = {
  .init    = target_init,
  .release = target_release,
};


#if 0

;; 0x10 MISC
;;  WM32  0xB8001010 0x00000004		; Initialization script for 32 bit DDR on Tortola EVB
;; 0x04 CFG0
;;  WM32  0xB8001004 0x006ac73a
;; 0x00 CTL0
;;  WM32  0xB8001000 0x92100000
;;  WM32  0x80000f00 0x12344321
;;  WM32  0xB8001000 0xa2100000
;;  WM32  0x80000000 0x12344321
;;  WM32  0x80000000 0x12344321
;;  WM32  0xB8001000 0xb2100000
;;  WM8   0x80000033 0xda
;;  WM8   0x81000000 0xff
;;  WM32  0xB8001000 0x82226080
;;  WM32  0x80000000 0xDEADBEEF
;;  WM32  0xB8001010 0x0000000c
;;  WGPR  15         0x83F00000		; boot secret

+    setup_sdram ddr X32 DDR 0

+    .macro setup_sdram, name, bus_width, mode, full_page
+        /* It sets the "Z" flag in the CPSR at the end of the macro */
+        ldr r0, ESDCTL_BASE_W
+        mov r2, #SDRAM_BASE_ADDR
+        ldr r1, SDRAM_0x0075E73A
+        str r1, [r0, #0x4]
+        ldr r1, =0x2            // reset
+        str r1, [r0, #0x10]
+        ldr r1, SDRAM_PARAM1_\mode
+        str r1, [r0, #0x10]
+        // Hold for more than 200ns
+        ldr r1, =0x10000
+1:
+        subs r1, r1, #0x1
+        bne 1b
+
+        ldr r1, SDRAM_0x92100000
+        str r1, [r0]
+        ldr r1, =0x0
+        ldr r12, SDRAM_PARAM2_\mode
+        str r1, [r12]
+        ldr r1, SDRAM_0xA2100000
+        str r1, [r0]
+        ldr r1, =0x0
+        str r1, [r2]
+        ldr r1, SDRAM_0xB2100000
+        str r1, [r0]
+
+        ldr r1, =0x0
+        .if \full_page
+        strb r1, [r2, #SDRAM_FULL_PAGE_MODE]
+        .else
+        strb r1, [r2, #SDRAM_BURST_MODE]
+        .endif
+
+        ldr r1, =0xFF
+        ldr r12, =0x81000000
+        strb r1, [r12]
+        ldr r3, SDRAM_0x82116080
+        ldr r4, SDRAM_PARAM3_\mode
+        add r3, r3, r4
+        ldr r4, SDRAM_PARAM4_\bus_width
+        add r3, r3, r4
+        .if \full_page
+        add r3, r3, #0x100   /* Force to full page mode */
+        .endif
+
+        str r3, [r0]
+        ldr r1, =0x0
+        str r1, [r2]
+        /* Below only for DDR */
+        ldr r1, [r0, #0x10]
+        ands r1, r1, #0x4
+        ldrne r1, =0x0000000C
+        strne r1, [r0, #0x10]
+        /* Testing if it is truly DDR */
+        ldr r1, SDRAM_0x55555555
+        ldr r0, =SDRAM_BASE_ADDR
+        str r1, [r0]
+        ldr r2, SDRAM_0xAAAAAAAA
+        str r2, [r0, #0x4]
+        ldr r2, [r0]
+        cmp r1, r2
+    .endm


#endif
