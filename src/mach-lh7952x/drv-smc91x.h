/* drv-smc91x.h
     $Id$

   written by Marc Singer
   27 Jul 2005

   Copyright (C) 2005 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__DRV_SMC91X_H__)
#    define   __DRV_SMC91X_H__

#include <mach/lpd79520.h>

#define SMC_IOBASE		(0x54000000) /* LPD79520 */

#define SMC_IOBARRIER		(*(volatile unsigned char*) 0x20000000)

static inline u16 SMC_inw (unsigned long base, int r)
{
  return *(u16*) (base + r);
}

static inline void SMC_insw (unsigned long base, int r, 
			     unsigned char* pv, int l)
{
  u16* ps = (u16*) pv;
  while (l-- > 0)
    *ps++ = *(u16*) (base + r);
}

static inline void SMC_outw (unsigned long base, int r, u16 v)
{
  *(u16*) (base + r) = v;
  SMC_IOBARRIER;
}

static inline void SMC_outsw (unsigned long base, int r, 
			      unsigned char* p, int l)
{
  unsigned short* ps = (unsigned short*) p;
  while (l-- > 0) {
    *(u16*) (base + r) = *ps++;
    SMC_IOBARRIER;
  }
}

#define RPC_LSA_DEFAULT		RPC_LS_TX_RX
#define RPC_LSB_DEFAULT		RPC_LS_100_10

#endif  /* __DRV_SMC91X_H__ */
