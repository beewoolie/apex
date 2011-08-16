/** @file mx5-spi.h

   written by Marc Singer
   15 Jul 2011

   Copyright (C) 2011 Marc Singer

   -----------
   DESCRIPTION
   -----------

   General interface for SPI.  Because SPI does not conform to the
   standard driver interface and, more importantly, it isn't an
   interface amenable to generic IO, SPI is relegated to an internal
   interface within Apex.

*/

#if !defined (MX5_SPI_H_INCLUDED)
#    define   MX5_SPI_H_INCLUDED

/* ----- Includes */

/* ----- Types */

struct mx5_spi {
  s8   bus;                     	/* 1, 2, ... */
  s8   slave;                   	/* 0, 1, ... */
  u32  sclk_frequency;          	/* in Hz */
  u32  context;                 	/* available to select () */
  bool ss_active_high;          	/* slave-select active high */
  bool data_inactive_high;
  bool sclk_inactive_high;
  bool sclk_polarity;                   /* initial 0, rising; 1, falling edge */
  bool sclk_phase;                      /* sample on 0, first; 1, second edge */
  void (*select) (const struct mx5_spi*); /* Platform speciic slave select */
  bool talk;
};

/* ----- Globals */

/* ----- Prototypes */

extern void mx5_ecspi_transfer (const struct mx5_spi* spi,
                                void* rgb, size_t cbOut,
                                size_t cbIn, size_t cbInSkip);
extern void mx5_spi_select (const struct mx5_spi*);


#endif  /* MX5_SPI_H_INCLUDED */
