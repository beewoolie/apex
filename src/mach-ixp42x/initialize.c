/* initialize.c
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

   Hardware initializations for the ixp42x.  Some initializations may
   be left to drivers, such as the serial interface and timer.

   The COPROCESSOR_WAIT macro comes from redboot.  It isn't clear to
   me that we need to do this, though the cache and tlb flushing might
   require it.

*/

#include <config.h>
#include <asm/bootstrap.h>
#include <service.h>
#include <debug_ll.h>

#include "hardware.h"

#if CONFIG_SDRAM_BANK_LENGTH == (32*1024*1024)
# if defined CONFIG_SDRAM_BANK1
#  define SDR_CONFIG_CHIPS	SDR_CONFIG_4x8Mx16;
# else
#  define SDR_CONFIG_CHIPS	SDR_CONFIG_2x8Mx16;
# endif
#endif

#if CONFIG_SDRAM_BANK_LENGTH == (64*1024*1024)
# if defined CONFIG_SDRAM_BANK1
#  define SDR_CONFIG_CHIPS	SDR_CONFIG_4x16Mx16;
# else
#  define SDR_CONFIG_CHIPS	SDR_CONFIG_2x16Mx16;
# endif
#endif

#if CONFIG_SDRAM_BANK_LENGTH == (128*1024*1024)
# if defined CONFIG_SDRAM_BANK1
#  define SDR_CONFIG_CHIPS	SDR_CONFIG_4x32Mx16;
# else
#  define SDR_CONFIG_CHIPS	SDR_CONFIG_2x32Mx16;
# endif
#endif


/* *** FIXME: sdram config constants should go here.  The Integrated
   Circuit Solution Inc IC42S16800 DRAM can do CAS2.  Later. */


/* usleep

   this function accepts a count of microseconds and will wait at
   least that long before returning.  The base counter rate is
   66.66MHz or a 15ns cycle time.  1us takes 67 counts.

   The 32 bit counter wraps after about 64s.

   Note that this function is neither __naked nor static.  It is
   available to the rest of APEX. 

 */

void __section (.bootstrap) usleep (unsigned long us)
{
  unsigned long s;
  __asm volatile ("ldr %0, [%1]" : "=r" (s) : "r" (OST_TS_PHYS));
  us *= 67;

  __asm volatile ("0: ldr %0, [%1]\n\t"
		     "sub %0, %0, %2\n\t"
		     "cmp %0, %3\n\t"
		     "bls 0b\n\t"
		  : : "r" (0), 
		      "r" (OST_TS_PHYS),
		      "r" (s),
		      "r" (us)
		  );
}


/* initialize_bootstrap

   performs the critical bootstrap initializaton before APEX is copied
   to SDRAM.

   It *should* perform no work if we are already running from SDRAM.

   The return value is true if SDRAM has been initialized and false if
   this initialization has already been performed.  Note that the
   non-SDRAM initializations are performed regardless of whether or
   not we're running in SDRAM.

*/

