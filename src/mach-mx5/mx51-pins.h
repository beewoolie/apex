/** @file mx51-pins.h

   written by Marc Singer
   2 Jul 2011

   Copyright (C) 2011 Marc Singer

   -----------
   DESCRIPTION
   -----------

   o There is a single IOMUX control register for every BGA pad.
   o In some case, several BGA pads share the same pad drive control
     register though usually they are distinct.

   o ub used <<29 for the GPIO_PORT, <<21 for the ALT, <<24 for the GPIO_PIN
     <<0 for the MUX_R, and <<10 for the PAD_R
     This means that there are 10 bits for the MUX_R, 11 for PAD_R,
     5 for GPIO_PIN, 3 for GPIO_ALT, and 3 for the GPIO_PORT

   o We're going to use 10 each for the MUX_R and PAD_R,
     5 for GPIO_PIN because those are theoretical maximums, 1K pins
     and 32 bits per word.  That leaves 3 for ALT which is the
     minimum, and 3 for the GPIO_PORT which we need because there is
     one encoding for no port.  That leaves one free bit.


      31   30     28   27    23   22    20   19     10   9       0
     +---+-----------+----------+----------+-----------+-----------+
     | 0 | GPIO_PORT | GPIO_PIN | GPIO_ALT | IOPAD_REG | IOMUX_REG |
     +---+-----------+----------+----------+-----------+-----------+

     GPIO_PORT: 0 none, 1-4; mask 0x3   shift 28
     GPIO_PIN:  0-31;        mask 0x1f  shift 23
     GPIO_ALT:  0-7;         mask 0x7   shift 20
     IOPAD_REG: 0 none, 1-x; mask 0x3ff shift 10
     IOMUX_REG: 0 none, 1-x; mask 0x3ff shift  0

*/

#if !defined (MX51_PINS_H_INCLUDED)
#    define   MX51_PINS_H_INCLUDED

#define IOMUX_R_BASE (0x01c)
#define IOPAD_R_BASE (0x3f0)

#define NO_IOMUX	(0)
#define NO_IOPAD	(0)
#define NO_GPIO_PORT    (0)

/* Register fields are, at most, 12 bits, 10 when we eliminate the
   lowest two bits that are always zero.  The GPIO_R field is 2 bits,
   the GPIO_P field is 5 bits, and the GPIO_A field is 3 bits
*/

#define R__(r,b,z,s)                                          \
  (((((r) == (z)) ? 0 : ((r) - (b))/4 + 1) & 0x3ff) << (s))

#define P__(mux_r,pad_r,gpio_r,gpio_p,gpio_a)                        \
  (0                                                                 \
  | R__(mux_r, IOMUX_R_BASE, NO_IOMUX,  0)                           \
  | R__(pad_r, IOPAD_R_BASE, NO_IOPAD, 10)                           \
  | ((((gpio_r) == NO_GPIO_PORT) ? 0 : ((gpio_r) & 0x3)) << 28)      \
  | (((gpio_p) & 0x1f) << 23)                                        \
  | (((gpio_a) & 0x07) << 20))

#define N_(mux_r, pad_r) \
  P__ (mux_r, pad_r, NO_GPIO_PORT, 0, 0)
#define G_(mux_r, pad_r, gpio_r, gpio_p, gpio_a)        \
  P__ (mux_r, pad_r, gpio_r, gpio_p, gpio_a)

