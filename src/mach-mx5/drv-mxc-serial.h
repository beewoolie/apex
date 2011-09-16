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

#if defined (CONFIG_MX5_UART1)
# define UART PHYS_UART1
#endif

#define UART_BAUDRATE_V		(115200)
#define UART_CLOCK_FREQ		(66500000) /* Static to allow early init */
#define UART_BIR_V		(16)
#define UART_BMR_V                                                      \
  ((UART_CLOCK_FREQ/2/16*UART_BIR_V + UART_BAUDRATE_V/2)/UART_BAUDRATE_V)
#define UART_RFDIV		(4) /* UART_CLK /2 as above */

/*** FIXME: no modem control lines being driven.  OK?  See CR3. */

/** UART init must be inline so that it can be incldued in the early
    initialization to support DEBUG_LL.  Under normal circumstances we
    wouldn't use a macro for such an extensive piece of code. */
# define INITIALIZE_CONSOLE_UART\
  ({   GPIO_CONFIG_FUNC (MX51_PIN_UART1_RXD, 0);                        \
    GPIO_CONFIG_PAD  (MX51_PIN_UART1_RXD,                               \
                      GPIO_PAD_HYST_EN | GPIO_PAD_PKE | GPIO_PAD_PUE    \
                      | GPIO_PAD_DRIVE_HIGH | GPIO_PAD_SLEW_FAST);      \
    GPIO_CONFIG_FUNC (MX51_PIN_UART1_TXD, 0);                           \
    GPIO_CONFIG_PAD  (MX51_PIN_UART1_TXD,                               \
                      GPIO_PAD_HYST_EN | GPIO_PAD_PKE | GPIO_PAD_PUE    \
                      | GPIO_PAD_DRIVE_HIGH | GPIO_PAD_SLEW_FAST);      \
    GPIO_CONFIG_FUNC (MX51_PIN_UART1_RTS, 0);                           \
    GPIO_CONFIG_PAD  (MX51_PIN_UART1_RTS,                               \
                      GPIO_PAD_HYST_EN | GPIO_PAD_PKE | GPIO_PAD_PUE    \
                      | GPIO_PAD_DRIVE_HIGH);                           \
    GPIO_CONFIG_FUNC (MX51_PIN_UART1_CTS, 0);                           \
    GPIO_CONFIG_PAD  (MX51_PIN_UART1_CTS,                               \
                      GPIO_PAD_HYST_EN | GPIO_PAD_PKE | GPIO_PAD_PUE    \
                      | GPIO_PAD_DRIVE_HIGH);                           \
                                                                        \
    __REG (UART + UART_CR1) = 0;                                        \
    __REG (UART + UART_CR2) = UART_CR2_IRTS /*| UART_CR2_CTSC */        \
      | UART_CR2_WS                                                     \
      | UART_CR2_TXEN | UART_CR2_RXEN                                   \
      | UART_CR2_NSRST;                                                 \
    __REG (UART + UART_CR3) = UART_CR3_RXDMUXSEL;                       \
    __REG (UART + UART_CR4) = (32<<UART_CR4_CTSTL_SH)                   \
      | UART_CR4_LPBYP /* | UART_CR4_DREN */ ;                          \
    __REG (UART + UART_FCR) = (16<<UART_FCR_RXTL_SH)                    \
      | ((UART_RFDIV)<<UART_FCR_RFDIV_SH)                               \
      | (16<<UART_FCR_TXTL_SH);                                         \
    __REG (UART + UART_SR1) = ~0;                                       \
    __REG (UART + UART_SR2) = ~0;                                       \
    __REG (UART + UART_BIR) = UART_BIR_V - 1;                           \
    __REG (UART + UART_BMR) = UART_BMR_V - 1;                           \
    __REG (UART + UART_CR1) = UART_CR1_EN;                              \
  })

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */


#endif  /* DRV_MXC_SERIAL_H_INCLUDED */
