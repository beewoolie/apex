/** @file drv-mx5-esdhc.c

   written by Marc Singer
   6 Aug 2011

   Copyright (C) 2011 Marc Singer

   -----------
   DESCRIPTION
   -----------

   SD/MMC controller driver.

   NOTES
   =====

   DMA
   ---

   The DMA IP block is limited to transfers within a 1K aligned region
   of memory.  There is a section defined to enforce this alignment.
   We're having limited success with DMA.  We don't *really* care
   about it so it's problems will not prevent progress.


   XFER_LEN
   --------

   Using a block_len greater than 512B means that the eSDHC IP block
   has to make more than one transfer.  We are supposed to monitor the
   CRC error bit and detect failed transfers.  I know, obnoxious.
   We're sticking with polled transfers because DMA seems to be more
   prone to failed transfers.  Probably, we don't yet have the DMA
   transfer properly setup.  Oddly, it looks as if the transfer
   reaches the final block.  The DMA address is at the full extent of
   the transfer.  Yet, the second 512B are FFs.

   Card Registers
   --------------

   FYI, these are the card register names and sizes.

     Name	Bits	Description
     ----	----	-----------
     CID	 128	Card Identification Register
     RCA	  16	Relative Card Address
     DSR	  16	Driver Stage Register (optional)
     CSD	 128	Card Specific Data
     SCR	  64	SD Configuration Register
     OCR	  32	Operation Conditions Register
     SSR	 512	SD Status Register
     CSR	  32	Card Status Register

*/

#define USE_WIDE4	      /* Permit Use of 4 bit IO with capable cards */
//#define USE_DMA
//#define USE_MULTIBLOCK_READ     /* Unimplemented */

//#define TALK 2

#include <config.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <apex.h>
#include <command.h>
#include <mach/hardware.h>
#include <console.h>
#include <error.h>

#include <debug_ll.h>

#include <mmc.h>

#include <talk.h>

#define SYSCTL_INITA	(1<<27)
#define SYSCTL_RSTA	(1<<24)
#define SYSCTL_SDCLKEN	(1<< 3)
#define SYSCTL_PEREN	(1<< 2)
#define SYSCTL_HCKEN	(1<< 1)
#define SYSCTL_IPGEN	(1<< 0)

#define PRSSTAT_BREN	(1<<11)
#define PRSSTAT_BWEN	(1<<10)
#define PRSSTAT_RTA	(1<< 9)
#define PRSSTAT_WTA	(1<< 8)
#define PRSSTAT_SDOFF	(1<< 7)
#define PRSSTAT_PEROFF	(1<< 6)
#define PRSSTAT_HCKOFF	(1<< 5)
#define PRSSTAT_IPGOFF	(1<< 4)
#define PRSSTAT_SDSTB	(1<< 3)
#define PRSSTAT_DLA	(1<< 2)
#define PRSSTAT_CDIHB	(1<< 1)
#define PRSSTAT_CIHB	(1<< 0)

#define IRQ_DMAE	(1<<28)
#define IRQ_AC12E       (1<<24)
#define IRQ_DEBE        (1<<22)
#define IRQ_DCE         (1<<21)
#define IRQ_DTOE        (1<<20)
#define IRQ_CIE         (1<<19)
#define IRQ_CEBE        (1<<18)
#define IRQ_CCE         (1<<17)
#define IRQ_CTOE        (1<<16)
#define IRQ_CINT        (1<< 8)
#define IRQ_CRM         (1<< 7)
#define IRQ_CINS        (1<< 6)
#define IRQ_BRR         (1<< 5)
#define IRQ_BWR         (1<< 4)
#define IRQ_DINT        (1<< 3)
#define IRQ_BGE         (1<< 2)
#define IRQ_TC          (1<< 1)
#define IRQ_CC          (1<< 0)

#define LOW_SPEED_CLK	(400*1000)
//#define HIGH_SPEED_CLK  (20*1000*1000)
#define HIGH_SPEED_CLK  (25*1000*1000)

#define MS_ACQUIRE_DELAY	(10)
#define MS_ACQUIRE_TIMEOUT      (250)
#define C_ACQUIRE_TRIES         (40)

#define C_SIZE		csd_short(62, 12)
#define C_SIZE_2	csd_long(48, 22)
#define C_SIZE_MULT	csd_byte(47, 3)
#define NSAC		csd_byte(104, 8)
#define READ_BL_LEN	csd_byte(80, 4)
#define SECTOR_SIZE	csd_byte(39, 7)
#define TAAC		csd_byte(112, 8)
#define TRAN_SPEED	csd_byte(96, 8)

//#define MMC_SECTOR_SIZE_MAX 512	/* *** FIXME: should come from card */
#define MMC_SECTOR_SIZE_MAX 1024

#define IRQ_BITS (~0)
/*
#define IRQ_BITS (0                                                     \
                  | IRQ_CC | IRQ_TC | IRQ_BWR | IRQ_BRR | IRQ_CINT | IRQ_CTOE \
                  | IRQ_CCE | IRQ_CEBE | IRQ_CIE | IRQ_DTOE | IRQ_DCE   \
                  | IRQ_DINT                                            \
                  | IRQ_DEBE )
*/

struct mmc_info {
//  u32 response[4];		/* Most recent response */
  uint8_t  response[128/8];	/* Most recent response, MSB order */
  uint32_t ocr;                 /* OCR of acquired card */
  uint8_t  cid[128/8];          /* CID of acquired card */
  uint8_t  csd[128/8];          /* CSD of acquired card */
  uint8_t  scr[ 64/8];          /* SCR of acquired card */
  uint8_t  ssr[512/8];          /* SSR of acquired card */
  uint32_t rca;                 /* Relative address assigned to card */
  int      version;             /* Card version */
  int      acquire_time;	/* Count of delays to acquire card */
  bool     width4;              /* 4 bit IO enabled */
  bool     sd;                  /* SD card detected */
  bool     hc;                  /* High capacity version 2.0 card */
  bool     acquired;            /* Boolean true when card acquired */

