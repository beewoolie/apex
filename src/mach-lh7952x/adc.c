/* adc.c
     $Id$

   written by Marc Singer
   1 Jan 2005

   Copyright (C) 2005 Marc Singer

   -----------
   DESCRIPTION
   -----------

 Processing: 
  Initialize the adc to its default state.
  Map all touchscreen pins to ADC input
  Set up the idle step to prevent false pen auto-triggering.
  Flush the result FIFO.
  Clear the pen and end of sequence interrupts.
  Set up the sequence as follows:
      0. Strong pullup on X+ pin, other pins float.
         Measure on pin x+. This measurement will be discarded.
         This step de-glitches any partial triggers.
      1. Weak pullup on X+ pin, X- floats, Y- to Ground and neg
         ref, Y+ floats (ratiometric).
         Measure on X+ pin. This measurement is used to verify
         the pen was down at the beginning of the coordinate
         measurement.
      2. X+ and X- float, Y- to Ground and neg
         ref, Y+ to VDDA_ADC and positive reference (ratiometric).
         Measure on pin x+. This is the X measurement.
      3. Repeat the X measurement 3 more times (we will average
         4 measurements to improve signal-to-noise.
      4. Y+ and Y- float, X- to Ground and neg
         ref, X+ to VDDA_ADC and positive reference (ratiometric).
         Measure on pin Y+. This is the Y measurement.
      5. Repeat the Y measurement 3 more times (we will average
         4 measurements to improve signal-to-noise.
      6. Strong pullup on X+ pin, other pins float.
         Measure on pin x+. This measurement will be discarded.
         This step de-glitches any partial triggers.
      7. Weak pullup on X+ pin, X- floats, Y- to Ground and neg
         ref, Y+ floats (ratiometric).
         Measure on X+ pin. This measurement is used to verify
         the pen was down at the end of the coordinate
         measurement. 
 

*/

#include <config.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <apex.h>
#include <command.h>
#include "hardware.h"

#define ADC_FS_FFF		(1<<3)
#define ADC_FS_FEMPTY		(1<<2)

#define ADC_IC_BOIC		(1<<2)
#define ADC_IC_PENINC		(1<<1)
#define ADC_IC_EOSINTC		(1<<0)

#define ADC_PC_CLKSEL_SHIFT	(8)
#define ADC_PC_CLKSEL_MASK	(7<<ADC_PC_CLKSEL_SHIFT)
#define ADC_PC_REFEN		(1<<5)
#define ADC_PC_BATEN		(1<<4)
#define ADC_PC_NOC_SHIFT	(0)
#define ADC_PC_NOC_MASK		(0xf)
#define ADC_PC_PWM_MASK		(3<<6)	
#define ADC_PC_PWM_OFF		(0<<6)	
#define ADC_PC_PWM_STANDBY	(1<<6)	
#define ADC_PC_PWM_RUN		(2<<6)	

#define ADC_HW_SETTIME_SHIFT	(7)
#define ADC_HW_SETTIME_MASK	(0x1ff<<ADC_HW_SETTIME_SHIFT)
#define ADC_HW_INP_SHIFT	(3)
#define ADC_HW_INP_MASK		(0xf<<ADC_HW_INP_SHIFT)
#define ADC_HW_INP_AN0		(0<<3)		/* UL/X+ */
#define ADC_HW_INP_AN1		(1<<3)		/* UR/X- */
#define ADC_HW_INP_AN2		(2<<3)		/* LL/Y+ */
#define ADC_HW_INP_AN3		(3<<3)		/* LR/Y- */
#define ADC_HW_INP_AN4		(4<<3)		/* Wiper */
#define ADC_HW_INP_AN5		(5<<3)
#define ADC_HW_INP_AN6		(6<<3)
#define ADC_HW_INP_AN7		(7<<3)
#define ADC_HW_INP_AN8		(8<<3)
#define ADC_HW_INP_AN9		(9<<3)
#define ADC_HW_INP_VREFN	(10<<3)		/* VREF- */
#define ADC_HW_INN_GND		(1<<2)
#define ADC_HW_INN_REFN		(0<<2)		/* VREF- */
#define ADC_HW_REFP_MASK	(3<<0)
#define ADC_HW_REFP_VREFP	(0<<0)		/* VREF+ */
#define ADC_HW_REFP_AN0		(1<<0)		/* UL/X+ */
#define ADC_HW_REFP_AN2		(2<<0)		/* LL/Y+ */
#define ADC_HW_REFP_AN8		(3<<0)
#define ADC_LW_REFN_VREFN	(0<<0)
#define ADC_LW_REFN_AN1		(1<<0)
#define ADC_LW_REFN_AN3		(2<<0)
#define ADC_LW_REFN_AN9		(3<<0)
#define ADC_LW_BIASCON_SHIFT	(2)
#define ADC_LW_BIASCON_MASK	(0xfff<<2)