void __naked __section (.bootstrap) initialize_bootstrap (void)
{
  unsigned long lr;

  __asm volatile ("mov %0, lr" : "=r" (lr));

#if defined (CONFIG_DEBUG_LL)
  UART_LCR  = UART_LCR_WLS_8 | UART_LCR_STB_1 | UART_LCR_DLAB;
  UART_DLL  = 8;	// divisor_l;
  UART_DLH  = 0;	// divisor_h;
  UART_LCR  = UART_LCR_WLS_8 | UART_LCR_STB_1;

  UART_IER  = UART_IER_UUE;	/* Enable UART, mask all interrupts */
				/* Clear interrupts? */
  PUTC_LL('A');
#endif

  GPIO_ER &= ~0xf;
  _L(LEDf);			/* Start with all on */

	/* Configure GPIO */
  //  GPIO_OUTR = 0x20c3; 
  //  GPIO_ER   &= ~GPIO_ER_OUTPUTS;
  GPIO_ER   = GPIO_ER_V;
  //  GPIO_ISR  = 0x31f3;
  GPIO_IT1R = 0x9240;		/* GPIO2 AL; GPIO3 AL; GPIO5 RE; GPIO6 RE */
  GPIO_IT2R = 0x0249;		/* GPIO8 AL; GPIO9 AL; GPIO10 AL; GPIO11 AL */
  GPIO_CLKR = GPIO_CLKR_V;

  /* *** FIXME: this CPSR and CP15 manipulation should not be
     *** necessary as we don't allow warm resets.  This is here until
     *** we get a working loader. */

	/* Disable interrupts and set supervisor mode */
  __asm volatile ("msr cpsr, %0" : : "r" ((1<<7)|(1<<6)|(0x13<<0)));
 
	/* Drain write buffer */
  __asm volatile ("mcr p15, 0, r0, c7, c10, 4" : : : "r0");
  COPROCESSOR_WAIT;

	/* Invalidate caches (I&D) and branch buffer (BTB) */
  __asm volatile ("mcr p15, 0, r0, c7, c7, 0" : : : "r0");
  COPROCESSOR_WAIT;

	/* Invalidate TLBs (I&D) */
  __asm volatile ("mcr p15, 0, r0, c8, c7, 0" : : : "r0");
  COPROCESSOR_WAIT;

	/* Disable write buffer coalescing */
#if 1
  {
    unsigned long v;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 1\n\t" 
		    "orr %0, %0, #(1<<0)\n\t"
		    "mcr p15, 0, %0, c1, c0, 1"
		    : "=r" (v));
  }		

  COPROCESSOR_WAIT;
#endif

     	/* Fixup the CP15 control word.  This is done for the cases
	   where we are bootstrapping from redboot which does not
	   cleanup before jumping to code.  */
  __asm volatile ("mrc p15, 0, r0, c1, c0, 0\n\t"
		  "bic r0, r0, #(1<<0)\n\t" /* Disable MMU */
//		  "bic r0, r0, #(1<<1)\n\t" /* Disable alignment check */
//		  "orr r0, r0, #(1<<4)\n\t" /* Enable write buffer */
//		  "bic r0, r0, #(1<<4)\n\t" /* Disable write buffer */
//		  "orr r0, r0, #(1<<12)\n\t" /* Enable instruction cache */
//		  "bic r0, r0, #(1<<12)\n\t" /* Disable instruction cache */
		  "mcr p15, 0, r0, c1, c0, 0" : : : "r0");
  COPROCESSOR_WAIT;


	/* Configure flash access, slowest timing */
  /* *** FIXME: do we really need this?  We're already running in
     *** flash.  Moreover, I'd rather make it fast instead of slow. */
  EXP_TIMING_CS0 = 0
    | (( 3 & EXP_T1_MASK)	<<EXP_T1_SHIFT)
    | (( 3 & EXP_T2_MASK)	<<EXP_T2_SHIFT)
    | ((15 & EXP_T3_MASK)	<<EXP_T3_SHIFT)
    | (( 3 & EXP_T4_MASK)	<<EXP_T4_SHIFT)
    | ((15 & EXP_T5_MASK)	<<EXP_T5_SHIFT)
    | ((15 & EXP_CNFG_MASK)	<<EXP_CNFG_SHIFT)	/* 16MiB window */
    | (( 0 & EXP_CYC_TYPE_MASK)	<<EXP_CYC_TYPE_SHIFT)	/* Intel cycling */
    | EXP_BYTE_RD16
    | EXP_WR_EN
    | EXP_CS_EN
    ;

	  /* Exit now if executing in SDRAM */ 
  if (EXP_CNFG0 & (1<<31)) {
    PUTC_LL ('b');
    _L(LED1);

    /* Boot mode */
    __asm volatile ("cmp %0, %2\n\t"
		    "bls 1f\n\t"
		    "cmp %0, %1\n\t"
		    "movls r0, #0\n\t"
		    "movls pc, %0\n\t"
		    "1:": : "r" (lr), "i" (0x40000000), "i" (256*1024*1024));

    PUTC_LL ('f');
    /* Jump to the proper flash address before disabling boot mode */

    {
      __asm volatile ("add %0, %0, %1\n\t"
		      "add pc, pc, %1"
		      : "=r" (lr) : "r" (0x50000000 - 4));
    }

	/* Disable interrupts and set supervisor mode */
    __asm volatile ("msr cpsr, %0" : : "r" ((1<<7)|(1<<6)|(0x13<<0)));
 
	/* Drain write buffer */
    __asm volatile ("mcr p15, 0, r0, c7, c10, 4" : : : "r0");
    COPROCESSOR_WAIT;

    PUTC_LL ('+');
    EXP_CNFG0 &= ~EXP_CNFG0_MEM_MAP; /* Disable boot-mode for EXP_CS0  */
    PUTC_LL ('#');
  }
  else {
    PUTC_LL ('n');
    _L(LED2);

    /* Non-boot mode */
    __asm volatile ("cmp %0, %1\n\t"
		    "movls r0, #0\n\t"
		    "movls pc, %0\n\t"
		    : : "r" (lr), "i" (0x40000000));
  }

  PUTC_LL ('X');

  usleep (1000);		/* Wait for CPU to stabilize SDRAM signals */

  SDR_CONFIG = SDR_CONFIG_CAS3 | SDR_CONFIG_CHIPS;
  SDR_REFRESH = 0;		/* Disable refresh */
  SDR_IR = SDR_IR_NOP;
  usleep (200);			/* datasheet: maintain 200us of NOP */

  /* 133MHz timer cycle count, 0x81->969ns == ~1us */
  /* datasheet: not clear what the refresh requirement is.  */
  SDR_REFRESH = 0x81;		/* *** FIXME: calc this */

  SDR_IR = SDR_IR_PRECHARGE_ALL;

  usleep (1);			/* datasheet: wait for Trp (<=20ns) 
				   before starting AUTO REFRESH */

  /* datasheet: needs at least 8 refresh (bus) cycles.  Timing diagram
     shows only two AUTO_REFRESH commands, each is Trc (20ns) long. */

  SDR_IR = SDR_IR_AUTO_REFRESH;
  usleep (1);
  SDR_IR = SDR_IR_AUTO_REFRESH;
  usleep (1);

  SDR_IR = SDR_IR_MODE_CAS3;
  SDR_IR = SDR_IR_NORMAL;
  usleep (1);			/* Must wait for 3 CPU cycles before
				   SDRAM access */

  _L(LED3);

  __asm volatile ("mov r0, #-1\t\n"
		  "mov pc, %0" : : "r" (lr));
}