  int      c_size;
  int      c_size_mult;
  int      read_bl_len;
  int      mult;
  int      blocknr;
  int      block_len;
  int      xfer_len;
  int      cb_cache;
  uint32_t block_count;
  uint32_t ib;                  /* Index of cached data */
} mmc;


#if defined (USE_MULTIBLOCK_READ)
# define CB_CACHE_MAX	(2*MMC_SECTOR_SIZE_MAX)
#else
# define CB_CACHE_MAX	(MMC_SECTOR_SIZE_MAX)
#endif

uint8_t __xbss(mmc) mmc_rgb[CB_CACHE_MAX];

bool mmc_card_acquired (void) {
  return mmc.acquired; }

void mmc_clear (void) {
  memset (&mmc, 0, sizeof (mmc));
  mmc.ib = -1;
}

uint32_t ocr_host (void) {
  uint32_t ocr = MMC_OCR_CCS;        /* We support high-capacity */
  /* Low voltage card selection bit doesn't appear to be supported by
     all cards.  Setting this bit breaks card detection for some
     cards. */
//  if (ESDHCx_HOSTCAPBLT(1) & (1<<26))
//    ocr |= 1<<7;                /* VDD_195_165 */
  if (ESDHCx_HOSTCAPBLT(1) & (1<<25))
    ocr |= 1<<18;               /* VDD_31_30 */
  if (ESDHCx_HOSTCAPBLT(1) & (1<<24))
    ocr |= 1<<20;               /* VDD_34_33 */
  return ocr;
}

void mmc_cd_gpio (bool as_gpio)
{
  if (as_gpio) {
    GPIO_CONFIG_INPUT (PIN_SDHC1_CD);
    GPIO_CONFIG_PAD   (PIN_SDHC1_CD,
                       GPIO_PAD_HYST_EN
                       | GPIO_PAD_PKE
                       | GPIO_PAD_PUE
                       | GPIO_PAD_PU_100K
                       | GPIO_PAD_SLEW_SLOW);
  }
  else {
    GPIO_CONFIG_FUNC (PIN_SDHC1_CD, 0 | GPIO_PIN_FUNC_SION);
    GPIO_CONFIG_PAD  (PIN_SDHC1_CD,
                      GPIO_PAD_DRIVE_HIGH | GPIO_PAD_PKE
                      | GPIO_PAD_HYST_EN | GPIO_PAD_PU_100K
                      | GPIO_PAD_SLEW_FAST);
  }
}

bool mmc_present (void)
{
  uint8_t present = 0;
  int i;
  mmc_cd_gpio (true);
  for (i = 0; i < 8; ++i)
    present = (present << 1) | (GPIO_VALUE (PIN_SDHC1_CD) ? 0 : 1);

  mmc_cd_gpio (false);

  return present == (uint8_t) ~0;
}

static uint8_t response_byte (int bit, int length)
{
  uint16_t v = (   (mmc.response[15 - bit/8    ]
                 | (mmc.response[15 - bit/8 - 1] <<  8)) >> (bit%8))
    & ((1<<length) - 1);
  return v;
}

static uint16_t response_short (int bit, int length)
{
  uint32_t v = (   (mmc.response[15 - bit/8    ]
                 | (mmc.response[15 - bit/8 - 1] <<  8)
                 | (mmc.response[15 - bit/8 - 2] << 16)) >> (bit%8))
    & ((1<<length) - 1);
  return v;
}

static inline uint32_t response_ocr (void) {
  return 0
    | (mmc.response[0] << 24)
    | (mmc.response[1] << 16)
    | (mmc.response[2] << 8)
    |  mmc.response[3];
}

static inline uint32_t response_rca (void) {
  return response_short (16, 16);
}

/** return eight or fewer bits from the CSD register where the bits
    are present in one or two bytes read from the register.  The
    register holds field bits in MSB order which means that bit 0 is
    LSB of the last byte of the register byte array. */

static uint8_t csd_byte (int bit, int length)
{
  uint16_t v = (   (mmc.csd[15 - bit/8    ]
                 | (mmc.csd[15 - bit/8 - 1] <<  8)) >> (bit%8))
    & ((1<<length) - 1);
  return v;
}

/** return sixteen or fewer bits from the CSD register where the bits
    are present in no more than three bytes read from the register.
    The register holds field bits in MSB order which means that bit 0
    is LSB of the last byte of the register byte array. */

static uint16_t csd_short (int bit, int length)
{
  uint32_t v = (   (mmc.csd[15 - bit/8    ]
                 | (mmc.csd[15 - bit/8 - 1] <<  8)
                 | (mmc.csd[15 - bit/8 - 2] << 16)) >> (bit%8))
    & ((1<<length) - 1);
  return v;
}

/** return thirty-two or fewer bits from the CSD register where the
    bits are present in no more than five bytes read from the
    register.  The register holds field bits in MSB order which means
    that bit 0 is LSB of the last byte of the register byte array. */

static uint32_t csd_long (int bit, int length)
{
  uint32_t v = 0
    |    (uint32_t) (mmc.csd[15 - bit/8 - 0]) >> (     bit%8)
    |    (uint32_t) (mmc.csd[15 - bit/8 - 1]) << ( 8 - bit%8);
  if (length + bit%8 > 16)
    v |= (uint32_t) (mmc.csd[15 - bit/8 - 2]) << (16 - bit%8);
  if (length + bit%8 > 24)
    v |= (uint32_t) (mmc.csd[15 - bit/8 - 3]) << (24 - bit%8);
  if (length + bit%8 > 32)
    v |= (uint32_t) (mmc.csd[15 - bit/8 - 4]) << (32 - bit%8);
  v &= (1<<length) - 1;

  return v;
}