enum {
  MX51_PIN_EIM_DA0        = N_ (0x01c, 0x7a8),
  MX51_PIN_EIM_DA1        = N_ (0x020, 0x7a8),
  MX51_PIN_EIM_DA2        = N_ (0x024, 0x7a8),
  MX51_PIN_EIM_DA3        = N_ (0x028, 0x7a8),
  MX51_PIN_EIM_DA4        = N_ (0x02c, 0x7ac),
  MX51_PIN_EIM_DA5        = N_ (0x030, 0x7ac),
  MX51_PIN_EIM_DA6        = N_ (0x034, 0x7ac),
  MX51_PIN_EIM_DA7        = N_ (0x038, 0x7ac),
  MX51_PIN_EIM_DA8        = N_ (0x03c, 0x7b0),
  MX51_PIN_EIM_DA9        = N_ (0x040, 0x7b0),
  MX51_PIN_EIM_DA10       = N_ (0x044, 0x7b0),
  MX51_PIN_EIM_DA11       = N_ (0x048, 0x7b0),
  MX51_PIN_EIM_DA12       = N_ (0x04c, 0x7bc),
  MX51_PIN_EIM_DA13       = N_ (0x050, 0x7bc),
  MX51_PIN_EIM_DA14       = N_ (0x054, 0x7bc),
  MX51_PIN_EIM_DA15       = N_ (0x058, 0x7bc),
  MX51_PIN_EIM_D16        = G_ (0x05c, 0x3f0, 2,  0, 1),
  MX51_PIN_EIM_D17        = G_ (0x060, 0x3f4, 2,  1, 1),
  MX51_PIN_EIM_D18        = G_ (0x064, 0x3f8, 2,  2, 1),
  MX51_PIN_EIM_D19        = G_ (0x068, 0x3fc, 2,  3, 1),
  MX51_PIN_EIM_D20        = G_ (0x06c, 0x400, 2,  4, 1),
  MX51_PIN_EIM_D21        = G_ (0x070, 0x404, 2,  5, 1),
  MX51_PIN_EIM_D22        = G_ (0x074, 0x408, 2,  6, 1),
  MX51_PIN_EIM_D23        = G_ (0x078, 0x40c, 2,  7, 1),
  MX51_PIN_EIM_D24        = G_ (0x07c, 0x410, 2,  8, 1),
  MX51_PIN_EIM_D25        = N_ (0x080, 0x414),
  MX51_PIN_EIM_D26        = N_ (0x084, 0x418),
  MX51_PIN_EIM_D27        = G_ (0x088, 0x41c, 2,  9, 1),
  MX51_PIN_EIM_D28        = N_ (0x08c, 0x420),
  MX51_PIN_EIM_D29        = N_ (0x090, 0x424),
  MX51_PIN_EIM_D30        = N_ (0x094, 0x428),
  MX51_PIN_EIM_D31        = N_ (0x098, 0x42c),
  MX51_PIN_EIM_A16        = G_ (0x09c, 0x430, 2, 10, 1),
  MX51_PIN_EIM_A17        = G_ (0x0a0, 0x434, 2, 11, 1),
  MX51_PIN_EIM_A18        = G_ (0x0a4, 0x438, 2, 12, 1),
  MX51_PIN_EIM_A19        = G_ (0x0a8, 0x43c, 2, 13, 1),
  MX51_PIN_EIM_A20        = G_ (0x0ac, 0x440, 2, 14, 1),
  MX51_PIN_EIM_A21        = G_ (0x0b0, 0x444, 2, 15, 1),
  MX51_PIN_EIM_A22        = G_ (0x0b4, 0x448, 2, 16, 1),
  MX51_PIN_EIM_A23        = G_ (0x0b8, 0x44c, 2, 17, 1),
  MX51_PIN_EIM_A24        = G_ (0x0bc, 0x450, 2, 18, 1),
  MX51_PIN_EIM_A25        = G_ (0x0c0, 0x454, 2, 19, 1),
  MX51_PIN_EIM_A26        = G_ (0x0c4, 0x458, 2, 20, 1),
  MX51_PIN_EIM_A27        = G_ (0x0c8, 0x45c, 2, 21, 1),
  MX51_PIN_EIM_EB0        = N_ (0x0cc, 0x460),
  MX51_PIN_EIM_EB1        = N_ (0x0d0, 0x464),
  MX51_PIN_EIM_EB2        = G_ (0x0d4, 0x468, 2, 22, 1),
  MX51_PIN_EIM_EB3        = G_ (0x0d8, 0x46c, 2, 23, 1),
  MX51_PIN_EIM_OE         = G_ (0x0dc, 0x470, 2, 24, 1),
  MX51_PIN_EIM_CS0        = G_ (0x0e0, 0x474, 2, 25, 1),
  MX51_PIN_EIM_CS1        = G_ (0x0e4, 0x478, 2, 26, 1),
  MX51_PIN_EIM_CS2        = G_ (0x0e8, 0x47c, 2, 27, 1),
  MX51_PIN_EIM_CS3        = G_ (0x0ec, 0x480, 2, 28, 1),
  MX51_PIN_EIM_CS4        = G_ (0x0f0, 0x484, 2, 29, 1),
  MX51_PIN_EIM_CS5        = G_ (0x0f4, 0x488, 2, 30, 1),
  MX51_PIN_EIM_DTACK      = G_ (0x0f8, 0x48c, 2, 31, 1),
  MX51_PIN_EIM_LBA        = G_ (0x0fc, 0x494, 3,  1, 1),
  MX51_PIN_EIM_CRE        = G_ (0x100, 0x4a0, 3,  2, 1),
  MX51_PIN_DRAM_CS1       = N_ (0x104, 0x4d0),
  MX51_PIN_NANDF_WE_B     = G_ (0x108, 0x4e4, 3,  3, 3),
  MX51_PIN_NANDF_RE_B     = G_ (0x10c, 0x4e8, 3,  4, 3),
  MX51_PIN_NANDF_ALE      = G_ (0x110, 0x4ec, 3,  5, 3),
  MX51_PIN_NANDF_CLE      = G_ (0x114, 0x4f0, 3,  6, 3),
  MX51_PIN_NANDF_WP_B     = G_ (0x118, 0x4f4, 3,  7, 3),
  MX51_PIN_NANDF_RB0      = G_ (0x11c, 0x4f8, 3,  8, 3),
  MX51_PIN_NANDF_RB1      = G_ (0x120, 0x4fc, 3,  9, 3),
  MX51_PIN_NANDF_RB2      = G_ (0x124, 0x500, 3, 10, 3),
  MX51_PIN_NANDF_RB3      = G_ (0x128, 0x504, 3, 11, 3),
  MX51_PIN_GPIO_NAND      = G_ (0x12c, 0x514, 3, 12, 3),
  MX51_PIN_NANDF_RB4      = MX51_PIN_GPIO_NAND,
  MX51_PIN_NANDF_RB5      = G_ (0x130, 0x5d8, 3, 13, 3),
  MX51_PIN_NANDF_RB6      = G_ (0x134, 0x5dc, 3, 14, 3),
  MX51_PIN_NANDF_RB7      = G_ (0x138, 0x5e0, 3, 15, 3),
  MX51_PIN_NANDF_CS0      = G_ (0x130, 0x518, 3, 16, 3),
  MX51_PIN_NANDF_CS1      = G_ (0x134, 0x51c, 3, 17, 3),
  MX51_PIN_NANDF_CS2      = G_ (0x138, 0x520, 3, 18, 3),
  MX51_PIN_NANDF_CS3      = G_ (0x13c, 0x524, 3, 19, 3),
  MX51_PIN_NANDF_CS4      = G_ (0x140, 0x528, 3, 20, 3),
  MX51_PIN_NANDF_CS5      = G_ (0x144, 0x52c, 3, 21, 3),
  MX51_PIN_NANDF_CS6      = G_ (0x148, 0x530, 3, 22, 3),
  MX51_PIN_NANDF_CS7      = G_ (0x14c, 0x534, 3, 23, 3),
  MX51_PIN_NANDF_RDY_INT  = G_ (0x150, 0x538, 3, 24, 3),
  MX51_PIN_NANDF_D15      = G_ (0x154, 0x53c, 3, 25, 3),
  MX51_PIN_NANDF_D14      = G_ (0x158, 0x540, 3, 26, 3),
  MX51_PIN_NANDF_D13      = G_ (0x15c, 0x544, 3, 27, 3),
  MX51_PIN_NANDF_D12      = G_ (0x160, 0x548, 3, 28, 3),
  MX51_PIN_NANDF_D11      = G_ (0x164, 0x54c, 3, 29, 3),
  MX51_PIN_NANDF_D10      = G_ (0x168, 0x550, 3, 30, 3),
  MX51_PIN_NANDF_D9       = G_ (0x16c, 0x554, 3, 31, 3),
  MX51_PIN_NANDF_D8       = G_ (0x170, 0x558, 4,  0, 3),
  MX51_PIN_NANDF_D7       = G_ (0x174, 0x55c, 4,  1, 3),
  MX51_PIN_NANDF_D6       = G_ (0x178, 0x560, 4,  2, 3),
  MX51_PIN_NANDF_D5       = G_ (0x17c, 0x564, 4,  3, 3),
  MX51_PIN_NANDF_D4       = G_ (0x180, 0x568, 4,  4, 3),
  MX51_PIN_NANDF_D3       = G_ (0x184, 0x56c, 4,  5, 3),
  MX51_PIN_NANDF_D2       = G_ (0x188, 0x570, 4,  6, 3),
  MX51_PIN_NANDF_D1       = G_ (0x18c, 0x574, 4,  7, 3),
  MX51_PIN_NANDF_D0       = G_ (0x190, 0x578, 4,  8, 3),
  MX51_PIN_CSI1_D8        = G_ (0x194, 0x57c, 3, 12, 3),
  MX51_PIN_CSI1_D9        = G_ (0x198, 0x580, 3, 13, 3),
  MX51_PIN_CSI1_D10       = N_ (0x19c, 0x584),
  MX51_PIN_CSI1_D11       = N_ (0x1a0, 0x588),
  MX51_PIN_CSI1_D12       = N_ (0x1a4, 0x58c),
  MX51_PIN_CSI1_D13       = N_ (0x1a8, 0x590),
  MX51_PIN_CSI1_D14       = N_ (0x1ac, 0x594),
  MX51_PIN_CSI1_D15       = N_ (0x1b0, 0x598),
  MX51_PIN_CSI1_D16       = N_ (0x1b4, 0x59c),
  MX51_PIN_CSI1_D17       = N_ (0x1b8, 0x5a0),
  MX51_PIN_CSI1_D18       = N_ (0x1bc, 0x5a4),
  MX51_PIN_CSI1_D19       = N_ (0x1c0, 0x5a8),
  MX51_PIN_CSI1_VSYNC     = G_ (0x1c4, 0x5ac, 3, 14, 3),
  MX51_PIN_CSI1_HSYNC     = G_ (0x1c8, 0x5b0, 3, 15, 3),
  MX51_PIN_CSI1_PIXCLK    = N_ (NO_IOMUX, 0x5B4),
  MX51_PIN_CSI1_MCLK      = N_ (NO_IOMUX, 0x5B8),
  MX51_PIN_CSI1_PKE0      = N_ (NO_IOMUX, 0x860),
  MX51_PIN_CSI2_D12       = G_ (0x1cc, 0x5bc, 4,  9, 3),
  MX51_PIN_CSI2_D13       = G_ (0x1d0, 0x5c0, 4, 10, 3),
  MX51_PIN_CSI2_D14       = G_ (0x1d4, 0x5c4, 4, 11, 3),
  MX51_PIN_CSI2_D15       = G_ (0x1d8, 0x5c8, 4, 12, 3),
  MX51_PIN_CSI2_D16       = G_ (0x1dc, 0x5cc, 4, 11, 3),
  MX51_PIN_CSI2_D17       = G_ (0x1e0, 0x5d0, 4, 12, 3),
  MX51_PIN_CSI2_D18       = G_ (0x1e4, 0x5d4, 4, 11, 3),
  MX51_PIN_CSI2_D19       = G_ (0x1e8, 0x5d8, 4, 12, 3),
  MX51_PIN_CSI2_VSYNC     = G_ (0x1ec, 0x5dc, 4, 13, 3),
  MX51_PIN_CSI2_HSYNC     = G_ (0x1f0, 0x5e0, 4, 14, 3),
  MX51_PIN_CSI2_PIXCLK    = G_ (0x1f4, 0x5e4, 4, 15, 3),
  MX51_PIN_CSI2_PKE0      = N_ (NO_IOMUX, 0x81C),
  MX51_PIN_I2C1_CLK       = G_ (0x1f8, 0x5e8, 4, 16, 3),
  MX51_PIN_I2C1_DAT       = G_ (0x1fc, 0x5ec, 4, 17, 3),
  MX51_PIN_AUD3_BB_TXD    = G_ (0x200, 0x5f0, 4, 18, 3),
  MX51_PIN_AUD3_BB_RXD    = G_ (0x204, 0x5f4, 4, 19, 3),
  MX51_PIN_AUD3_BB_CK     = G_ (0x208, 0x5f8, 4, 20, 3),
  MX51_PIN_AUD3_BB_FS     = G_ (0x20c, 0x5fc, 4, 21, 3),
  MX51_PIN_CSPI1_MOSI     = G_ (0x210, 0x600, 4, 22, 3),
  MX51_PIN_CSPI1_MISO     = G_ (0x214, 0x604, 4, 23, 3),
  MX51_PIN_CSPI1_SS0      = G_ (0x218, 0x608, 4, 24, 3),
  MX51_PIN_CSPI1_SS1      = G_ (0x21c, 0x60c, 4, 25, 3),
  MX51_PIN_CSPI1_RDY      = G_ (0x220, 0x610, 4, 26, 3),
  MX51_PIN_CSPI1_SCLK     = G_ (0x224, 0x614, 4, 27, 3),
  MX51_PIN_UART1_RXD      = G_ (0x228, 0x618, 4, 28, 3),
  MX51_PIN_UART1_TXD      = G_ (0x22c, 0x61c, 4, 29, 3),
  MX51_PIN_UART1_RTS      = G_ (0x230, 0x620, 4, 30, 3),
  MX51_PIN_UART1_CTS      = G_ (0x234, 0x624, 4, 31, 3),
  MX51_PIN_UART2_RXD      = G_ (0x238, 0x628, 1, 20, 3),
  MX51_PIN_UART2_TXD      = G_ (0x23c, 0x62c, 1, 21, 3),
  MX51_PIN_UART3_RXD      = G_ (0x240, 0x630, 1, 22, 3),
  MX51_PIN_UART3_TXD      = G_ (0x244, 0x634, 1, 23, 3),
  MX51_PIN_OWIRE_LINE     = G_ (0x248, 0x638, 1, 24, 3),
  MX51_PIN_KEY_ROW0       = N_ (0x24c, 0x63c),
  MX51_PIN_KEY_ROW1       = N_ (0x250, 0x640),
  MX51_PIN_KEY_ROW2       = N_ (0x254, 0x644),
  MX51_PIN_KEY_ROW3       = N_ (0x258, 0x648),
  MX51_PIN_KEY_COL0       = N_ (0x25c, 0x64c),
  MX51_PIN_KEY_COL1       = N_ (0x260, 0x650),
  MX51_PIN_KEY_COL2       = N_ (0x264, 0x654),
  MX51_PIN_KEY_COL3       = N_ (0x268, 0x658),
  MX51_PIN_KEY_COL4       = N_ (0x26c, 0x65c),
  MX51_PIN_KEY_COL5       = N_ (0x270, 0x660),
  MX51_PIN_USBH1_CLK      = G_ (0x278, 0x678, 1, 25, 2),
  MX51_PIN_USBH1_DIR      = G_ (0x27c, 0x67c, 1, 26, 2),
  MX51_PIN_USBH1_STP      = G_ (0x280, 0x680, 1, 27, 2),
  MX51_PIN_USBH1_NXT      = G_ (0x284, 0x684, 1, 28, 2),
  MX51_PIN_USBH1_DATA0    = G_ (0x288, 0x688, 1, 11, 2),
  MX51_PIN_USBH1_DATA1    = G_ (0x28c, 0x68c, 1, 12, 2),
  MX51_PIN_USBH1_DATA2    = G_ (0x290, 0x690, 1, 13, 2),
  MX51_PIN_USBH1_DATA3    = G_ (0x294, 0x694, 1, 14, 2),
  MX51_PIN_USBH1_DATA4    = G_ (0x298, 0x698, 1, 15, 2),
  MX51_PIN_USBH1_DATA5    = G_ (0x29c, 0x69c, 1, 16, 2),
  MX51_PIN_USBH1_DATA6    = G_ (0x2a0, 0x6a0, 1, 17, 2),
  MX51_PIN_USBH1_DATA7    = G_ (0x2a4, 0x6a4, 1, 18, 2),
  MX51_PIN_DI1_PIN11      = G_ (0x2a8, 0x6a8, 3,  0, 4),
  MX51_PIN_DI1_PIN12      = G_ (0x2ac, 0x6ac, 3,  1, 4),
  MX51_PIN_DI1_PIN13      = G_ (0x2b0, 0x6b0, 3,  2, 4),
  MX51_PIN_DI1_D0_CS      = G_ (0x2b4, 0x6b4, 3,  3, 4),
  MX51_PIN_DI1_D1_CS      = G_ (0x2b8, 0x6b8, 3,  4, 4),
  MX51_PIN_DISPB2_SER_DIN = G_ (0x2bc, 0x6bc, 3,  5, 4),
  MX51_PIN_DISPB2_SER_DIO = G_ (0x2c0, 0x6c0, 3,  6, 4),
  MX51_PIN_DISPB2_SER_CLK = G_ (0x2c4, 0x6c4, 3,  7, 4),
  MX51_PIN_DISPB2_SER_RS  = G_ (0x2c8, 0x6c8, 3,  8, 4),
  MX51_PIN_DISP1_DAT0     = N_ (0x2cc, 0x6cc),
  MX51_PIN_DISP1_DAT1     = N_ (0x2d0, 0x6d0),
  MX51_PIN_DISP1_DAT2     = N_ (0x2d4, 0x6d4),
  MX51_PIN_DISP1_DAT3     = N_ (0x2d8, 0x6d8),
  MX51_PIN_DISP1_DAT4     = N_ (0x2dc, 0x6dc),
  MX51_PIN_DISP1_DAT5     = N_ (0x2e0, 0x6e0),
  MX51_PIN_DISP1_DAT6     = N_ (0x2e4, 0x6e4),
  MX51_PIN_DISP1_DAT7     = N_ (0x2e8, 0x6e8),
  MX51_PIN_DISP1_DAT8     = N_ (0x2ec, 0x6ec),
  MX51_PIN_DISP1_DAT9     = N_ (0x2f0, 0x6f0),
  MX51_PIN_DISP1_DAT10    = N_ (0x2f4, 0x6f4),
  MX51_PIN_DISP1_DAT11    = N_ (0x2f8, 0x6f8),
  MX51_PIN_DISP1_DAT12    = N_ (0x2fc, 0x6fc),
  MX51_PIN_DISP1_DAT13    = N_ (0x300, 0x700),
  MX51_PIN_DISP1_DAT14    = N_ (0x304, 0x704),
  MX51_PIN_DISP1_DAT15    = N_ (0x308, 0x708),
  MX51_PIN_DISP1_DAT16    = N_ (0x30c, 0x70c),
  MX51_PIN_DISP1_DAT17    = N_ (0x310, 0x710),
  MX51_PIN_DISP1_DAT18    = N_ (0x314, 0x714),
  MX51_PIN_DISP1_DAT19    = N_ (0x318, 0x718),
  MX51_PIN_DISP1_DAT20    = N_ (0x31c, 0x71c),
  MX51_PIN_DISP1_DAT21    = N_ (0x320, 0x720),
  MX51_PIN_DISP1_DAT22    = N_ (0x324, 0x724),
  MX51_PIN_DISP1_DAT23    = N_ (0x328, 0x728),
  MX51_PIN_DI1_PIN3       = N_ (0x32c, 0x72c),
  MX51_PIN_DI1_PIN2       = N_ (0x330, 0x734),
  MX51_PIN_DI_GP1         = N_ (0x334, 0x73c),
  MX51_PIN_DI_GP2         = N_ (0x338, 0x740),
  MX51_PIN_DI_GP3         = N_ (0x33c, 0x744),
  MX51_PIN_DI2_PIN4       = N_ (0x340, 0x748),
  MX51_PIN_DI2_PIN2       = N_ (0x344, 0x74c),
  MX51_PIN_DI2_PIN3       = N_ (0x348, 0x750),
  MX51_PIN_DI2_DISP_CLK   = N_ (0x34c, 0x754),
  MX51_PIN_DI_GP4         = N_ (0x350, 0x758),
  MX51_PIN_DISP2_DAT0     = N_ (0x354, 0x75c),
  MX51_PIN_DISP2_DAT1     = N_ (0x358, 0x760),
  MX51_PIN_DISP2_DAT2     = N_ (0x35c, 0x764),
  MX51_PIN_DISP2_DAT3     = N_ (0x360, 0x768),
  MX51_PIN_DISP2_DAT4     = N_ (0x364, 0x76c),
  MX51_PIN_DISP2_DAT5     = N_ (0x368, 0x770),
  MX51_PIN_DISP2_DAT6     = G_ (0x36c, 0x774, 1, 19, 5),
  MX51_PIN_DISP2_DAT7     = G_ (0x370, 0x778, 1, 29, 5),
  MX51_PIN_DISP2_DAT8     = G_ (0x374, 0x77c, 1, 30, 5),
  MX51_PIN_DISP2_DAT9     = G_ (0x378, 0x780, 1, 31, 5),
  MX51_PIN_DISP2_DAT10    = N_ (0x37c, 0x784),
  MX51_PIN_DISP2_DAT11    = N_ (0x380, 0x788),
  MX51_PIN_DISP2_DAT12    = N_ (0x384, 0x78c),
  MX51_PIN_DISP2_DAT13    = N_ (0x388, 0x790),
  MX51_PIN_DISP2_DAT14    = N_ (0x38c, 0x794),
  MX51_PIN_DISP2_DAT15    = N_ (0x390, 0x798),
  MX51_PIN_SD1_CMD        = N_ (0x394, 0x79c),
  MX51_PIN_SD1_CLK        = N_ (0x398, 0x7a0),
  MX51_PIN_SD1_DATA0      = N_ (0x39c, 0x7a4),
  MX51_PIN_SD1_DATA1      = N_ (0x3a0, 0x7a8),
  MX51_PIN_SD1_DATA2      = N_ (0x3a4, 0x7ac),
  MX51_PIN_SD1_DATA3      = N_ (0x3a8, 0x7b0),
  MX51_PIN_GPIO1_0        = G_ (0x3ac, 0x7b4, 1,  0, 1),
  MX51_PIN_GPIO1_1        = G_ (0x3b0, 0x7b8, 1,  1, 1),
  MX51_PIN_SD2_CMD        = N_ (0x3b4, 0x7bc),
  MX51_PIN_SD2_CLK        = N_ (0x3b8, 0x7c0),
  MX51_PIN_SD2_DATA0      = N_ (0x3bc, 0x7c4),
  MX51_PIN_SD2_DATA1      = N_ (0x3c0, 0x7c8),
  MX51_PIN_SD2_DATA2      = N_ (0x3c4, 0x7cc),
  MX51_PIN_SD2_DATA3      = N_ (0x3c8, 0x7d0),
  MX51_PIN_GPIO1_2        = G_ (0x3cc, 0x7d4, 1,  2, 0),
  MX51_PIN_GPIO1_3        = G_ (0x3d0, 0x7d8, 1,  3, 0),
  MX51_PIN_PMIC_INT_REQ   = N_ (0x3d4, 0x7fc),
  MX51_PIN_GPIO1_4        = G_ (0x3d8, 0x804, 1,  4, 0),
  MX51_PIN_GPIO1_5        = G_ (0x3dc, 0x808, 1,  5, 0),
  MX51_PIN_GPIO1_6        = G_ (0x3e0, 0x80c, 1,  6, 0),
  MX51_PIN_GPIO1_7        = G_ (0x3e4, 0x810, 1,  7, 0),
  MX51_PIN_GPIO1_8        = G_ (0x3e8, 0x814, 1,  8, 0),
  MX51_PIN_GPIO1_9        = G_ (0x3ec, 0x818, 1,  9, 0),
};