#define ADC_LW_FET_AN0_VDD	(1<<2)
#define ADC_LW_FET_AN0_VDD100K	(1<<12)
#define ADC_LW_FET_AN1_VDD	(1<<3)
#define ADC_LW_FET_AN1_GND	(1<<4)
#define ADC_LW_FET_AN2_VDD	(1<<5)
#define ADC_LW_FET_AN2_GND	(1<<6)
#define ADC_LW_FET_AN3_GND	(1<<7)
#define ADC_LW_FET_AN4_VDD100K	(1<<13)
#define ADC_LW_FET_AN4_GND	(1<<8)

#define ADC_GC_SSM_MASK		(3<<0)
#define ADC_GC_SSM_SSB_PEN	(0<<0)
#define ADC_GC_SSM_PEN		(1<<0)
#define ADC_GC_SSM_SSB		(2<<0)
#define ADC_GC_SSM_CONT		(3<<0)
#define ADC_GC_SSB		(1<<2)
#define ADC_GC_FIFOWMK_SHIFT	(3)
#define ADC_GC_FIFOWMK_MASK	(0xf<<ADC_GC_FIFOWMK_SHIFT)

#define US_SETTLING		(20)		/* Signal stablization */
#define RCPC_ADCPRE_V		(1)
#define ADC_CLKSEL		(0x7)
#define ADC_PC_CLKSEL_V		(ADC_CLKSEL<<ADC_PC_CLKSEL_SHIFT)
#define CLKSEL			(1<<ADC_CLKSEL)

#define A2DCLK			(50803200/RCPC_ADCPRE_V/CLKSEL)
#define SETTIME(us)		(((A2DCLK*(us) + 1000000/2)/1000000)\
				 <<ADC_HW_SETTIME_SHIFT)

static void adc_setup (void)
{
  int i;

  printf ("adc: A2DCLK %d  SETTIME(US) %d\r\n", A2DCLK, SETTIME(US_SETTLING));

  ADC_IC = ADC_IC_BOIC | ADC_IC_PENINC | ADC_IC_EOSINTC;

				/* Flush the results FIFO */
  while ((ADC_FS & ADC_FS_FEMPTY) == 0)
    ADC_RR;

  MASK_AND_SET (ADC_PC, 
		ADC_PC_NOC_MASK | ADC_PC_PWM_MASK,
		(11<<ADC_PC_NOC_SHIFT)|(ADC_PC_PWM_STANDBY)
		|ADC_PC_REFEN);

  MASK_AND_SET (ADC_GC,
		ADC_GC_SSM_MASK | ADC_GC_FIFOWMK_MASK,
		ADC_GC_SSM_SSB
		//		| (12<<ADC_GC_FIFOWMK_SHIFT)
		);

//  ADC_PC |= ADC_PC_REFEN;

#define _HI(p)	(SETTIME (US_SETTLING) | (p) | ADC_HW_INN_REFN)
#define _LO(f)	(f)

	/* Idle */
  ADC_IHWCTRL = _HI (0);
  ADC_ILWCTRL = _LO (0);
    
	/* Sampling */
  for (i = 0; i < 16; ++i) {
    switch (i) {
    case 0:
      __REG(ADC_HWC_BASE_PHYS + i*4) = _HI (ADC_HW_INP_AN0);
      __REG(ADC_LWC_BASE_PHYS + i*4) = _LO (ADC_LW_FET_AN0_VDD);
      break;
    case 1:
      __REG(ADC_HWC_BASE_PHYS + i*4) = _HI (ADC_HW_INP_AN0);
      __REG(ADC_LWC_BASE_PHYS + i*4)
	= _LO (ADC_LW_FET_AN0_VDD100K | ADC_LW_FET_AN3_GND);
      break;
    case 2:
    case 3:
    case 4:
    case 5:
      __REG(ADC_HWC_BASE_PHYS + i*4)
	= _HI (ADC_HW_INP_AN2 | ADC_HW_REFP_AN0);
      __REG(ADC_LWC_BASE_PHYS + i*4)
	= _LO (ADC_LW_REFN_AN1 | ADC_LW_FET_AN0_VDD | ADC_LW_FET_AN1_GND);
      break;
    case 6:
    case 7:
    case 8:
    case 9:
      __REG(ADC_HWC_BASE_PHYS + i*4)
	= _HI (ADC_HW_INP_AN0 | ADC_HW_REFP_AN2);
      __REG(ADC_LWC_BASE_PHYS + i*4)
	= _LO (ADC_LW_REFN_AN3 | ADC_LW_FET_AN2_VDD | ADC_LW_FET_AN3_GND);
      break;
    case 10:
      __REG(ADC_HWC_BASE_PHYS + i*4) = _HI (ADC_HW_INP_AN0);
      __REG(ADC_LWC_BASE_PHYS + i*4) = _LO (ADC_LW_FET_AN0_VDD);
      break;
    case 11:
      __REG(ADC_HWC_BASE_PHYS + i*4) = _HI (ADC_HW_INP_AN0);
      __REG(ADC_LWC_BASE_PHYS + i*4)
	= _LO (ADC_LW_FET_AN0_VDD100K | ADC_LW_FET_AN3_GND);
      break;
    default:
      __REG(ADC_HWC_BASE_PHYS + i*4) = 0;
      __REG(ADC_LWC_BASE_PHYS + i*4) = 0;
      break;
    }
  }
}