static uint32_t xfertyp_from_cmd (uint32_t cmd)
{
  uint32_t xfertyp = ((cmd >> CMD_SHIFT_CMD) & CMD_MASK_CMD) << 24;

  switch ((cmd >> CMD_SHIFT_RESPLEN) & CMD_MASK_RESPLEN) {
  default:
  case 0:
    break;
  case CMD_RESP_136:
    xfertyp |= 1 << 16;         /* RSPTYP */
    break;
  case CMD_RESP_48:
    xfertyp |= 2 << 16;         /* RSPTYP */
    break;
  }
  if (cmd & CMD_BIT_RESPWAIT)
    xfertyp |= 1 << 16;         /* RSPTYP */

  if (cmd & CMD_BIT_RESPCRC)
    xfertyp |= 1 << 19;         /* CCCEN */

  if (cmd & CMD_BIT_RESPOPCODE)
    xfertyp |= 1 << 20;         /* CICEN */

  if (cmd & CMD_BIT_DATA)
    xfertyp |= 1 << 21;         /* DPSEL */

  if (!(cmd & CMD_BIT_WRITE))
    xfertyp |= 1 << 4;          /* DTDSEL */

#if defined (USE_DMA)
  if (cmd & CMD_BIT_DATA)
    xfertyp |= 1 << 0;          /* DMAEN */
#endif

  return xfertyp;
}

const char* decode_cmd (uint32_t cmd, uint32_t arg)
{
  static char sz[128];
  const char* name = 0;

  switch (((cmd >> CMD_SHIFT_CMD) & CMD_MASK_CMD)
          | ((cmd & CMD_BIT_APP) ? (1<<8) : 0)) {
  case MMC_GO_IDLE_STATE:	      name = "GO_IDLE";       break;
  case MMC_SEND_OP_COND:              name = "SEND_OP_COND";  break;
  case MMC_ALL_SEND_CID:              name = "ALL_SEND_CID";  break;
  case MMC_SET_RELATIVE_ADDR:         name = "SET_RCA";       break;
  case MMC_SET_DSR:                   name = "SET_DSR";       break;
  case MMC_SELECT_CARD:               name = "SELECT_CARD";   break;
  case SD_SEND_IF_COND:               name = "SEND_IF_COND";  break;
  case MMC_SEND_CSD:                  name = "SEND_CSD";      break;
  case MMC_SEND_CID:                  name = "SEND_CID";      break;
  case MMC_STOP_TRANSMISSION:         name = "STOP";          break;
  case MMC_SEND_STATUS:               name = "SEND_STATUS";   break;
  case MMC_GO_INACTIVE_STATE:         name = "GO_INACTIVE";   break;
  case MMC_SET_BLOCKLEN:              name = "SET_BLOCKLEN";  break;
  case MMC_READ_SINGLE_BLOCK:         name = "READ_SINGLE";   break;
  case MMC_READ_MULTIPLE_BLOCK:       name = "READ_MULTIPLE"; break;
  case MMC_WRITE_DAT_UNTIL_STOP:      name = "WRITE_STREAM";  break;
  case MMC_SET_BLOCK_COUNT:           name = "SET_BLOCKCNT";  break;
  case MMC_WRITE_BLOCK:               name = "WRITE_SINGLE";  break;
  case MMC_WRITE_MULTIPLE_BLOCK:      name = "WRITE_MULT";    break;
  case MMC_PROGRAM_CID:               name = "PROG_CID";      break;
  case MMC_PROGRAM_CSD:               name = "PROG_CSD";      break;
  case MMC_SET_WRITE_PROT:            name = "SET_WP";        break;
  case MMC_CLR_WRITE_PROT:            name = "CLR_WP";        break;
  case MMC_SEND_WRITE_PROT:           name = "SEND_WP";       break;
  case SD_ERASE:                      name = "SD_ERASE";      break;
  case MMC_ERASE:                     name = "MMC_ERASE";     break;
  case MMC_FAST_IO:                   name = "FAST_IO";       break;
  case MMC_GO_IRQ_STATE:              name = "GO_IRQ";        break;
  case MMC_LOCK_UNLOCK:               name = "LOCK_UNLOCK";   break;
//  case SD_SEND_RELATIVE_ADDR:         name = "SD_SEND_RCA";   break;
  case SD_APP_SET_BUS_WIDTH | (1<<8): name = "SD_SET_WIDTH";  break;
  case SD_APP_STATUS        | (1<<8): name = "SD_STATUS";     break;
  case SD_APP_OP_COND       | (1<<8): name = "SD_OP_COND";    break;
  case SD_APP_SEND_SCR      | (1<<8): name = "SD_SEND_SCR";   break;
  }

  snprintf (sz, sizeof (sz), "cmd %d (0x%x)%s (%s) arg %d (0x%x) => R%d(%d)",
            (cmd >> CMD_SHIFT_CMD) & CMD_MASK_CMD,
            (cmd >> CMD_SHIFT_CMD) & CMD_MASK_CMD,
            (cmd & CMD_BIT_APP) ? "*" : "",
            name ? name : "?",
            arg, arg,
            (cmd >> CMD_SHIFT_RESPR) & CMD_MASK_RESPR,
            (cmd >> CMD_SHIFT_RESPLEN) & CMD_MASK_RESPLEN
            );
  return sz;
}

const char* decode_irq (uint32_t irq)
{
  static char sz[128];

  snprintf (sz, sizeof (sz), "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
            (irq & IRQ_DMAE)  ? " DMAE" : "",
            (irq & IRQ_AC12E) ? " AC12E" : "",
            (irq & IRQ_DEBE)  ? " DEBE" : "",
            (irq & IRQ_DCE)   ? " DCE" : "",
            (irq & IRQ_DTOE)  ? " DTOE" : "",
            (irq & IRQ_CIE)   ? " CIE" : "",
            (irq & IRQ_CEBE)  ? " CEBE" : "",
            (irq & IRQ_CCE)   ? " CCE" : "",
            (irq & IRQ_CTOE)  ? " CTOE" : "",
            (irq & IRQ_CINT)  ? " CINT" : "",
            (irq & IRQ_CRM)   ? " CRM" : "",
            (irq & IRQ_CINS)  ? " CINS" : "",
            (irq & IRQ_BRR)   ? " BRR" : "",
            (irq & IRQ_BWR)   ? " BWR" : "",
            (irq & IRQ_DINT)  ? " DINT" : "",
            (irq & IRQ_BGE)   ? " BGE" : "",
            (irq & IRQ_TC)    ? " TC" : "",
            (irq & IRQ_CC)    ? " CC" : "");

  return sz;
}

