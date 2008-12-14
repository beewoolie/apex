/* nor-cfi.h

   written by Marc Singer
   2 Nov 2008

   Copyright (C) 2008 Marc Singer

   -----------
   DESCRIPTION
   -----------

nor phys fff80000 51 52 59 2
nor_init_chip: success
00000000: ff ff 00 00 00 00 ff ff  00 00 00 00 00 00 00 00  ........ ........
00000010: ff ff ff ff ff ff 00 00  00 00 ff ff 00 00 00 00  ........ ........
00000020: 51 00 52 00 59 00 02 00  00 00 40 00 00 00 00 00  Q.R.Y... ..@.....
00000030: 00 00 00 00 00 00 27 00  36 00 00 00 00 00 04 00  ......'. 6.......
00000040: 00 00 0a 00 00 00 05 00  00 00 04 00 00 00 13 00  ........ ........
00000050: 02 00 00 00 00 00 00 00  04 00 00 00 00 00 40 00  ........ ......@.
00000060: 00 00 01 00 00 00 20 00  00 00 00 00 00 00 80 c0  ...... . ........
00000070: 00 00 06 00 00 00 00 00  01 00 ff ff ff ff ff ff  ........ ........
00000080: 50 00 52 00 49 00 31 00  30 00 00 00 02 00 01 00  P.R.I.1. 0.......
00000090: 01 00 04 00 00 00 00 00  00 00 ff ff ff ff ff ff  ........ ........
000000a0: 00 00 ff ff ff ff 00 00  00 00 00 00 ff ff ff ff  ........ ........
000000b0: ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  ........ ........
000000c0: ff ff ff ff 00 00 00 00  ff ff ff ff 00 00 ff ff  ........ ........
000000d0: 00 00 00 00 ff ff ff ff  ff ff ff ff 00 00 ff ff  ........ ........
000000e0: ff ff ff ff ff ff 00 00  ff ff ff ff ff ff ff ff  ........ ........
000000f0: ff ff ff ff 55 00 ff ff  55 00 ff ff ff ff 00 00  ....U... U.......
    region 0 16384 1
    region 1 8192 2
    region 2 12615680 1
    region 3 65536 7



*/

#if !defined (__NOR_CFI_H__)
#    define   __NOR_CFI_H__

/* ----- Includes */

#include <config.h>
#include <mach/hardware.h>

/* ----- Constants */

#if !defined (NOR_WIDTH)
# define NOR_WIDTH		(16)
#endif
#if !defined (NOR_0_PHYS)
# define NOR_0_PHYS		(0xfff80000)
#endif

#if !defined (NOR_CHIP_MULTIPLIER)
# define NOR_CHIP_MULTIPLIER	(1)	/* Number of chips at REGA */
#endif



#endif  /* __NOR_CFI_H__ */
