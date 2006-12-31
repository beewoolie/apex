/* drv-npe.h

   written by Marc Singer
   21 Jul 2006

   Copyright (C) 2006 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__DRV_NPE_H__)
#    define   __DRV_NPE_H__

/* ----- Includes */

#include <mach/hardware.h>

/* ----- Types */

#define NPEA_PHYS	IXP4XX_NPEA_PHYS
#define NPEB_PHYS	IXP4XX_NPEB_PHYS
#define NPEC_PHYS	IXP4XX_NPEC_PHYS

#define NPEA_NAME	"NPE-A"
#define NPEA_DATA_SIZE	(0x800)
#define NPEA_INST_SIZE	(0x1000)

#define NPEB_NAME	"NPE-B"
#define NPEB_DATA_SIZE	(0x800)
#define NPEB_INST_SIZE	(0x1000)

#define NPEC_NAME	"NPE-C"
#define NPEC_DATA_SIZE	(0x800)
#define NPEC_INST_SIZE	(0x1000)

#define ETHA_PHYS	IXP4XX_ETHB_PHYS
#define ETHA_NPE_ID	1
#define ETHA_PHY_ID	0
#define ETHA_ETH_ID	0
#define ETHA_RXQ_ID	27
#define ETHA_TXQ_ID	24

#define ETHB_PHYS	IXP4XX_ETHC_PHYS
#define ETHB_NPE_ID	2
#define ETHB_PHY_ID	1
#define ETHB_ETH_ID	1
#define ETHB_RXQ_ID	28
#define ETHB_TXQ_ID	25

#endif  /* __DRV-NPE_H__ */