/* current_state
   0 idle
   1 ready
   2 ident
   3 stby
   4 tran
   5 data
   6 rcv
   7 prg
   8 dis

*/

const char* decode_response_r1 (void)
{
  static char sz[128];

  snprintf (sz, sizeof (sz), "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s st-%d%s%s%s",
            response_byte (31, 1) ? " OOR" : "",
            response_byte (30, 1) ? " ADER" : "",
            response_byte (29, 1) ? " BLER" : "",
            response_byte (28, 1) ? " ESER" : "",
            response_byte (27, 1) ? " ERPM" : "",
            response_byte (26, 1) ? " WPVI" : "",
            response_byte (25, 1) ? " CRLK" : "",
            response_byte (24, 1) ? " LUFA" : "",
            response_byte (23, 1) ? " CRCE" : "",
            response_byte (22, 1) ? " ILLC" : "",
            response_byte (21, 1) ? " ECCE" : "",
            response_byte (20, 1) ? " CCER" : "",
            response_byte (19, 1) ? " ERR" : "",
            response_byte (16, 1) ? " CSDO" : "",
            response_byte (15, 1) ? " WPES" : "",
            response_byte (14, 1) ? " ECCD" : "",
            response_byte (13, 1) ? " ERES" : "",
            response_byte ( 9, 4),
            response_byte ( 8, 1) ? " RDYD" : "",
            response_byte ( 5, 1) ? " APPC" : "",
            response_byte ( 3, 1) ? " AKEE" : "");

  return sz;
}


void decode_response (uint32_t cmd)
{
#define b_(i) ((mmc.response[15 - i/8] >> (i%8)) & 1)
  switch ((cmd >> CMD_SHIFT_RESPR) & CMD_MASK_RESPR) {
  default:
    return;
  case 1:
    DBG (1,"R1:%s\n", decode_response_r1 ());
    break;

  case 2:
//    printf("resp R2\n");
//    dumpw (mmc.response, sizeof (mmc.response), 0, 0);
    switch ((cmd >> CMD_SHIFT_CMD) & CMD_MASK_CMD) {
    case MMC_ALL_SEND_CID:
    case MMC_SEND_CID:
#if 0
      printf ("  mid %d  oid %c%c  prod %c%c%c%c%c  rev %d  psn 0x%x  mdt %x\n",
              response_byte (120,8),
              response_byte (112,8),
              response_byte (104,8),
              response_byte (96,8),
              response_byte (88,8),
              response_byte (80,8),
              response_byte (72,8),
              response_byte (64,8),
              response_byte (56,8),
              (response_short (40, 16) << 16)
              | response_short (24, 16),
              response_short (8, 12));
#endif
      break;
    case MMC_SEND_CSD:
      break;
    default:
      break;
    }
    break;

  case 3:
#if 0
    printf("resp R3");
    if (b_(31))
      printf (" BSY");
    if (b_(30))
      printf (" CCS");
    if (b_(23))
      printf (" 3.6");
    if (b_(22))
      printf (" 3.5");
    if (b_(21))
      printf (" 3.4");
    if (b_(20))
      printf (" 3.3");
    if (b_(19))
      printf (" 3.2");
    if (b_(18))
      printf (" 3.1");
    if (b_(17))
      printf (" 3.0");
    if (b_(16))
      printf (" 2.9");
    if (b_(15))
      printf (" 2.8");
    printf ("\n");
#endif
    break;

  case 6:                       /* RCA */
#if 0
    printf ("resp R6");
    printf (" RCA %d (0x%x)  status 0x%04x\n",
            response_rca (), response_rca (),
            (mmc.response[2] << 8) | mmc.response[3]);
#endif
    break;

  case 7:                       /* Card IFC */
    DBG (1, "resp R7: %x\n", 0
         | (mmc.response[0] << 24)
         | (mmc.response[1] << 16)
         | (mmc.response[2] <<  8)
         | (mmc.response[3] <<  0));
    break;
  }
}

void mx5_esdhc_setup_clock (int frequency)
{
  static uint32_t frequency_last;
  uint32_t clock = esdhcx_clk (1);
  int sdclkfs = 0;
  int dvs;

  if (frequency == frequency_last)
    return;

  DBG (1, "setup_clock %d\n", frequency);

  while (clock/frequency > 16 && sdclkfs < 8) {
    ++sdclkfs;
    clock >>= 1;
  }
  sdclkfs = (1 << sdclkfs) >> 1;
  dvs = clock/frequency - 1;

  /* Disable clock and set dividers */
  ESDHCx_SYSCTL(1) &= ~SYSCTL_SDCLKEN;
  MASK_AND_SET (ESDHCx_SYSCTL(1), (0xff << 8) | (0xf << 4),
                (sdclkfs << 8) | (dvs << 4));

  // *** FIXME: Do we even needs this sleep?
//  mdelay(100);

  // V1?
//  ESDHCx_SYSCTL(1) |= SYSCTL_PEREN;

  /* Wait for clock to stabilize and then enable */
  while (!(ESDHCx_PRSSTAT(1) & PRSSTAT_SDSTB))
    ;
  ESDHCx_SYSCTL(1) |= SYSCTL_SDCLKEN;

  frequency_last = frequency;
}

