/* mmc.h

   written by Marc Singer
   26 May 2006

   Copyright (C) 2006 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   Generic SD/MMC definitions.
*/

#if !defined (__MMC_H__)
#    define   __MMC_H__

/* ----- Includes */

/* ----- Constants */

  /* MMC codes cribbed from the Linux MMC driver */

/* Standard MMC commands (3.1)           type  argument     response */
   /* class 1 */
#define	MMC_GO_IDLE_STATE         0   /* bc                      R1 / 0   */
#define MMC_SEND_OP_COND          1   /* bcr  [31:0]  OCR        R3 / R3  */
#define MMC_ALL_SEND_CID          2   /* bcr                     R2 / R2  */
#define MMC_SET_RELATIVE_ADDR     3   /* ac   [31:16] RCA        R1 / R6  */
#define MMC_SET_DSR               4   /* bc   [31:16] RCA        0  / 0   */
#define SDIO_SEND_OP_COND         5   /* bc   [31:0]  OCR        R4       */
#define SD_SWITCH_FUNC            6   /* adtc args               -  / R1  */
#define MMC_SWITCH                6   /* adtc args               R1b / -  */
#define MMC_SELECT_CARD           7   /* ac   [31:16] RCA        R1b / R1b */
#define MMC_SEND_EXT_CSD          8   /* adtc                    R1       */
#define SD_SEND_IF_COND           8   /* bcr  [11:8]  VHS        -  / R7  */
#define MMC_SEND_CSD              9   /* ac   [31:16] RCA        R2 / R2  */
#define MMC_SEND_CID             10   /* ac   [31:16] RCA        R2 / R2  */
/* Deprecated */
//#define MMC_READ_DAT_UNTIL_STOP 11  /* adtc [31:0]  dadr       R1  */
#define MMC_STOP_TRANSMISSION    12   /* ac                      R1b / R1b */
#define MMC_SEND_STATUS	         13   /* ac   [31:16] RCA        R1 / R1  */
#define MMC_GO_INACTIVE_STATE    15   /* ac   [31:16] RCA        0  / 0    */

  /* class 2 */
#define MMC_SET_BLOCKLEN         16   /* ac   [31:0]  block len  R1 / R1  */
#define MMC_READ_SINGLE_BLOCK    17   /* adtc [31:0]  data addr  R1 / R1  */
#define MMC_READ_MULTIPLE_BLOCK  18   /* adtc [31:0]  data addr  R1 / R1  */

  /* class 3 */
#define MMC_WRITE_DAT_UNTIL_STOP 20   /* adtc [31:0]  data addr  R1 / R1  */

  /* class 4 */
#define MMC_SET_BLOCK_COUNT      23   /* adtc [31:0]  data addr  R1 / -   */
#define MMC_WRITE_BLOCK          24   /* adtc [31:0]  data addr  R1 / R1  */
#define MMC_WRITE_MULTIPLE_BLOCK 25   /* adtc                    R1 / R1  */
#define MMC_PROGRAM_CID          26   /* adtc                    R1 / -   */
#define MMC_PROGRAM_CSD          27   /* adtc                    R1 / R1  */

  /* class 6/4 */
#define MMC_SET_WRITE_PROT       28   /* ac   [31:0]  data addr  R1b / R1b */
#define MMC_CLR_WRITE_PROT       29   /* ac   [31:0]  data addr  R1b / R1b */
#define MMC_SEND_WRITE_PROT      30   /* adtc [31:0]  wpdata adr R1  / R1  */

  /* class 5/6 */
#define SD_ERASE_WR_BLK_START    32   /* ac   [31:0]  data addr  -   / R1  */
#define SD_ERASE_WR_BLK_END      33   /* ac   [31:0]  data addr  -   / R1  */
#define SD_ERASE                 38   /* ac                      -   / R1  */

#define MMC_ERASE_GROUP_START    35   /* ac   [31:0]  data addr  R1  / -   */
#define MMC_ERASE_GROUP_END      36   /* ac   [31:0]  data addr  R1  / -   */
#define MMC_ERASE                37   /* ac                      R1b / -   */

  /* class 9 */
#define MMC_FAST_IO              39   /* ac   args               R4  / -   */
#define MMC_GO_IRQ_STATE         40   /* bcr                     R5  / -   */

  /* class 7 */