static void adc_init (void)
{
  int i;

  __REG (RCPC_PHYS + RCPC_CTRL)      |=  (1<<9);	/* Unlock */
  __REG (RCPC_PHYS + RCPC_ADCPRE)     =  RCPC_ADCPRE_V/2;
  __REG (RCPC_PHYS + RCPC_PCLKSEL1)  &= ~(1<<2);	/* ADC src HCLK */
//  __REG (RCPC_PHYS + RCPC_PCLKSEL1)  |=  (1<<2);	/* ADC src sys-osc */
  __REG (RCPC_PHYS + RCPC_PCLKCTRL1) &= ~(1<<2);	/* Enable ADC clock */
  __REG (RCPC_PHYS + RCPC_CTRL)      &= ~(1<<9);	/* Lock */


  __REG (IOCON_PHYS + IOCON_MUXCTL25) = 0;
  //  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_MUXCTL25),
  //		(3<<6)|(3<<0),
  //		(0<<6)|(0<<0));

  ADC_IM = 0; /* Disable all interrupts */

  ADC_PC = ADC_PC_CLKSEL_V | ADC_PC_PWM_OFF 
    | (1<<ADC_PC_NOC_SHIFT);
  ADC_GC = ADC_GC_SSM_SSB | (1<<ADC_GC_FIFOWMK_SHIFT);


  for (i = 0; i < 15; ++i) {
    __REG(ADC_HWC_BASE_PHYS + i*4) = (0x1ff << ADC_HW_SETTIME_SHIFT)
      | (i << ADC_HW_INP_SHIFT) | ADC_HW_INN_GND | ADC_HW_REFP_VREFP;
    __REG(ADC_LWC_BASE_PHYS + i*4) = ADC_LW_REFN_VREFN;
  }

  ADC_IHWCTRL = (0x1ff << ADC_HW_SETTIME_SHIFT)
    | ADC_HW_INP_AN0 | ADC_HW_INN_GND | ADC_HW_REFP_VREFP;
  ADC_ILWCTRL = ADC_LW_REFN_VREFN;
  
  ADC_IC = ADC_IC_BOIC | ADC_IC_PENINC | ADC_IC_EOSINTC;

				/* Flush the results FIFO */
  while ((ADC_FS & ADC_FS_FEMPTY) == 0)
    ADC_RR;

  adc_setup ();
}


static __service_7 struct service_d lpd79524_adc_service = {
  .init = adc_init,
//  .release = adc_release,
};