int mx5_esdhc_execute (uint32_t cmd, uint32_t arg, size_t cb)
{
  int state   = (cmd & CMD_BIT_APP) ? 99 : 0;
  int status  = MMC_RES_OK;
  uint32_t irqstat;
  uint32_t xfertyp = 0;

  DBG (1, "execute cmd 0x%x 0x%x\n", cmd, arg);

  mx5_esdhc_setup_clock ((cmd & CMD_BIT_LS) ? LOW_SPEED_CLK : HIGH_SPEED_CLK);

  ESDHCx_IRQSIGEN(1)   = 0;     /* Mask all interrupts */
  ESDHCx_IRQSTATEN(1) |= IRQ_BITS;

 top:
  /* *** FIXME: need to handle this better.  We only want to
     *** interface with the IP block when a command is about to be
     *** sent. */
  if (state != 1) {
    while (ESDHCx_PRSSTAT(1) & (PRSSTAT_CIHB | PRSSTAT_CDIHB | PRSSTAT_DLA))
      ;

    /* Wait 8 SD clocks, though I'm not sure where this requirement
       comes from.  8 SD clocks at 400KHz is 20us. */
    udelay (40);                  /* 8 clocks at 400KHz */
  }

  DBG (1, "  exec state %d\n", state);
  if (state == 0)
    DBG (1, "  %s, rca 0x%x\n", decode_cmd (cmd, arg), mmc.rca);

  switch (state) {

  case 0:                       /* Send basic command */
    if (cmd & CMD_BIT_DATA) {
      uint32_t wml = mmc.xfer_len/4;
      if (cmd & CMD_BIT_WRITE)
        wml <<= 16;
      DBG (2, "wml 0x%x\n", wml);
      ESDHCx_WML(1) = wml;
      MASK_AND_SET (ESDHCx_SYSCTL(1), 0xf<<16, 14); /* Timeout 2^27 clocks */
      ESDHCx_BLKATTR(1) = 0
        | (1<<16)               /* Block count */
        | cb;                   /* Block size */
      DBG (2, "blkattr 0x%lx\n", ESDHCx_BLKATTR(1));

      ESDHCx_PROCTL(1) = 0
        | 0x20                        		/* Little endian */
        | (mmc.width4 ? (1<<1) : (0<<1));       /* 4/1 bit width */

#if defined (USE_DMA)
      ESDHCx_DSADDR(1) = (uint32_t) mmc_rgb;
#endif
    }

    ESDHCx_CMDARG(1) = arg;
    xfertyp = xfertyp_from_cmd (cmd);
    ++state;
    break;

  case 1:                       /* Return status */
    return status;

  case 99:                      /* Send APP command prefix */
    ESDHCx_CMDARG(1) = mmc.rca << 16;
    xfertyp = xfertyp_from_cmd (CMD_APP);
    state = 0;
    break;
  }

  {
    //    uint32_t irqstat_last;
    ESDHCx_IRQSTAT(1)  = ~0;      /* Clear all IRQ status' */
//    irqstat_last       = ESDHCx_IRQSTAT(1);
    ESDHCx_XFERTYP(1)  = xfertyp;
    DBG (1, "  irqstat %lx\n", ESDHCx_IRQSTAT(1));

    DBG (1, "  xfertyp 0x%08x  cmd %d (0x%02x)\n", xfertyp,
         (xfertyp >> 24) & 0xff, (xfertyp >> 24) & 0xff);

    /* Wait for completion */
    {
      uint32_t time = timer_read ();
      do {
        irqstat = ESDHCx_IRQSTAT (1);
//        if (irqstat != irqstat_last) {
//          DBG (1, "  -> irq %x\n", irqstat);
//          irqstat_last = irqstat;
//        }
      } while (!(irqstat & IRQ_CC) && timer_delta (time, timer_read () < 300));
      if (!(irqstat & IRQ_CC))
        return MMC_RES_TIMEOUT;
    }
  }

//  irqstat = ESDHCx_IRQSTAT (1); /* One more read, probably redundant */
  /* Don't clear irqstat since we need to know if the BRR bit is set.  */
//  ESDHCx_IRQSTAT(1) = irqstat;
  if (irqstat & IRQ_CTOE)
    status |= MMC_RES_TIMEOUT;
  if (irqstat & IRQ_CIE)
    status |= MMC_RES_CMD_ERR;
  if (irqstat & (IRQ_CCE | IRQ_CEBE))
    status |= MMC_RES_CRC_ERR;
  DBG (1, "  irqstat %x%s\n", irqstat, decode_irq (irqstat));

  switch ((cmd >> CMD_SHIFT_RESPLEN) & CMD_MASK_RESPLEN) {
  case CMD_RESP_NONE:
    break;
  case CMD_RESP_136:
    /* Put MSB first */
    {
      uint32_t r;
      r = ESDHCx_CMDRSP0(1);
      mmc.response[15] = 0;
      mmc.response[14] =  r        & 0xff;
      mmc.response[13] = (r >>  8) & 0xff;
      mmc.response[12] = (r >> 16) & 0xff;
      mmc.response[11] = (r >> 24) & 0xff;

      r = ESDHCx_CMDRSP1(1);
      mmc.response[10] =  r        & 0xff;
      mmc.response[9]  = (r >>  8) & 0xff;
      mmc.response[8]  = (r >> 16) & 0xff;
      mmc.response[7]  = (r >> 24) & 0xff;

      r = ESDHCx_CMDRSP2(1);
      mmc.response[6]  =  r         & 0xff;
      mmc.response[5]  = (r >>  8) & 0xff;
      mmc.response[4]  = (r >> 16) & 0xff;
      mmc.response[3]  = (r >> 24) & 0xff;

      r = ESDHCx_CMDRSP3(1);
      mmc.response[2]  =  r        & 0xff;
      mmc.response[1]  = (r >>  8) & 0xff;
      mmc.response[0]  = (r >> 16) & 0xff;
//      dumpw (mmc.response, 16, 0, 0);
    }
    break;
  case CMD_RESP_48:
    {
      uint32_t r = ESDHCx_CMDRSP0(1);
      mmc.response[0] = (r >> 24) & 0xff;
      mmc.response[1] = (r >> 16) & 0xff;
      mmc.response[2] = (r >>  8) & 0xff;
      mmc.response[3] =  r        & 0xff;
      memcpy (&mmc.response[12], &mmc.response[0], 4); // Dup 32 bit resp
      DBG (1, "  resp 0x%02x%02x%02x%02x\n", mmc.response[0],
           mmc.response[1], mmc.response[2], mmc.response[3]);
    }
    break;
  }

  decode_response (cmd);        /* Show response */

//  ESDHCx_IRQSTAT(1) = ~0;

  goto top;
}

