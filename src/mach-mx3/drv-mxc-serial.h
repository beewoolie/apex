/** @file drv-mxc-serial.h

   written by Marc Singer
   15 Sep 2011

   Copyright (C) 2011 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (DRV_MXC_SERIAL_H_INCLUDED)
#    define   DRV_MXC_SERIAL_H_INCLUDED

/* ----- Includes */

#include <mach/hardware.h>

/* ----- Macros */

#if defined (CONFIG_MX31_UART1)
# define UART PHYS_UART1
#endif

#define UART_BMR_115200		(5968 - 1)
#define UART_BIR_115200		(1000 - 0)

#if defined (CPLD_CTRL1_CLR)
# define INITIALIZE_CONSOLE_UART_MX31ADS \
     CPLD_CTRL1_CLR = 1<<4; CPLD_CTRL2_SET = 1<<2;
#else
# define INITIALIZE_CONSOLE_UART_MX31ADS
#endif

# define INITIALIZE_CONSOLE_UART\
  ({ MASK_AND_SET (__REG (0x43fac080), 0xffff, 0x1210); /* txd1/rxd1 */ \
    __REG (UART + UART_CR1) = 0;                                        \
    __REG (UART + UART_CR2) = UART_CR2_IRTS | UART_CR2_CTSC | UART_CR2_WS \
      | UART_CR2_TXEN | UART_CR2_RXEN;                                  \
    __REG (UART + UART_CR3) = UART_CR3_RXDMUXSEL;                       \
    __REG (UART + UART_CR4) = (32<<UART_CR4_CTSTL_SH)                   \
      | UART_CR4_LPBYP | UART_CR4_DREN;                                 \
    __REG (UART + UART_FCR) = (16<<UART_FCR_RXTL_SH)                    \
      | ( 0<<UART_FCR_RFDIV_SH)                                         \
      | (16<<UART_FCR_TXTL_SH);                                         \
    __REG (UART + UART_SR1) = ~0;                                       \
    __REG (UART + UART_SR2) = ~0;                                       \
    __REG (UART + UART_BIR) = UART_BIR_115200;                          \
    __REG (UART + UART_BMR) = UART_BMR_115200;                          \
    __REG (UART + UART_CR1) = UART_CR1_EN;                              \
    INITIALIZE_CONSOLE_UART_MX31ADS;                                    \
  })

# define INITIALIZE_UARTB\
  ({ __REG (UARTB + UART_CR1) = 0;                                      \
    __REG (UARTB + UART_CR2) = UART_CR2_IRTS | UART_CR2_CTSC | UART_CR2_WS \
      | UART_CR2_TXEN | UART_CR2_RXEN;                                  \
    __REG (UARTB + UART_CR3) = UART_CR3_RXDMUXSEL;                      \
    __REG (UARTB + UART_CR4) = (32<<UART_CR4_CTSTL_SH)                  \
      | UART_CR4_LPBYP | UART_CR4_DREN;                                 \
    __REG (UARTB + UART_FCR) = (16<<UART_FCR_RXTL_SH)                   \
      | ( 0<<UART_FCR_RFDIV_SH)                                         \
      | (16<<UART_FCR_TXTL_SH);                                         \
    __REG (UARTB + UART_SR1) = ~0;                                      \
    __REG (UARTB + UART_SR2) = ~0;                                      \
    __REG (UARTB + UART_BIR) = UART_BIR_115200;                         \
    __REG (UARTB + UART_BMR) = UART_BMR_115200;                         \
    __REG (UARTB + UART_CR1) = UART_CR1_EN;                             \
  })

/* ----- Globals */

/* ----- Prototypes */


#endif  /* DRV_MXC_SERIAL_H_INCLUDED */