#define MMC_LOCK_UNLOCK          42   /* adtc                    R1b / R1  */

  /* class 8 */
#define MMC_APP_CMD              55   /* ac   [31:16] RCA        R1  / R1  */
#define MMC_GEN_CMD              56   /* adtc [0]     RD/WR      R1b / R1  */

/* SD commands                           type  argument     response */
  /* class 8 */
/* This is basically the same command as for MMC with some quirks. */
#define SD_SEND_RELATIVE_ADDR     3   /* ac                      R6  */

  /* Application commands */
#define SD_APP_SET_BUS_WIDTH      6   /* ac   [1:0]   bus width  R1	  */
#define SD_APP_STATUS            13   /* adtc                    R1	  */
#define SD_APP_OP_COND           41   /* bcr  [31:0]  OCR        R3       */
#define SD_APP_SEND_SCR          51   /* adtc                    R1	  */

#define CMD_BIT_APP		 (1<<23)
#define CMD_BIT_INIT		 (1<<22)
#define CMD_BIT_BUSY		 (1<<21)
#define CMD_BIT_LS		 (1<<20) /* Low speed, used during acquire */
#define CMD_BIT_DATA		 (1<<19)
#define CMD_BIT_WRITE		 (1<<18)
#define CMD_BIT_STREAM		 (1<<17)
#define CMD_BIT_RESPWAIT         (1<<16) /* Busy wait after command */
#define CMD_BIT_RESPCRC          (1<<15)
#define CMD_BIT_RESPOPCODE       (1<<14)
#define CMD_MASK_RESPLEN	 (0x03)
#define CMD_SHIFT_RESPLEN	 (28)
#define CMD_MASK_RESPR           (0x0f)
#define CMD_SHIFT_RESPR          (24)
#define CMD_MASK_CMD		 (0xff)
#define CMD_SHIFT_CMD		 (0)
#define CMD_RESP_NONE            (0)
#define CMD_RESP_136             (1)
#define CMD_RESP_48              (2)

#define CMD(c,r)		(  ((c) &  CMD_MASK_CMD)\
				 | CMD_RESP_##r \
				 )

#define CMD_RESP_(w,r)    ((CMD_RESP_##w << CMD_SHIFT_RESPLEN)  \
                           | (r << CMD_SHIFT_RESPR))
#define CMD_RESP_X(w,r)   ((CMD_RESP_##w << CMD_SHIFT_RESPLEN)  \
                           | CMD_BIT_RESPCRC                    \
                           | CMD_BIT_RESPOPCODE                 \
                           | (r << CMD_SHIFT_RESPR))
#define CMD_RESP_0	  (0)
#define CMD_RESP_R1       CMD_RESP_X(48, 1)
#define CMD_RESP_R1b      (CMD_RESP_R1 | CMD_BIT_RESPWAIT)
#define CMD_RESP_R2       CMD_RESP_(136, 2)
#define CMD_RESP_R3       CMD_RESP_(48, 3)
#define CMD_RESP_R6       CMD_RESP_X(48, 6)
#define CMD_RESP_R7	  CMD_RESP_X(48, 7)