void mx5_esdhc_init (void)
{
  /* Reset and wait for reset to complete */
  ESDHCx_SYSCTL(1) |= SYSCTL_RSTA;
  while (ESDHCx_SYSCTL(1) & SYSCTL_RSTA)
    ;

  /* *** FIXME: old hardware? V1? */
  //  ESDHCx_SYSCTL(1) |= SYSCTL_HCKEN | SYSCTL_IPGEN;

  mx5_esdhc_setup_clock (400*1000); /* Initial, slow clock */

  ESDHCx_PROCTL(1) = 0x20;      /* System reset state, little endian */

  while (ESDHCx_PRSSTAT(1) & (PRSSTAT_CIHB | PRSSTAT_CDIHB))
    ;

  // Card insertion detection would go here.

  // Initialize card, if there is one
  ESDHCx_SYSCTL(1) |= SYSCTL_INITA;
  while (ESDHCx_SYSCTL(1) & SYSCTL_INITA)
    ;
}


/** read data from an SD/MMC card register.  Unlike the data read,
    this command only reads the whole register in one pass.  It will
    not use DMA even if it is available. */

ssize_t mx5_esdhc_read_register (uint32_t cmd, void* pv, size_t cb)
{
  int status;

  status = mx5_esdhc_execute (CMD_DESELECT_CARD, 0, 0);
  status = mx5_esdhc_execute (CMD_SELECT_CARD, mmc.rca<<16, 0);
  if (mmc.width4)
    mx5_esdhc_execute (CMD_SD_SET_WIDTH, 2, 0);	/* SD, 4 bit width */

  status = mx5_esdhc_execute (cmd, 0, cb);
  DBG (2, "irqstat 0x%lx\n", ESDHCx_IRQSTAT(1));
  DBG (2, "cb %d  xter_len %d\n", cb, mmc.xfer_len);
  {
    int i;
    uint32_t irqstat;
    uint8_t* pb = pv;
    ESDHCx_IRQSTATEN(1) |= IRQ_BRR; /* Probably redundant */
    while (!((irqstat = ESDHCx_IRQSTAT(1)) & IRQ_BRR))
      ;           /* *** FIXME: need to check for timeout */
    for (i = cb/4; i--; ) {
      uint32_t v = ESDHCx_DATPORT(1);
      DBG (2, "rr: 0x%x\n", v);
      *pb++ = (v >>  0) & 0xff;
      *pb++ = (v >>  8) & 0xff;
      *pb++ = (v >> 16) & 0xff;
      *pb++ = (v >> 24) & 0xff;
    }
//    dumpw (pv, cb, 0, 0);
  }

  return cb;
}


int mmc_acquire (void)
{
  int           state   = 0;
  int           tries   = 0;
  uint32_t      command = 0;
  int           status;
  uint32_t      ocr     = 0;
  uint32_t      r;
  unsigned long time;

  mmc_clear ();

  status = mx5_esdhc_execute (CMD_IDLE, 0, 0);
  if (status) {
    DBG (1, "failed to execute CMD_IDLE\n");
    return status;
  }

  status = mx5_esdhc_execute (CMD_SD_IF_COND, (1<<8) | 0xaa, 0);
//  if (status) {
//    DBG (1, "failed to execute CMD_SD_IF_COND\n");
//    return status;
//  }
  /* *** FIXME: I suppose we should check the return to make sure it
     *** matches what we sent. */

  while (state < 100) {
    DBG (1, "acquire state %d\n", state);

    switch (state) {

    case 0:
      mmc.acquired = false;
      command      = CMD_SD_OP_COND;
      tries        = 10;
      mmc.sd       = true;
      ++state;
      break;

    case 10:
      command      = CMD_MMC_OP_COND;
      tries        = 10;
      mmc.sd       = false;
      ++state;
      break;

    case 1:
    case 11:
      status = mx5_esdhc_execute (command, ocr_host (), 0);
      if (status & MMC_RES_TIMEOUT)
        state += 8;
      else
        ++state;
      break;

    case 2:
    case 12:
      ocr = response_ocr ();
      if (ocr & MMC_OCR_READY)
        ++state;
      else
        state += 2;
      break;

    case 3:
    case 13:
      while ((ocr & MMC_OCR_READY) && --tries > 0) {
        mdelay (MS_ACQUIRE_DELAY);
        status = mx5_esdhc_execute (command, ocr_host (), 0);
        ocr = response_ocr ();
      }
      if (ocr & MMC_OCR_READY)
        state += 6;
      else
        ++state;
      break;

    case 4:
    case 14:
      time   = timer_read ();
      tries  = C_ACQUIRE_TRIES;
      ocr   &= ocr_host ();
      do {
        mdelay (MS_ACQUIRE_DELAY);
        status = mx5_esdhc_execute (command, ocr, 0);
        r = response_ocr ();
      } while (!(r & MMC_OCR_READY) && --tries > 0
               && timer_delta (time, timer_read ()) < MS_ACQUIRE_TIMEOUT);
      if (r & MMC_OCR_READY) {
        mmc.ocr = r;
        mmc.acquire_time += timer_delta (time, timer_read ());
        ++state;
        DBG (1, "acquired (%d tries)\n", C_ACQUIRE_TRIES - tries);
      }
      else {
        state += 5;
        DBG (1, "failed to acquire (%d tries)\n", C_ACQUIRE_TRIES - tries);
      }
      break;

    case 5:
    case 15:
      /* All cards send CID.  After RCA assignment/fetch, SEND_CID can
         be repeated to discover all of the cards on the bus.  Timeout
         on ALL_SEND_CID means that no unidentified cards are on the
         bus.  */
      status = mx5_esdhc_execute (CMD_ALL_SEND_CID, 0, 0);
      if (status)
        return status;
      memcpy (mmc.cid, mmc.response, 16);
      ++state;
      break;

    case 6:
      status = mx5_esdhc_execute (CMD_SD_SEND_RCA, 0, 0);
      if (status)
        return status;
      mmc.rca = response_rca ();
      ++state;
      break;

    case 16:                    /* RCA assignment */
      mmc.rca = 1;
      status = mx5_esdhc_execute (CMD_MMC_SET_RCA, mmc.rca << 16, 0);
      if (status)
        return status;
      ++state;
      break;

    case 7:
    case 17:
      status = mx5_esdhc_execute (CMD_SEND_CSD, mmc.rca << 16, 0);
      if (status)
        return status;
      memcpy (mmc.csd, mmc.response, 16);
      mmc.acquired = 1;
      ++state;
      break;

    case 8:
      state = 100;
      break;

    case 18:
      state = 100;
      break;

    case 9:
      ++state;			/* Continue with MMC */
      break;

    case 19:
    case 20:
      state = 999;
      return MMC_RES_FAILURE;
      break;			/* No cards */
    }
  }

  if (MMC_CSD_VER(mmc.csd) == MMC_CSD_VER_1) {
    mmc.c_size      = C_SIZE;
    mmc.c_size_mult = C_SIZE_MULT;
    mmc.mult        = 1<<(mmc.c_size_mult + 2);
    mmc.read_bl_len = READ_BL_LEN;
    mmc.block_len   = 1<<mmc.read_bl_len;
    mmc.blocknr     = (mmc.c_size + 1)*mmc.mult;
    mmc.block_count = mmc.blocknr*(mmc.block_len/512);
  }
  else {
    mmc.c_size      = C_SIZE_2;
    mmc.block_count = (mmc.c_size + 1)*1024;
  }
  mmc.block_len   = 512;        /* Many problems avoided with 512B reads */
  mmc.cb_cache    = mmc.block_len;
  {
    uint32_t wml = mmc.block_len/4;
    if (wml > 0x80)
      wml = 0x80;
//    wml = 4;                  /* *** FIXME: debug */
//    wml = 16;
    mmc.xfer_len = wml*4;
  }

  if (mmc.sd) {
    mx5_esdhc_read_register (CMD_SD_SEND_SCR, &mmc.scr, sizeof (mmc.scr));
    mmc.version = MMC_SCR_SPEC (mmc.scr);
  }

#if defined (USE_WIDE4)
  if (MMC_SCR_BUS_WIDTH (mmc.scr) & MMC_SCR_BUS_WIDTH_4)
    mmc.width4 = true;
#endif

  if ((mmc.ocr & MMC_OCR_CCS) && MMC_SCR_SPEC (mmc.scr) >= MMC_SCR_SPEC_2_0)
    mmc.hc = true;

  return MMC_RES_OK;
}