#undef P__
#undef R__
#undef G_
#undef N_

#define _PIN_MUX_R(v)   ((((((v) >>  0) & 0x3ff) - 1)*4) + IOMUX_R_BASE)
#define _PIN_PAD_R(v)   ((((((v) >> 10) & 0x3ff) - 1)*4) + IOPAD_R_BASE)
#define _PIN_GPIO_R(v)  (((v) >> 28) & 3)
#define _PIN_GPIO_P(v)  (((v) >> 23) & 0x1f)
#define _PIN_GPIO_A(v)  (((v) >> 20) & 7)
#define _PIN_IS_GPIO(v) (_PIN_GPIO_R(v) != 0)
#define _PIN_IS_MUX(v)	((((v) >> 0) & 0x3ff) != 0)

#define _GPIO_DR(i)	__REG(PHYS_GPIOX(i) + 0x00)
#define _GPIO_GDIR(i)	__REG(PHYS_GPIOX(i) + 0x04)
#define _GPIO_PSR(i)	__REG(PHYS_GPIOX(i) + 0x08)
#define _GPIO_ICR1(i)	__REG(PHYS_GPIOX(i) + 0x0c)
#define _GPIO_ICR2(i)	__REG(PHYS_GPIOX(i) + 0x10)
#define _GPIO_IMR(i)	__REG(PHYS_GPIOX(i) + 0x14)
#define _GPIO_ISR(i)	__REG(PHYS_GPIOX(i) + 0x18)