#define CMD_IDLE          CMD(MMC_GO_IDLE_STATE,       0) | CMD_BIT_LS | CMD_BIT_INIT
#define CMD_SD_OP_COND    CMD(SD_APP_OP_COND,         R3) | CMD_BIT_LS | CMD_BIT_APP
#define CMD_SD_IF_COND    CMD(SD_SEND_IF_COND,        R7) | CMD_BIT_LS
#define CMD_MMC_OP_COND   CMD(MMC_SEND_OP_COND,       R3) | CMD_BIT_LS | CMD_BIT_INIT
#define CMD_ALL_SEND_CID  CMD(MMC_ALL_SEND_CID,       R2) | CMD_BIT_LS
#define CMD_MMC_SET_RCA   CMD(MMC_SET_RELATIVE_ADDR,  R6) | CMD_BIT_LS
#define CMD_SD_SEND_RCA   CMD(SD_SEND_RELATIVE_ADDR,  R6) | CMD_BIT_LS
#define CMD_SEND_CSD      CMD(MMC_SEND_CSD,           R2)
#define CMD_DESELECT_CARD CMD(MMC_SELECT_CARD,        0)
#define CMD_SELECT_CARD   CMD(MMC_SELECT_CARD,        R1b)
#define CMD_SET_BLOCKLEN  CMD(MMC_SET_BLOCKLEN,       R1)
#define CMD_READ_SINGLE   CMD(MMC_READ_SINGLE_BLOCK,  R1) | CMD_BIT_DATA
#define CMD_READ_MULTIPLE CMD(MMC_READ_MULTIPLE_BLOCK,R1) | CMD_BIT_DATA
#define CMD_SD_SET_WIDTH  CMD(SD_APP_SET_BUS_WIDTH,   R1) | CMD_BIT_APP
#define CMD_STOP          CMD(MMC_STOP_TRANSMISSION,  R1) | CMD_BIT_BUSY
#define CMD_WRITE_SINGLE  CMD(MMC_WRITE_BLOCK,        R1) | CMD_BIT_DATA | CMD_BIT_WRITE
#define CMD_APP           CMD(MMC_APP_CMD,            R1)
#define CMD_SD_SEND_SCR   CMD(SD_APP_SEND_SCR,	      R1) | CMD_BIT_APP


/*
  MMC status in R1
  Type
	e : error bit
	s : status bit
	r : detected and set for the actual command response
	x : detected and set during command execution. the host must poll
	    the card by sending status command in order to read these bits.
  Clear condition
	a : according to the card state
	b : always related to the previous command. Reception of
	    a valid command will clear it (with a delay of one command)
	c : clear by read
 */

#define R1_OUT_OF_RANGE		(1 << 31)	/* er, c */
#define R1_ADDRESS_ERROR	(1 << 30)	/* erx, c */
#define R1_BLOCK_LEN_ERROR	(1 << 29)	/* er, c */
#define R1_ERASE_SEQ_ERROR      (1 << 28)	/* er, c */
#define R1_ERASE_PARAM		(1 << 27)	/* ex, c */
#define R1_WP_VIOLATION		(1 << 26)	/* erx, c */
#define R1_CARD_IS_LOCKED	(1 << 25)	/* sx, a */
#define R1_LOCK_UNLOCK_FAILED	(1 << 24)	/* erx, c */
#define R1_COM_CRC_ERROR	(1 << 23)	/* er, b */
#define R1_ILLEGAL_COMMAND	(1 << 22)	/* er, b */
#define R1_CARD_ECC_FAILED	(1 << 21)	/* ex, c */
#define R1_CC_ERROR		(1 << 20)	/* erx, c */
#define R1_ERROR		(1 << 19)	/* erx, c */
#define R1_UNDERRUN		(1 << 18)	/* ex, c */
#define R1_OVERRUN		(1 << 17)	/* ex, c */
#define R1_CID_CSD_OVERWRITE	(1 << 16)	/* erx, c, CID/CSD overwrite */
#define R1_WP_ERASE_SKIP	(1 << 15)	/* sx, c */
#define R1_CARD_ECC_DISABLED	(1 << 14)	/* sx, a */
#define R1_ERASE_RESET		(1 << 13)	/* sr, c */
#define R1_STATUS(x)            (x & 0xFFFFE000)
#define R1_CURRENT_STATE(x)	((x & 0x00001E00) >> 9)	/* sx, b (4 bits) */
#define R1_READY_FOR_DATA	(1 << 8)	/* sx, a */
#define R1_APP_CMD		(1 << 5)	/* sr, c */