void mx5_esdhc_report (void)
{
//  printf ("  esdhc:  prs 0x%08lx", ESDHCx_PRSSTAT(1));
//  printf ("  cap 0x%08lx  host 0x%08lx\n",
//          ESDHCx_HOSTCAPBLT(1), ESDHCx_HOSTVER(1));
  printf ("  mmc/sd: %s, %s%s card acquired (v%d)",
          mmc_present () ? "card present" : "no card preset",
          mmc.acquired ? (mmc.sd ? "sd" : "mmc") : "no",
          mmc.hc ? "hc" : "",
          mmc.version);
  if (mmc.acquired) {
    printf (", rca 0x%x (%d ms)", mmc.rca, mmc.acquire_time);
    if (mmc.block_count > 1024*1024*2)
        printf (", %d.%02d GiB\n",
                mmc.block_count/(1024*1024*2),
                (((mmc.block_count/(1024*2))%1024)*100)/1024);
      else
        printf (", %d.%02d MiB\n",
                mmc.block_count/(1024*2),
                (((mmc.block_count/2)%1024)*100)/1024);
    printf ("          %d B sectors, %d sectors/block, C_SIZE %d,"
            " C_SIZE_MULT %d\n",
            1<<READ_BL_LEN, SECTOR_SIZE + 1,
            mmc.c_size, mmc.c_size_mult);
//    printf ("          TAAC 0x%x  NSAC %d  TRAN_SPEED 0x%x\n",
//            TAAC, NSAC, TRAN_SPEED);
//    printf ("          xfer_len %d\n", mmc.xfer_len);
//    printf ("cid\n");
//    dump (mmc.cid, sizeof (mmc.cid), 0);
//    printf ("csd\n");
//    dump (mmc.csd, sizeof (mmc.csd), 0);
//    printf ("scr\n");
//    dump (mmc.scr, sizeof (mmc.scr), 0);
//    printf ("\n");
  }
  else
    printf ("\n");
}

static int mx5_esdhc_open (struct descriptor_d* d)
{
  DBG (2,"%s: opened %d %d\n", __FUNCTION__, d->start, d->length);

  if (!mmc.acquired)
    ERROR_RETURN (ERROR_IOFAILURE, "no card");

  return 0;
}


/* mx5_esdhc_read

   performs the read of data from the SD/MMC card.  It handles
   unaligned, and sub-block length I/O.

*/