#define GPIO_PAD_PKE	    (1<<7)
#define GPIO_PAD_PUE	    (1<<6)
#define GPIO_PAD_PD_100K    (0<<4)
#define GPIO_PAD_PU_47K     (1<<4)
#define GPIO_PAD_PU_100K    (2<<4)
#define GPIO_PAD_PU_22K     (3<<4)
#define GPIO_PAD_SLEW_SLOW  (0<<0)
#define GPIO_PAD_SLEW_FAST  (1<<0)
#define GPIO_PAD_DRIVE_LOW  (0<<1)
#define GPIO_PAD_DRIVE_MED  (1<<1)
#define GPIO_PAD_DRIVE_HIGH (2<<1)
#define GPIO_PAD_DRIVE_MAX  (3<<1)
#define GPIO_PAD_HYST_EN    (1<<8)
#define GPIO_PAD_OPEN_DRAIN (1<<3)
#define GPIO_PAD_DRIVE_HIGHVOLT (1<<13)

#define GPIO_PIN_FUNC_SION	(1<<4)

#define GPIO_CONFIG_PAD(p,v) \
  (__REG(PHYS_IOMUXC + _PIN_PAD_R(p)) = (v))
#define GPIO_CONFIG_FUNC(p,v) \
  (__REG(PHYS_IOMUXC + _PIN_MUX_R(p)) = (v))

#define GPIO_CONFIG_OUTPUT(p)\
  (GPIO_CONFIG_FUNC(p, _PIN_GPIO_A(p)),                         \
   _GPIO_GDIR(_PIN_GPIO_R (p)) |=  (1 << _PIN_GPIO_P (p)))
#define GPIO_CONFIG_INPUT(p)\
  (GPIO_CONFIG_FUNC(p, _PIN_GPIO_A(p)),                         \
   _GPIO_GDIR(_PIN_GPIO_R (p)) &= ~(1 << _PIN_GPIO_P (p)))

#define GPIO_SET(p)\
	(_GPIO_DR(_PIN_GPIO_R (p))   |=   1 << _PIN_GPIO_P (p))
#define GPIO_CLEAR(p)\
	(_GPIO_DR(_PIN_GPIO_R (p))   &= ~(1 << _PIN_GPIO_P (p)))
#define GPIO_VALUE(p)\
	((_GPIO_DR(_PIN_GPIO_R (p)) & (1 << _PIN_GPIO_P (p))) != 0)

#endif  /* MX51_PINS_H_INCLUDED */
