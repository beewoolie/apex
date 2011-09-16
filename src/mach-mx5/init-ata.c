/** @file init-ata.c

   written by Marc Singer
   16 Sep 2011

   Copyright (C) 2011 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#include <config.h>
#include <apex.h>
#include <asm/reg.h>
#include <error.h>
#include <command.h>

#include <mach/hardware.h>
#include <mach/drv-ata.h>

#if 0
/* PIO timing table */
struct timing {
  uint16_t t1;
  uint16_t t2_8;
  uint16_t t4;
  uint16_t t9;
  uint16_t tA;
};

static struct timing timings[] = {
  { .t1 = 70, .t2_8 = 290, .t4 = 30, .t9 = 20, .tA = 50 },
  { .t1 = 50, .t2_8 = 290, .t4 = 20, .t9 = 15, .tA = 50 },
  { .t1 = 30, .t2_8 = 290, .t4 = 15, .t9 = 10, .tA = 50 },
  { .t1 = 30, .t2_8 =  80, .t4 = 10, .t9 = 10, .tA = 50 },
  { .t1 = 25, .t2_8 =  70, .t4 = 10, .t9 = 10, .tA = 50 },
};
#endif


void mx5_ata_init (void)
{
//  uint32_t freq = ipg_clk ();
//  uint32_t ns = 1000*1000*1000/freq; /* Cycle time */

  ATA_CONTROL      = 0x80;
  ATA_CONTROL      = 0xc0;

  /* *** FIXME: calculation of these values not yet clear. */
  ATA_TIME_OFF     = 3;
  ATA_TIME_ON      = 3;
  ATA_TIME_1       = 2;
  ATA_TIME_2W      = 5;
  ATA_TIME_2R      = 5;
  ATA_TIME_AX      = 6;
  ATA_TIME_PIO_RDX = 1;
  ATA_TIME_4       = 1;
  ATA_TIME_9       = 1;

  ATA_CONTROL      = 0x41;
  ATA_FIFO_ALARM   = 20;
}