ssize_t mx5_esdhc_read (struct descriptor_d* d, void* pv, size_t cb)
{
  ssize_t cbRead = 0;
  int status;

#if 0
  PUTHEX_LL (d->index);
  PUTC_LL ('/');
  PUTHEX_LL (d->start);
  PUTC_LL ('/');
  PUTHEX_LL (d->length);
  PUTC_LL ('/');
  PUTHEX_LL (cb);
  PUTC_LL ('\r');
  PUTC_LL ('\n');
#endif

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  //  DBG (1, "%s: %ld %ld %d; %ld\n",
  //       __FUNCTION__, d->start + d->index, d->length, cb, mmc.ib);

#if 0
 {
   unsigned long sp;
   __asm volatile ("mov %0, sp\n\t"
		   : "=r" (sp));
   PUTHEX_LL (sp);
 }
#endif

  while (cb) {
    unsigned long index        = d->start + d->index;
    int           availableMax = mmc.cb_cache - (index & (mmc.cb_cache - 1));
    int           available    = cb;

#if 0
    PUTC_LL ('X');
    PUTHEX_LL (index);
    PUTC_LL ('/');
    PUTHEX_LL (cbRead);
    PUTC_LL ('\r');
    PUTC_LL ('\n');
#endif

    if (available > availableMax)
      available = availableMax;

    //    DBG (1, "%ld %ld\n", mmc.ib, index);

    if (   mmc.ib == -1
	|| mmc.ib >= index  + mmc.cb_cache
        || index  >= mmc.ib + mmc.cb_cache) {

      mmc.ib = index & ~(mmc.cb_cache - 1);
//      memset (mmc_rgb, 0xe5, sizeof (mmc_rgb)); /* Clear */

      DBG (1, "%s: read av %d  avM %d  ind %ld  cb %d\n", __FUNCTION__,
	   available, availableMax, index, cb);

      status = mx5_esdhc_execute (CMD_DESELECT_CARD, 0, 0);
      status = mx5_esdhc_execute (CMD_SELECT_CARD, mmc.rca<<16, 0);

      if (mmc.width4)
	mx5_esdhc_execute (CMD_SD_SET_WIDTH, 2, 0);	/* SD, 4 bit width */

      status = mx5_esdhc_execute (CMD_SET_BLOCKLEN, mmc.block_len, 0);

		/* Fill the sector cache */
      {
	int i = 0;
        memset (mmc_rgb, 0xe5, sizeof (mmc_rgb)); /* Clear */
	for (i = 0; i < 1 /* mmc.cb_cache/MMC_SECTOR_SIZE */; ++i) {
	  if (i == 0) {
	    DBG (3, "%s: first sector\n", __FUNCTION__);
	    status = mx5_esdhc_execute (
#if defined (USE_MULTIBLOCK_READ)
//				 CMD_READ_MULTIPLE
#else
				 CMD_READ_SINGLE
#endif
				 ,
                                 mmc.hc ? mmc.ib/512 : mmc.ib,
                                 mmc.block_len);
	  }
	  else {
	    DBG (3, "%s: next sector\n", __FUNCTION__);
//	    start_clock ();
//	    status = wait_for_completion (  MMC_STATUS_TRANDONE
//					  | MMC_STATUS_FIFO_FULL
//					  | MMC_STATUS_TOREAD);
	  }

          if (status) {
	    if (status & MMC_RES_TIMEOUT)
              ERROR_RETURN (ERROR_TIMEOUT, "timeout on read");
            ERROR_RETURN (ERROR_FAILURE, "failure");
          }

#if defined (USE_DMA)
          {
            uint32_t time = timer_read ();
            uint32_t irqstat;
            while (!((irqstat = ESDHCx_IRQSTAT(1)) & IRQ_DINT)
                   && timer_delta (time, timer_read ()) < 500)
              ;
            DBG (1, "irqstat %x%s  DMAADDR %lx (%x)\n",
                 irqstat, decode_irq (irqstat),
                 ESDHCx_DSADDR(1), (uint32_t) mmc_rgb);
          }
#else
          {
            int xfer;
            int i;
            uint32_t irqstat;
            uint8_t* pb = &mmc_rgb[0];
            ESDHCx_IRQSTATEN(1) |= IRQ_BRR; /* Probably redundant */
            for (xfer = mmc.block_len/mmc.xfer_len; xfer--; ) {
//              irqstat = ESDHCx_IRQSTAT(1);
//              DBG (1, "xfer %d irqstat 0x%x%s\n",
//                   xfer, irqstat, decode_irq (irqstat));

              while (!((irqstat = ESDHCx_IRQSTAT(1)) & IRQ_BRR))
                ;           /* *** FIXME: need to check for timeout */
              for (i = mmc.xfer_len/4; i--; ) {
                uint32_t v = ESDHCx_DATPORT(1);
                *pb++ = (v >>  0) & 0xff;
                *pb++ = (v >>  8) & 0xff;
                *pb++ = (v >> 16) & 0xff;
                *pb++ = (v >> 24) & 0xff;
              }
            }
//            dumpw (mmc_rgb, sizeof (mmc_rgb), mmc.ib, 0);
          }
#endif

#if defined (USE_MULTIBLOCK_READ)
	  status = mx5_esdhc_execute (CMD_STOP, 0, 0);
#endif
	}
      }


//      status = wait_for_completion (MMC_STATUS_TRANDONE);
//      printf ("\nstatus %x\n", status);
//      if (status & MMC_STATUS_TIMED_OUT)
//	ERROR_RETURN (ERROR_TIMEOUT, "timeout on trandone");
    }


    memcpy (pv, mmc_rgb + (index & (mmc.cb_cache - 1)), available);
    d->index += available;
    cb       -= available;
    cbRead   += available;
    pv       += available;
  }

  return cbRead;
}


static __driver_5 struct driver_d mx5_esdhc_driver = {
  .name		= "mmc-esdhc-mx5",
  .description	= "MMC/SD card driver",
  .flags	= DRIVER_READPROGRESS(2),
  .open		= mx5_esdhc_open,
  .close	= close_helper,
  .read		= mx5_esdhc_read,
//  .write	= mx5_esdhc_write,
  .seek		= seek_helper,
};

static __service_6 struct service_d mx5_esdhc_service = {
  .name		= "mmc-esdhc-mx5",
  .description  = "Freescale iMX51x SD/MMC service",
  .init		= mx5_esdhc_init,
#if !defined (CONFIG_SMALL)
  .report	= mx5_esdhc_report,
#endif
};


static int cmd_mmc (int argc, const char** argv)
{
  mx5_esdhc_init ();
  mmc_acquire ();

  return 0;
}


static __command struct command_d c_mmc = {
  .command = "mmc",
  .func = cmd_mmc,
  COMMAND_DESCRIPTION ("test MMC controller")
};
