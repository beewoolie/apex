/* pci.c
     $Id$

   written by Marc Singer
   8 Mar 2005

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

   This module exists solely to initialize PCI. 

*/

#include <config.h>
#include <service.h>

#include "hardware.h"

static void pci_init (void)
{
#if 0
    static int inited = 0;
    int  is_host = (*IXP425_PCI_CSR & PCI_CSR_HOST);

    if (inited)
        return;
    else
        inited = 1;
            
    // If IC is set, assume warm start
    if (*IXP425_PCI_CSR  & PCI_CSR_IC)
        return;

    // We use identity AHB->PCI address translation
    // in the 0x48000000 address space
    *IXP425_PCI_PCIMEMBASE = 0x48494A4B;

    // We also use identity PCI->AHB address translation
    // in 4 16MB BARs that begin at the physical memory start
    *IXP425_PCI_AHBMEMBASE = 0x00010203;

    if (is_host) {

        HAL_PCI_CFG_WRITE_UINT32(0, 0, CYG_PCI_CFG_BAR_0, 0x00000000);
        HAL_PCI_CFG_WRITE_UINT32(0, 0, CYG_PCI_CFG_BAR_1, 0x01000000);
        HAL_PCI_CFG_WRITE_UINT32(0, 0, CYG_PCI_CFG_BAR_2, 0x02000000);
        HAL_PCI_CFG_WRITE_UINT32(0, 0, CYG_PCI_CFG_BAR_3, 0x03000000);

        cyg_pci_set_memory_base(HAL_PCI_ALLOC_BASE_MEMORY);
        cyg_pci_set_io_base(HAL_PCI_ALLOC_BASE_IO);

        // This one should never get used, as we request the memory for
        // work with PCI with GFP_DMA, which will return mem in the first 64 MB.
        // But we still must initialize it so that it wont intersect with first 4
        // BARs
        // XXX: Should we initialize the BAR5 to some very large value, so that
        // it also will not be hit?
        //
        HAL_PCI_CFG_WRITE_UINT32(0, 0, CYG_PCI_CFG_BAR_4, 0x80000000);
        HAL_PCI_CFG_WRITE_UINT32(0, 0, CYG_PCI_CFG_BAR_5, 0x90000000);

        *IXP425_PCI_ISR = PCI_ISR_PSE | PCI_ISR_PFE | PCI_ISR_PPE | PCI_ISR_AHBE;

        //
        // Set Initialize Complete in PCI Control Register: allow IXP425 to
        // respond to PCI configuration cycles. Specify that the AHB bus is
        // operating in big endian mode. Set up byte lane swapping between 
        // little-endian PCI and the big-endian AHB bus 
        *IXP425_PCI_CSR = PCI_CSR_IC | PCI_CSR_ABE | PCI_CSR_PDS | PCI_CSR_ADS;
    
        HAL_PCI_CFG_WRITE_UINT16(0, 0, CYG_PCI_CFG_COMMAND,
                 CYG_PCI_CFG_COMMAND_MASTER | CYG_PCI_CFG_COMMAND_MEMORY);
    } else {

        // Set Initialize Complete in PCI Control Register: allow IXP425 to
        // respond to PCI configuration cycles. Specify that the AHB bus is
        // operating in big endian mode. Set up byte lane swapping between 
        // little-endian PCI and the big-endian AHB bus 
        *IXP425_PCI_CSR = PCI_CSR_IC | PCI_CSR_ABE | PCI_CSR_PDS | PCI_CSR_ADS;
    }
}


#endif
}

static __service_6 struct service_d pci_service = {
  .init = pci_init,
#if !defined (CONFIG_SMALL)
  //  .report = npe_report,
#endif
};
