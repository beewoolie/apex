/* drv-smc91x.h

   written by Marc Singer
   9 Jul 2008

   Copyright (C) 2008 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__DRV_SMC91X_H__)
#    define   __DRV_SMC91X_H__

#include <mach/hardware.h>

/* The PCM037 puts the SMC91x on CS1, 0xa8000000 */
# define SMC_IOBASE		(0xa8000000)

static inline u16 SMC_inw (unsigned long base, int r)
{
  u16 v = *(volatile u16*) (base + r);
  return v;
}

static inline void SMC_insw (unsigned long base, int r,
			     void* pv, int l)
{
  u16* ps = (u16*) pv;
  while (l-- > 0) {
    *ps++ = *(volatile u16*) (base + r);
  }
}

static inline void SMC_outw (unsigned long base, int r, u16 v)
{
  *(volatile u16*) (base + r) = v;
}

static inline void SMC_outsw (unsigned long base, int r,
			      const void* p, int l)
{
  const unsigned short* ps = (const unsigned short*) p;
  while (l-- > 0) {
    *(volatile u16*) (base + r) = *ps++;
  }
}

#define RPC_LSA_DEFAULT		RPC_LS_TX_RX
#define RPC_LSB_DEFAULT		RPC_LS_100_10

#endif  /* __DRV_SMC91X_H__ */