#define MMC_VDD_145_150	0x00000001	/* VDD voltage 1.45 - 1.50 */
#define MMC_VDD_150_155	0x00000002	/* VDD voltage 1.50 - 1.55 */
#define MMC_VDD_155_160	0x00000004	/* VDD voltage 1.55 - 1.60 */
#define MMC_VDD_160_165	0x00000008	/* VDD voltage 1.60 - 1.65 */
#define MMC_VDD_165_170	0x00000010	/* VDD voltage 1.65 - 1.70 */
#define MMC_VDD_17_18	0x00000020	/* VDD voltage 1.7 - 1.8 */
#define MMC_VDD_18_19	0x00000040	/* VDD voltage 1.8 - 1.9 */
#define MMC_VDD_19_20	0x00000080	/* VDD voltage 1.9 - 2.0 */
#define MMC_VDD_20_21	0x00000100	/* VDD voltage 2.0 ~ 2.1 */
#define MMC_VDD_21_22	0x00000200	/* VDD voltage 2.1 ~ 2.2 */
#define MMC_VDD_22_23	0x00000400	/* VDD voltage 2.2 ~ 2.3 */
#define MMC_VDD_23_24	0x00000800	/* VDD voltage 2.3 ~ 2.4 */
#define MMC_VDD_24_25	0x00001000	/* VDD voltage 2.4 ~ 2.5 */
#define MMC_VDD_25_26	0x00002000	/* VDD voltage 2.5 ~ 2.6 */
#define MMC_VDD_26_27	0x00004000	/* VDD voltage 2.6 ~ 2.7 */
#define MMC_VDD_27_28	0x00008000	/* VDD voltage 2.7 ~ 2.8 */
#define MMC_VDD_28_29	0x00010000	/* VDD voltage 2.8 ~ 2.9 */
#define MMC_VDD_29_30	0x00020000	/* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31	0x00040000	/* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_31_32	0x00080000	/* VDD voltage 3.1 ~ 3.2 */
#define MMC_VDD_32_33	0x00100000	/* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34	0x00200000	/* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_34_35	0x00400000	/* VDD voltage 3.4 ~ 3.5 */
#define MMC_VDD_35_36	0x00800000	/* VDD voltage 3.5 ~ 3.6 */
#define OCR_ALL_READY	0x80000000	/* Card Power up status bit */

/*
 * Card Command Classes (CCC)
 */
#define CCC_BASIC		(1<<0)	/* (0) Basic protocol functions */
					/* (CMD0,1,2,3,4,7,9,10,12,13,15) */
#define CCC_STREAM_READ		(1<<1)	/* (1) Stream read commands */
					/* (CMD11) */
#define CCC_BLOCK_READ		(1<<2)	/* (2) Block read commands */
					/* (CMD16,17,18) */
#define CCC_STREAM_WRITE	(1<<3)	/* (3) Stream write commands */
					/* (CMD20) */
#define CCC_BLOCK_WRITE		(1<<4)	/* (4) Block write commands */
					/* (CMD16,24,25,26,27) */
#define CCC_ERASE		(1<<5)	/* (5) Ability to erase blocks */
					/* (CMD32,33,34,35,36,37,38,39) */
#define CCC_WRITE_PROT		(1<<6)	/* (6) Able to write protect blocks */
					/* (CMD28,29,30) */
#define CCC_LOCK_CARD		(1<<7)	/* (7) Able to lock down card */
					/* (CMD16,CMD42) */
#define CCC_APP_SPEC		(1<<8)	/* (8) Application specific */
					/* (CMD55,56,57,ACMD*) */
#define CCC_IO_MODE		(1<<9)	/* (9) I/O mode */
					/* (CMD5,39,40,52,53) */
#define CCC_SWITCH		(1<<10)	/* (10) High speed switch */
					/* (CMD6,34,35,36,37,50) */
					/* (11) Reserved */
					/* (CMD?) */

/*
 * CSD field definitions
 */

#define CSD_STRUCT_VER_1_0  0           /* Valid for system spec 1.0 - 1.2 */
#define CSD_STRUCT_VER_1_1  1           /* Valid for system spec 1.4 - 2.2 */
#define CSD_STRUCT_VER_1_2  2           /* Valid for system spec 3.1       */

#define CSD_SPEC_VER_0      0           /* Implements system spec 1.0 - 1.2 */
#define CSD_SPEC_VER_1      1           /* Implements system spec 1.4 */
#define CSD_SPEC_VER_2      2           /* Implements system spec 2.0 - 2.2 */
#define CSD_SPEC_VER_3      3           /* Implements system spec 3.1 */


enum {
  MMC_RES_OK        =    0,
  MMC_RES_FAILURE   = 1<<0,
  MMC_RES_CMD_ERR   = 1<<1,
  MMC_RES_TIMEOUT   = 1<<2,
  MMC_RES_FIFO_FULL = 1<<3,
  MMC_RES_CRC_ERR   = 1<<4,
};

bool mmc_card_acquired (void);


#endif  /* __MMC_H__ */