/* target_init

   performs the rest of the hardware initialization that didn't have
   to be performed during the bootstrap phase.

*/

static void target_init (void)
{
  _L(LED6);

  //  EXP_CNFG0 &= ~EXP_CNFG0_MEM_MAP; /* Disable boot-mode for EXP_CS0  */
  //  __REG(EXP_PHYS + 0x28) |= (1<<15);	/* Undocumented, but set in redboot */

#if defined (CONFIG_MACH_NSLU2)

  EXP_TIMING_CS4 = 0
    | (( 3 & EXP_T1_MASK)	<<EXP_T1_SHIFT)
    | (( 3 & EXP_T2_MASK)	<<EXP_T2_SHIFT)
    | ((15 & EXP_T3_MASK)	<<EXP_T3_SHIFT)
    | (( 3 & EXP_T4_MASK)	<<EXP_T4_SHIFT)
    | ((15 & EXP_T5_MASK)	<<EXP_T5_SHIFT)
    | ((0 & EXP_CNFG_MASK)	<<EXP_CNFG_SHIFT)	/* 512 b window */
    | EXP_WR_EN
    | EXP_CS_EN
    | EXP_BYTE_EN;

  EXP_TIMING_CS5 = 0
    | (( 3 & EXP_T1_MASK)	<<EXP_T1_SHIFT)
    | (( 3 & EXP_T2_MASK)	<<EXP_T2_SHIFT)
    | ((15 & EXP_T3_MASK)	<<EXP_T3_SHIFT)
    | (( 3 & EXP_T4_MASK)	<<EXP_T4_SHIFT)
    | ((15 & EXP_T5_MASK)	<<EXP_T5_SHIFT)
    | ((0 & EXP_CNFG_MASK)	<<EXP_CNFG_SHIFT)	/* 512 b window */
    | EXP_WR_EN
    | EXP_CS_EN
    | EXP_BYTE_EN;

  EXP_TIMING_CS7 = 0
    | (( 3 & EXP_T1_MASK)	<<EXP_T1_SHIFT)
    | (( 3 & EXP_T2_MASK)	<<EXP_T2_SHIFT)
    | ((15 & EXP_T3_MASK)	<<EXP_T3_SHIFT)
    | (( 3 & EXP_T4_MASK)	<<EXP_T4_SHIFT)
    | ((15 & EXP_T5_MASK)	<<EXP_T5_SHIFT)
    | ((0 & EXP_CNFG_MASK)	<<EXP_CNFG_SHIFT)	/* 512 b window */
    | EXP_WR_EN
    | EXP_MUX_EN
    | EXP_CS_EN;

  //    *IXP425_EXP_CS4 = (EXP_ADDR_T(3) | EXP_SETUP_T(3) | EXP_STROBE_T(15) | EXP_HOLD_T(3) | EXP_RECOVERY_T(15) | EXP_SZ_512 | EXP_WR_EN | EXP_CS_EN | EXP_BYTE_EN);
  //    *IXP425_EXP_CS5 = (EXP_ADDR_T(3) | EXP_SETUP_T(3) | EXP_STROBE_T(15) | EXP_HOLD_T(3) | EXP_RECOVERY_T(15) | EXP_SZ_512 | EXP_WR_EN | EXP_CS_EN | EXP_BYTE_EN);
  //    *IXP425_EXP_CS7 = (EXP_ADDR_T(3) | EXP_SETUP_T(3) | EXP_STROBE_T(15) | EXP_HOLD_T(3) | EXP_RECOVERY_T(15) | EXP_SZ_512 | EXP_MUX_EN | EXP_WR_EN | EXP_CS_EN);

#endif

	/* PCI initialization */
  PCI_PCIMEMBASE = 0x48494a4b;
  PCI_AHBMEMBASE = 0x00010203;

  PCI_CONFIG_WRITE32 (PCI_CFG_BAR_0, 0x00000000);
  PCI_CONFIG_WRITE32 (PCI_CFG_BAR_1, 0x01000000);
  PCI_CONFIG_WRITE32 (PCI_CFG_BAR_2, 0x02000000);
  PCI_CONFIG_WRITE32 (PCI_CFG_BAR_3, 0x03000000);
  PCI_CONFIG_WRITE32 (PCI_CFG_BAR_4, 0x80000000); /* Put these */
  PCI_CONFIG_WRITE32 (PCI_CFG_BAR_5, 0x90000000); /*  out of reach */

  //  cyg_pci_set_memory_base(HAL_PCI_ALLOC_BASE_MEMORY);
  //  cyg_pci_set_io_base(HAL_PCI_ALLOC_BASE_IO);

  PCI_ISR = PCI_ISR_PSE | PCI_ISR_PFE | PCI_ISR_PPE | PCI_ISR_AHBE;
  PCI_CSR = PCI_CSR_IC | PCI_CSR_ABE | PCI_CSR_PDS | PCI_CSR_ADS;
  PCI_CONFIG_WRITE16 (PCI_CFG_COMMAND, 
		      PCI_CFG_COMMAND_MASTER | PCI_CFG_COMMAND_MEMORY);

  _L(LED7);
}

static void target_release (void)
{
  /* *** FIXME: I don't think this is necessary.  I'm trying to figure
     *** out why the kernel fails to boot.  */
 
	/* Invalidate caches (I&D) and BTB (?) */
  __asm volatile ("mcr p15, 0, r0, c7, c7, 0" : : : "r0");
  COPROCESSOR_WAIT;

	/* Invalidate TLBs (I&D) */
  __asm volatile ("mcr p15, 0, r0, c8, c7, 0" : : : "r0");
  COPROCESSOR_WAIT;

	/* Drain write buffer */
  __asm volatile ("mcr p15, 0, r0, c7, c10, 4" : : : "r0");
  COPROCESSOR_WAIT;

	/* Disable write buffer coalescing */
  {
    unsigned long v;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 1\n\t" 
		    "orr %0, %0, #(1<<0)\n\t"
		    "mcr p15, 0, %0, c1, c0, 1"
		    : "=r" (v));
    COPROCESSOR_WAIT;
  }		
}

static __service_0 struct service_d ixp42x_target_service = {
  .init    = target_init,
  .release = target_release,
};
