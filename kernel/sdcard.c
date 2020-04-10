/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 *
 * Based on https://github.com/moizumi99/RPiHaribote/blob/master/haribote/sdcard.c
 *
 * Copyright (C) 2020 Hong MingJian<hongmingjian@gmail.com>
 * All rights reserved.
 *
 * This file is part of the EPOS.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 *
 */
// Taken from here
// https://www.raspberrypi.org/forums/viewtopic.php?f=72&t=94133
// Update: Control2 is removed according the the advice from LdB. Thanks a lot.
//
// Notes:
// VOLTAGE_SWITCH: Haven't yet got this to work.  Command appears to work but dat0-3 go low and stay there.
// Data transfer notes:
// The EMMC module restricts the maximum block size to the size of the internal data FIFO which is 1k bytes
// 0x80  Extension FIFO config - what's that?
// This register allows fine tuning the dma_req generation for paced DMA transfers when reading from the card.
#include <stdint.h>
#include <string.h>
#include "kernel.h"

#define SD_OK                0
#define SD_ERROR             1
#define SD_TIMEOUT           2
#define SD_BUSY              3
#define SD_NO_RESP           5
#define SD_ERROR_RESET       6
#define SD_ERROR_CLOCK       7
#define SD_ERROR_VOLTAGE     8
#define SD_ERROR_APP_CMD     9
#define SD_CARD_CHANGED      10
#define SD_CARD_ABSENT       11
#define SD_CARD_REINSERTED   12

#define LOG_DEBUG(...) do { ; } while(0)
#define LOG_ERROR(...) do { printk(__VA_ARGS__); } while(0)

#define TIMEOUT_WAIT(stop_if_true, usec) 						\
do {															\
 	systimer_reg_t *pst = (systimer_reg_t *)(MMIO_BASE_VA+SYSTIMER_REG); \
	uint32_t unow = pst->clo;								\
	do {														\
		if(stop_if_true)										\
			break;												\
	} while(pst->clo - unow < (uint32_t)usec);			\
} while(0)

#define udelay(usec) TIMEOUT_WAIT(0, usec)

// Private functions.
static void sdParseCID();
static void sdParseCSD();
static int sdSendCommand( int index );
int fls_long (unsigned long x);

// EMMC command flags
#define CMD_TYPE_NORMAL  0x00000000
#define CMD_TYPE_SUSPEND 0x00400000
#define CMD_TYPE_RESUME  0x00800000
#define CMD_TYPE_ABORT   0x00c00000
#define CMD_IS_DATA      0x00200000
#define CMD_IXCHK_EN     0x00100000
#define CMD_CRCCHK_EN    0x00080000
#define CMD_RSPNS_NO     0x00000000
#define CMD_RSPNS_136    0x00010000
#define CMD_RSPNS_48     0x00020000
#define CMD_RSPNS_48B    0x00030000
#define TM_MULTI_BLOCK   0x00000020
#define TM_DAT_DIR_HC    0x00000000
#define TM_DAT_DIR_CH    0x00000010
#define TM_AUTO_CMD23    0x00000008
#define TM_AUTO_CMD12    0x00000004
#define TM_BLKCNT_EN     0x00000002
#define TM_MULTI_DATA    (CMD_IS_DATA|TM_MULTI_BLOCK|TM_BLKCNT_EN)

// INTERRUPT register settings
#define INT_AUTO_ERROR   0x01000000
#define INT_DATA_END_ERR 0x00400000
#define INT_DATA_CRC_ERR 0x00200000
#define INT_DATA_TIMEOUT 0x00100000
#define INT_INDEX_ERROR  0x00080000
#define INT_END_ERROR    0x00040000
#define INT_CRC_ERROR    0x00020000
#define INT_CMD_TIMEOUT  0x00010000
#define INT_ERR          0x00008000
#define INT_ENDBOOT      0x00004000
#define INT_BOOTACK      0x00002000
#define INT_RETUNE       0x00001000
#define INT_CARD         0x00000100
#define INT_READ_RDY     0x00000020
#define INT_WRITE_RDY    0x00000010
#define INT_BLOCK_GAP    0x00000004
#define INT_DATA_DONE    0x00000002
#define INT_CMD_DONE     0x00000001
#define INT_ERROR_MASK   (INT_CRC_ERROR|INT_END_ERROR|INT_INDEX_ERROR| \
                          INT_DATA_TIMEOUT|INT_DATA_CRC_ERR|INT_DATA_END_ERR| \
                          INT_ERR|INT_AUTO_ERROR)
#define INT_ALL_MASK     (INT_CMD_DONE|INT_DATA_DONE|INT_READ_RDY|INT_WRITE_RDY|INT_ERROR_MASK)

// CONTROL register settings
#define C0_SPI_MODE_EN   0x00100000
#define C0_HCTL_HS_EN    0x00000004
#define C0_HCTL_DWITDH   0x00000002

#define C1_SRST_DATA     0x04000000
#define C1_SRST_CMD      0x02000000
#define C1_SRST_HC       0x01000000
#define C1_TOUNIT_DIS    0x000f0000
#define C1_TOUNIT_MAX    0x000e0000
#define C1_CLK_GENSEL    0x00000020
#define C1_CLK_EN        0x00000004
#define C1_CLK_STABLE    0x00000002
#define C1_CLK_INTLEN    0x00000001

#define FREQ_SETUP           400000  // 400 Khz
#define FREQ_NORMAL        25000000  // 25 Mhz

// CONTROL2 values
#define C2_VDD_18        0x00080000
#define C2_UHSMODE       0x00070000
#define C2_UHS_SDR12     0x00000000
#define C2_UHS_SDR25     0x00010000
#define C2_UHS_SDR50     0x00020000
#define C2_UHS_SDR104    0x00030000
#define C2_UHS_DDR50     0x00040000

// SLOTISR_VER values
#define HOST_SPEC_NUM              0x00ff0000
#define HOST_SPEC_NUM_SHIFT        16
#define HOST_SPEC_V3               2
#define HOST_SPEC_V2               1
#define HOST_SPEC_V1               0

// STATUS register settings
#define SR_DAT_LEVEL1        0x1e000000
#define SR_CMD_LEVEL         0x01000000
#define SR_DAT_LEVEL0        0x00f00000
#define SR_DAT3              0x00800000
#define SR_DAT2              0x00400000
#define SR_DAT1              0x00200000
#define SR_DAT0              0x00100000
#define SR_WRITE_PROT        0x00080000  // From SDHC spec v2, BCM says reserved
#define SR_READ_AVAILABLE    0x00000800  // ???? undocumented
#define SR_WRITE_AVAILABLE   0x00000400  // ???? undocumented
#define SR_READ_TRANSFER     0x00000200
#define SR_WRITE_TRANSFER    0x00000100
#define SR_DAT_ACTIVE        0x00000004
#define SR_DAT_INHIBIT       0x00000002
#define SR_CMD_INHIBIT       0x00000001

// Arguments for specific commands.
// TODO: What's the correct voltage window for the RPi SD interface?
// 2.7v-3.6v (given by 0x00ff8000) or something narrower?
// TODO: For now, don't offer to switch voltage.
#define ACMD41_HCS           0x40000000
#define ACMD41_SDXC_POWER    0x10000000
#define ACMD41_S18R          0x01000000
#define ACMD41_VOLTAGE       0x00ff8000
#define ACMD41_ARG_HC        (ACMD41_HCS|ACMD41_SDXC_POWER|ACMD41_VOLTAGE|ACMD41_S18R)
#define ACMD41_ARG_SC        (ACMD41_VOLTAGE|ACMD41_S18R)

// R1 (Status) values
#define ST_OUT_OF_RANGE      0x80000000  // 31   E
#define ST_ADDRESS_ERROR     0x40000000  // 30   E
#define ST_BLOCK_LEN_ERROR   0x20000000  // 29   E
#define ST_ERASE_SEQ_ERROR   0x10000000  // 28   E
#define ST_ERASE_PARAM_ERROR 0x08000000  // 27   E
#define ST_WP_VIOLATION      0x04000000  // 26   E
#define ST_CARD_IS_LOCKED    0x02000000  // 25   E
#define ST_LOCK_UNLOCK_FAIL  0x01000000  // 24   E
#define ST_COM_CRC_ERROR     0x00800000  // 23   E
#define ST_ILLEGAL_COMMAND   0x00400000  // 22   E
#define ST_CARD_ECC_FAILED   0x00200000  // 21   E
#define ST_CC_ERROR          0x00100000  // 20   E
#define ST_ERROR             0x00080000  // 19   E
#define ST_CSD_OVERWRITE     0x00010000  // 16   E
#define ST_WP_ERASE_SKIP     0x00008000  // 15   E
#define ST_CARD_ECC_DISABLED 0x00004000  // 14   E
#define ST_ERASE_RESET       0x00002000  // 13   E
#define ST_CARD_STATE        0x00001e00  // 12:9
#define ST_READY_FOR_DATA    0x00000100  // 8
#define ST_APP_CMD           0x00000020  // 5
#define ST_AKE_SEQ_ERROR     0x00000004  // 3    E

#define R1_CARD_STATE_SHIFT  9
#define R1_ERRORS_MASK       0xfff9c004  // All above bits which indicate errors.

// R3 (ACMD41 APP_SEND_OP_COND)
#define R3_COMPLETE    0x80000000
#define R3_CCS         0x40000000
#define R3_S18A        0x01000000

// R6 (CMD3 SEND_REL_ADDR)
#define R6_RCA_MASK    0xffff0000
#define R6_ERR_MASK    0x0000e000
#define R6_STATE_MASK  0x00001e00

// Card state values as they appear in the status register.
#define CS_IDLE    0 // 0x00000000
#define CS_READY   1 // 0x00000200
#define CS_IDENT   2 // 0x00000400
#define CS_STBY    3 // 0x00000600
#define CS_TRAN    4 // 0x00000800
#define CS_DATA    5 // 0x00000a00
#define CS_RCV     6 // 0x00000c00
#define CS_PRG     7 // 0x00000e00
#define CS_DIS     8 // 0x00001000

// Response types.
// Note that on the PI, the index and CRC are dropped, leaving 32 bits in RESP0.
#define RESP_NO    0     // No response
#define RESP_R1    1     // 48  RESP0    contains card status
#define RESP_R1b  11     // 48  RESP0    contains card status, data line indicates busy
#define RESP_R2I   2     // 136 RESP0..3 contains 128 bit CID shifted down by 8 bits as no CRC
#define RESP_R2S  12     // 136 RESP0..3 contains 128 bit CSD shifted down by 8 bits as no CRC
#define RESP_R3    3     // 48  RESP0    contains OCR register
#define RESP_R6    6     // 48  RESP0    contains RCA and status bits 23,22,19,12:0
#define RESP_R7    7     // 48  RESP0    contains voltage acceptance and check pattern

#define RCA_NO     1
#define RCA_YES    2

typedef struct EMMCCommand
{
  const char* name;
  unsigned int code;
  unsigned char resp;
  unsigned char rca;
  int delay;
} EMMCCommand;

// Command table.
// TODO: TM_DAT_DIR_CH required in any of these?
static EMMCCommand sdCommandTable[] =
  {
    { "GO_IDLE_STATE", 0x00000000|CMD_RSPNS_NO                             , RESP_NO , RCA_NO  ,0},
    { "ALL_SEND_CID" , 0x02000000|CMD_RSPNS_136                            , RESP_R2I, RCA_NO  ,0},
    { "SEND_REL_ADDR", 0x03000000|CMD_RSPNS_48                             , RESP_R6 , RCA_NO  ,0},
    { "SET_DSR"      , 0x04000000|CMD_RSPNS_NO                             , RESP_NO , RCA_NO  ,0},
    { "SWITCH_FUNC"  , 0x06000000|CMD_RSPNS_48                             , RESP_R1 , RCA_NO  ,0},
    { "CARD_SELECT"  , 0x07000000|CMD_RSPNS_48B                            , RESP_R1b, RCA_YES ,0},
    { "SEND_IF_COND" , 0x08000000|CMD_RSPNS_48                             , RESP_R7 , RCA_NO  ,100},
    { "SEND_CSD"     , 0x09000000|CMD_RSPNS_136                            , RESP_R2S, RCA_YES ,0},
    { "SEND_CID"     , 0x0A000000|CMD_RSPNS_136                            , RESP_R2I, RCA_YES ,0},
    { "VOLT_SWITCH"  , 0x0B000000|CMD_RSPNS_48                             , RESP_R1 , RCA_NO  ,0},
    { "STOP_TRANS"   , 0x0C000000|CMD_RSPNS_48B                            , RESP_R1b, RCA_NO  ,0},
    { "SEND_STATUS"  , 0x0D000000|CMD_RSPNS_48                             , RESP_R1 , RCA_YES ,0},
    { "GO_INACTIVE"  , 0x0F000000|CMD_RSPNS_NO                             , RESP_NO , RCA_YES ,0},
    { "SET_BLOCKLEN" , 0x10000000|CMD_RSPNS_48                             , RESP_R1 , RCA_NO  ,0},
    { "READ_SINGLE"  , 0x11000000|CMD_RSPNS_48 |CMD_IS_DATA  |TM_DAT_DIR_CH, RESP_R1 , RCA_NO  ,0},
    { "READ_MULTI"   , 0x12000000|CMD_RSPNS_48 |TM_MULTI_DATA|TM_DAT_DIR_CH, RESP_R1 , RCA_NO  ,0},
    { "SEND_TUNING"  , 0x13000000|CMD_RSPNS_48                             , RESP_R1 , RCA_NO  ,0},
    { "SPEED_CLASS"  , 0x14000000|CMD_RSPNS_48B                            , RESP_R1b, RCA_NO  ,0},
    { "SET_BLOCKCNT" , 0x17000000|CMD_RSPNS_48                             , RESP_R1 , RCA_NO  ,0},
    { "WRITE_SINGLE" , 0x18000000|CMD_RSPNS_48 |CMD_IS_DATA  |TM_DAT_DIR_HC, RESP_R1 , RCA_NO  ,0},
    { "WRITE_MULTI"  , 0x19000000|CMD_RSPNS_48 |TM_MULTI_DATA|TM_DAT_DIR_HC, RESP_R1 , RCA_NO  ,0},
    { "PROGRAM_CSD"  , 0x1B000000|CMD_RSPNS_48                             , RESP_R1 , RCA_NO  ,0},
    { "SET_WRITE_PR" , 0x1C000000|CMD_RSPNS_48B                            , RESP_R1b, RCA_NO  ,0},
    { "CLR_WRITE_PR" , 0x1D000000|CMD_RSPNS_48B                            , RESP_R1b, RCA_NO  ,0},
    { "SND_WRITE_PR" , 0x1E000000|CMD_RSPNS_48                             , RESP_R1 , RCA_NO  ,0},
    { "ERASE_WR_ST"  , 0x20000000|CMD_RSPNS_48                             , RESP_R1 , RCA_NO  ,0},
    { "ERASE_WR_END" , 0x21000000|CMD_RSPNS_48                             , RESP_R1 , RCA_NO  ,0},
    { "ERASE"        , 0x26000000|CMD_RSPNS_48B                            , RESP_R1b, RCA_NO  ,0},
    { "LOCK_UNLOCK"  , 0x2A000000|CMD_RSPNS_48                             , RESP_R1 , RCA_NO  ,0},
    { "APP_CMD"      , 0x37000000|CMD_RSPNS_NO                             , RESP_NO , RCA_NO  ,100},
    { "APP_CMD"      , 0x37000000|CMD_RSPNS_48                             , RESP_R1 , RCA_YES ,0},
    { "GEN_CMD"      , 0x38000000|CMD_RSPNS_48                             , RESP_R1 , RCA_NO  ,0},

    // APP commands must be prefixed by an APP_CMD.
    { "SET_BUS_WIDTH", 0x06000000|CMD_RSPNS_48                             , RESP_R1 , RCA_NO  ,0},
    { "SD_STATUS"    , 0x0D000000|CMD_RSPNS_48                             , RESP_R1 , RCA_YES ,0}, // RCA???
    { "SEND_NUM_WRBL", 0x16000000|CMD_RSPNS_48                             , RESP_R1 , RCA_NO  ,0},
    { "SEND_NUM_ERS" , 0x17000000|CMD_RSPNS_48                             , RESP_R1 , RCA_NO  ,0},
    { "SD_SENDOPCOND", 0x29000000|CMD_RSPNS_48                             , RESP_R3 , RCA_NO  ,1000},
    { "SET_CLR_DET"  , 0x2A000000|CMD_RSPNS_48                             , RESP_R1 , RCA_NO  ,0},
    { "SEND_SCR"     , 0x33000000|CMD_RSPNS_48|CMD_IS_DATA|TM_DAT_DIR_CH   , RESP_R1 , RCA_NO  ,0},
  };

// Command indexes in the command table
#define IX_GO_IDLE_STATE    0
#define IX_ALL_SEND_CID     1
#define IX_SEND_REL_ADDR    2
#define IX_SET_DSR          3
#define IX_SWITCH_FUNC      4
#define IX_CARD_SELECT      5
#define IX_SEND_IF_COND     6
#define IX_SEND_CSD         7
#define IX_SEND_CID         8
#define IX_VOLTAGE_SWITCH   9
#define IX_STOP_TRANS       10
#define IX_SEND_STATUS      11
#define IX_GO_INACTIVE      12
#define IX_SET_BLOCKLEN     13
#define IX_READ_SINGLE      14
#define IX_READ_MULTI       15
#define IX_SEND_TUNING      16
#define IX_SPEED_CLASS      17
#define IX_SET_BLOCKCNT     18
#define IX_WRITE_SINGLE     19
#define IX_WRITE_MULTI      20
#define IX_PROGRAM_CSD      21
#define IX_SET_WRITE_PR     22
#define IX_CLR_WRITE_PR     23
#define IX_SND_WRITE_PR     24
#define IX_ERASE_WR_ST      25
#define IX_ERASE_WR_END     26
#define IX_ERASE            27
#define IX_LOCK_UNLOCK      28
#define IX_APP_CMD          29
#define IX_APP_CMD_RCA      30  // APP_CMD used once we have the RCA.
#define IX_GEN_CMD          31

// Commands hereafter require APP_CMD.
#define IX_APP_CMD_START    32
#define IX_SET_BUS_WIDTH    32
#define IX_SD_STATUS        33
#define IX_SEND_NUM_WRBL    34
#define IX_SEND_NUM_ERS     35
#define IX_APP_SEND_OP_COND 36
#define IX_SET_CLR_DET      37
#define IX_SEND_SCR         38

static const char* STATUS_NAME[] =
  { "idle", "ready", "identify", "standby", "transmit", "data", "receive", "prog", "dis" };

// CSD flags
// Note: all flags are shifted down by 8 bits as the CRC is not included.
// Most flags are common:
// in V1 the size is 12 bits with a 3 bit multiplier.
// in V1 currents for read and write are specified.
// in V2 the size is 22 bits, no multiplier, no currents.
#define CSD0_VERSION               0x00c00000
#define CSD0_V1                    0x00000000
#define CSD0_V2                    0x00400000

// CSD Version 1 and 2 flags
#define CSD1VN_TRAN_SPEED          0xff000000

#define CSD1VN_CCC                 0x00fff000
#define CSD1VN_READ_BL_LEN         0x00000f00
#define CSD1VN_READ_BL_LEN_SHIFT   8
#define CSD1VN_READ_BL_PARTIAL     0x00000080
#define CSD1VN_WRITE_BLK_MISALIGN  0x00000040
#define CSD1VN_READ_BLK_MISALIGN   0x00000020
#define CSD1VN_DSR_IMP             0x00000010

#define CSD2VN_ERASE_BLK_EN        0x00000040
#define CSD2VN_ERASE_SECTOR_SIZEH  0x0000003f
#define CSD3VN_ERASE_SECTOR_SIZEL  0x80000000

#define CSD3VN_WP_GRP_SIZE         0x7f000000

#define CSD3VN_WP_GRP_ENABLE       0x00800000
#define CSD3VN_R2W_FACTOR          0x001c0000
#define CSD3VN_WRITE_BL_LEN        0x0003c000
#define CSD3VN_WRITE_BL_LEN_SHIFT  14
#define CSD3VN_WRITE_BL_PARTIAL    0x00002000
#define CSD3VN_FILE_FORMAT_GROUP   0x00000080
#define CSD3VN_COPY                0x00000040
#define CSD3VN_PERM_WRITE_PROT     0x00000020
#define CSD3VN_TEMP_WRITE_PROT     0x00000010
#define CSD3VN_FILE_FORMAT         0x0000000c
#define CSD3VN_FILE_FORMAT_HDD     0x00000000
#define CSD3VN_FILE_FORMAT_DOSFAT  0x00000004
#define CSD3VN_FILE_FORMAT_UFF     0x00000008
#define CSD3VN_FILE_FORMAT_UNKNOWN 0x0000000c

// CSD Version 1 flags.
#define CSD1V1_C_SIZEH             0x00000003
#define CSD1V1_C_SIZEH_SHIFT       10

#define CSD2V1_C_SIZEL             0xffc00000
#define CSD2V1_C_SIZEL_SHIFT       22
#define CSD2V1_VDD_R_CURR_MIN      0x00380000
#define CSD2V1_VDD_R_CURR_MAX      0x00070000
#define CSD2V1_VDD_W_CURR_MIN      0x0000e000
#define CSD2V1_VDD_W_CURR_MAX      0x00001c00
#define CSD2V1_C_SIZE_MULT         0x00000380
#define CSD2V1_C_SIZE_MULT_SHIFT   7

// CSD Version 2 flags.
#define CSD2V2_C_SIZE              0x3fffff00
#define CSD2V2_C_SIZE_SHIFT        8

// SCR flags
// NOTE: SCR is big-endian, so flags appear byte-wise reversed from the spec.
#define SCR_STRUCTURE              0x000000f0
#define SCR_STRUCTURE_V1           0x00000000

#define SCR_SD_SPEC                0x0000000f
#define SCR_SD_SPEC_1_101          0x00000000
#define SCR_SD_SPEC_11             0x00000001
#define SCR_SD_SPEC_2_3            0x00000002

#define SCR_DATA_AFTER_ERASE       0x00008000

#define SCR_SD_SECURITY            0x00007000
#define SCR_SD_SEC_NONE            0x00000000
#define SCR_SD_SEC_NOT_USED        0x00001000
#define SCR_SD_SEC_101             0x00002000  // SDSC
#define SCR_SD_SEC_2               0x00003000  // SDHC
#define SCR_SD_SEC_3               0x00004000  // SDXC

#define SCR_SD_BUS_WIDTHS          0x00000f00
#define SCR_SD_BUS_WIDTH_1         0x00000100
#define SCR_SD_BUS_WIDTH_4         0x00000400

#define SCR_SD_SPEC3               0x00800000
#define SCR_SD_SPEC_2              0x00000000
#define SCR_SD_SPEC_3              0x00100000

#define SCR_EX_SECURITY            0x00780000

#define SCR_CMD_SUPPORT            0x03000000
#define SCR_CMD_SUPP_SET_BLKCNT    0x02000000
#define SCR_CMD_SUPP_SPEED_CLASS   0x01000000

// Capabilities registers.  Not supported by the Pi.
/*
  #define EMMC_HC_V18_SUPPORTED      0x04000000
  #define EMMC_HC_V30_SUPPORTED      0x02000000
  #define EMMC_HC_V33_SUPPORTED      0x01000000
  #define EMMC_HC_SUSPEND            0x00800000
  #define EMMC_HC_SDMA               0x00400000
  #define EMMC_HC_HIGH_SPEED         0x00200000
  #define EMMC_HC_ADMA2              0x00080000
  #define EMMC_HC_MAX_BLOCK          0x00030000
  #define EMMC_HC_MAX_BLOCK_SHIFT    16
  #define EMMC_HC_BASE_CLOCK_FREQ    0x00003f00  // base clock frequency in units of 1MHz, range 10-63Mhz.
  #define EMMC_HC_BASE_CLOCK_FREQ_V3 0x0000ff00  // base clock frequency in units of 1MHz, range 10-255Mhz.
  #define EMMC_HC_BASE_CLOCK_SHIFT   8
  #define EMMC_HC_TOCLOCK_MHZ        0x00000080
  #define EMMC_HC_TOCLOCK_FREQ       0x0000003f  // timeout clock frequency in MHz or KHz, range 1-63
  #define EMMC_HC_TOCLOCK_SHIFT      0
*/

// SD card types
#define SD_TYPE_MMC  1
#define SD_TYPE_1    2
#define SD_TYPE_2_SC 3
#define SD_TYPE_2_HC 4

static const char* SD_TYPE_NAME[] =
  {
  "Unknown", "MMC", "Type 1", "Type 2 SC", "Type 2 HC"
  };

// SD card functions supported values.
#define SD_SUPP_SET_BLOCK_COUNT 0x80000000
#define SD_SUPP_SPEED_CLASS     0x40000000
#define SD_SUPP_BUS_WIDTH_4     0x20000000
#define SD_SUPP_BUS_WIDTH_1     0x10000000


// SD card descriptor
typedef struct SDDescriptor
{
  // Static information about the SD Card.
  unsigned long long capacity;
  unsigned int cid[4];
  unsigned int csd[4];
  unsigned int scr[2];
  unsigned int ocr;
  unsigned int support;
  unsigned int fileFormat;
  unsigned char type;
  unsigned char uhsi;
  unsigned char init;
  unsigned char absent;

  // Dynamic information.
  unsigned int rca;
  unsigned int cardState;
  unsigned int status;

  EMMCCommand* lastCmd;
  unsigned int lastArg;
} SDDescriptor;

// The SD card descriptor.
static SDDescriptor sdCard;

static int sdHostVer = 0;
static int sdDebug = 0;
static int sdBaseClock;


static inline void wait(int32_t count)
{
  asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
               : "=r"(count): [count]"0"(count) : "cc");
}

// necessary function
//#define MBX_PROP_CLOCK_EMMC 1
/* Get the base clock speed.
 */
int sdGetBaseClock()
{
	/*XXX*/
  sdBaseClock = 250000000;//mailbox_getClockRate(MBX_PROP_CLOCK_EMMC);
  if( sdBaseClock == -1 )
    {
      LOG_ERROR("EMMC: Error, failed to get base clock from mailbox\r\n");
      return SD_ERROR;
    }

  return SD_OK;
}

//**************************************************************************
// SD Card PUBLIC functions.
//**************************************************************************

int sdInit()
{
  // Ensure SD information is zeroed.
  memset(&sdCard,0,sizeof(SDDescriptor));

  return 0;
}

/*
 */
static int sdDebugResponse( int resp )
{
  emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
  LOG_DEBUG("EMMC: Command %s resp %08x: %08x %08x %08x %08x\r\n",sdCard.lastCmd->name,resp,emmc->resp3,emmc->resp2,emmc->resp1,emmc->resp0);
  LOG_DEBUG("EMMC: Status: %08x, control1: %08x, interrupt: %08x\r\n",emmc->status,emmc->control1,emmc->interrupt);
  return resp;
}

/* Wait for interrupt.
 */
static int sdWaitForInterrupt( unsigned int mask )
{
  // Wait up to 1 second for the interrupt.
  int count = 1000000;
  int waitMask = mask | INT_ERROR_MASK;
  int ival;

  emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);

  // Wait for the specified interrupt or any error.
  while( !(emmc->interrupt & waitMask) && count-- )
    udelay(1);
  ival = emmc->interrupt;

  // Check for success.
  if( count <= 0 ||
      (ival & INT_CMD_TIMEOUT) ||
      (ival & INT_DATA_TIMEOUT) )
    {
      LOG_DEBUG("EMMC: Wait for interrupt %08x timeout: %08x %08x %08x\r\n",mask,emmc->status,ival,emmc->resp0);
      LOG_DEBUG("EMMC_STATUS:%08x\r\nEMMC_INTERRUPT: %08x\r\nEMMC_RESP0 : %08x\r\nn", emmc->status, emmc->interrupt, emmc->resp0);

      // Clear the interrupt register completely.
      emmc->interrupt = ival;

      return SD_TIMEOUT;
    }
  else if( ival & INT_ERROR_MASK )
    {
      LOG_ERROR("EMMC: Error waiting for interrupt: %08x %08x %08x\r\n",emmc->status,ival,emmc->resp0);

      // Clear the interrupt register completely.
      emmc->interrupt = ival;

      return SD_ERROR;
    }

  // Clear the interrupt we were waiting for, leaving any other (non-error) interrupts.
  emmc->interrupt = mask;

  return SD_OK;
}

/* Wait for any command that may be in progress.
 */
static int sdWaitForCommand()
{
   emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
  // Check for status indicating a command in progress.
  int count = 1000000;
  while( (emmc->status & SR_CMD_INHIBIT) && !(emmc->interrupt & INT_ERROR_MASK) && count-- )
    udelay(1);
  if( count <= 0 || (emmc->interrupt & INT_ERROR_MASK) )
    {
      LOG_ERROR("EMMC: Wait for command aborted: %08x %08x %08x\r\n",emmc->status,emmc->interrupt,emmc->resp0);
      return SD_BUSY;
    }

  return SD_OK;
}

/* Wait for any data that may be in progress.
 */
static int sdWaitForData()
{
	  emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
  // Check for status indicating data transfer in progress.
  // Spec indicates a maximum wait of 500ms.
  // For now this is done by waiting for the DAT_INHIBIT flag to go from the status register,
  // or until an error is flagged in the interrupt register.
  LOG_DEBUG("EMMC: Wait for data started: %08x %08x %08x; dat: %d\r\n",emmc->status,emmc->interrupt,emmc->resp0);
  int count = 0;
  while( (emmc->status & SR_DAT_INHIBIT) && !(emmc->interrupt & INT_ERROR_MASK) && ++count < 500000 )
    udelay(1);
  if( count >= 500000 || (emmc->interrupt & INT_ERROR_MASK) )
    {
      LOG_ERROR("EMMC: Wait for data aborted: %08x %08x %08x\r\n",emmc->status,emmc->interrupt,emmc->resp0);
      return SD_BUSY;
    }
  LOG_DEBUG("EMMC: Wait for data OK: count = %d: %08x %08x %08x\r\n",count,emmc->status,emmc->interrupt,emmc->resp0);

  return SD_OK;
}

/* Send command and handle response.
 */
static int sdSendCommandP( EMMCCommand* cmd, int arg )
{
	  emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
  // Check for command in progress
  if( sdWaitForCommand() != 0 )
    return SD_BUSY;

  if( sdDebug ) LOG_DEBUG("EMMC: Sending command %s code %08x arg %08x\r\n",cmd->name,cmd->code,arg);
  sdCard.lastCmd = cmd;
  sdCard.lastArg = arg;

  LOG_DEBUG("EMMC: Sending command %08x:%s arg %d\r\n",cmd->code,cmd->name,arg);

  int result;

  // Clear interrupt flags.  This is done by setting the ones that are currently set.
  LOG_DEBUG("EMMC_INTERRUPT before clearing: %08x\r\n", emmc->interrupt);
  emmc->interrupt = emmc->interrupt;

  // Set the argument and the command code.
  // Some commands require a delay before reading the response.
  LOG_DEBUG("EMMC_STATUS:%08x\r\nEMMC_INTERRUPT: %08x\r\nEMMC_RESP0 : %08x\r\n", emmc->status, emmc->interrupt, emmc->resp0);
  LOG_DEBUG("ARG: %08x, CODE: %08x\r\n", arg, cmd->code);
  emmc->arg1 = arg;
  emmc->cmdtm = cmd->code;
  if( cmd->delay ) udelay(cmd->delay);

  // Wait until command complete interrupt.
  if( (result = sdWaitForInterrupt(INT_CMD_DONE)) ) return result;

  // Get response from RESP0.
  int resp0 = emmc->resp0;
  LOG_DEBUG("EMMC: Sent command %08x:%s arg %d resp %08x\r\n",cmd->code,cmd->name,arg,resp0);

  // Handle response types.
  switch( cmd->resp )
    {
      // No response.
    case RESP_NO:
      return SD_OK;

      // RESP0 contains card status, no other data from the RESP* registers.
      // Return value non-zero if any error flag in the status value.
    case RESP_R1:
    case RESP_R1b:
      sdCard.status = resp0;
      // Store the card state.  Note that this is the state the card was in before the
      // command was accepted, not the new state.
      sdCard.cardState = (resp0 & ST_CARD_STATE) >> R1_CARD_STATE_SHIFT;
      return resp0 & R1_ERRORS_MASK;

      // RESP0..3 contains 128 bit CID or CSD shifted down by 8 bits as no CRC
      // Note: highest bits are in RESP3.
    case RESP_R2I:
    case RESP_R2S:
      sdCard.status = 0;
      unsigned int* data = cmd->resp == RESP_R2I ? sdCard.cid : sdCard.csd;
      data[0] = emmc->resp3;
      data[1] = emmc->resp2;
      data[2] = emmc->resp1;
      data[3] = resp0;
      return SD_OK;

      // RESP0 contains OCR register
      // TODO: What is the correct time to wait for this?
    case RESP_R3:
      sdCard.status = 0;
      sdCard.ocr = resp0;
      return SD_OK;

      // RESP0 contains RCA and status bits 23,22,19,12:0
    case RESP_R6:
      sdCard.rca = resp0 & R6_RCA_MASK;
      sdCard.status = ((resp0 & 0x00001fff)     ) |   // 12:0 map directly to status 12:0
        ((resp0 & 0x00002000) << 6) |   // 13 maps to status 19 ERROR
        ((resp0 & 0x00004000) << 8) |   // 14 maps to status 22 ILLEGAL_COMMAND
        ((resp0 & 0x00008000) << 8);    // 15 maps to status 23 COM_CRC_ERROR
      // Store the card state.  Note that this is the state the card was in before the
      // command was accepted, not the new state.
      sdCard.cardState = (resp0 & ST_CARD_STATE) >> R1_CARD_STATE_SHIFT;
      return sdCard.status & R1_ERRORS_MASK;

      // RESP0 contains voltage acceptance and check pattern, which should match
      // the argument.
    case RESP_R7:
      sdCard.status = 0;
      return resp0 == arg ? SD_OK : SD_ERROR;
    }

  return SD_ERROR;
}

/* Send APP_CMD.
 */
static int sdSendAppCommand()
{
  int resp;
  // If no RCA, send the APP_CMD and don't look for a response.
  if( !sdCard.rca )
    sdSendCommandP(&sdCommandTable[IX_APP_CMD],0x00000000);

  // If there is an RCA, include that in APP_CMD and check card accepted it.
  else
    {
      if( (resp = sdSendCommandP(&sdCommandTable[IX_APP_CMD_RCA],sdCard.rca)) ) return sdDebugResponse(resp);
      // Debug - check that status indicates APP_CMD accepted.
      if( !(sdCard.status & ST_APP_CMD) )
        return SD_ERROR;
    }

  return SD_OK;
}

/* Send a command with no argument.
 * RCA automatically added if required.
 * APP_CMD sent automatically if required.
 */
static int sdSendCommand( int index )
{
  // Issue APP_CMD if needed.
  int resp;
  if( index >= IX_APP_CMD_START && (resp = sdSendAppCommand()) )
    return sdDebugResponse(resp);

  // Get the command and set RCA if required.
  EMMCCommand* cmd = &sdCommandTable[index];
  int arg = 0;
  if( cmd->rca == RCA_YES )
    arg = sdCard.rca;

  if( (resp = sdSendCommandP(cmd,arg)) ) return resp;

  // Check that APP_CMD was correctly interpreted.
  if( index >= IX_APP_CMD_START && sdCard.rca && !(sdCard.status & ST_APP_CMD) )
    return SD_ERROR_APP_CMD;

  return resp;
}

/* Send a command with a specific argument.
 * APP_CMD sent automatically if required.
 */
static int sdSendCommandA( int index, int arg )
{
  // Issue APP_CMD if needed.
  int resp;
  if( index >= IX_APP_CMD_START && (resp = sdSendAppCommand()) )
    return sdDebugResponse(resp);

  // Get the command and pass the argument through.
  if( (resp = sdSendCommandP(&sdCommandTable[index],arg)) ) return resp;

  // Check that APP_CMD was correctly interpreted.
  if( index >= IX_APP_CMD_START && sdCard.rca && !(sdCard.status & ST_APP_CMD) )
    return SD_ERROR_APP_CMD;

  return resp;
}

/* Read card's SCR
 */
static int sdReadSCR()
{
	  emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
  // SEND_SCR command is like a READ_SINGLE but for a block of 8 bytes.
  // Ensure that any data operation has completed before reading the block.
  if( sdWaitForData() ) return SD_TIMEOUT;

  // Set BLKSIZECNT to 1 block of 8 bytes, send SEND_SCR command
  emmc->blksizecnt = (1 << 16) | 8;
  int resp;
  if( (resp = sdSendCommand(IX_SEND_SCR)) ) return sdDebugResponse(resp);

  // Wait for READ_RDY interrupt.
  if( (resp = sdWaitForInterrupt(INT_READ_RDY)) )
    {
      LOG_ERROR("EMMC: Timeout waiting for ready to read\r\n");
      return sdDebugResponse(resp);
    }

  // Allow maximum of 100ms for the read operation.
  int numRead = 0, count = 100000;
  while( numRead < 2 )
    {
      if( emmc->status & SR_READ_AVAILABLE )
        sdCard.scr[numRead++] = emmc->data;
      else
        {
          udelay(1);
          if( --count == 0 ) break;
        }
    }

  // If SCR not fully read, the operation timed out.
  if( numRead != 2 )
    {
      LOG_ERROR("EMMC: SEND_SCR ERR: %08x %08x %08x\r\n",emmc->status,emmc->interrupt,emmc->resp0);
      LOG_ERROR("EMMC: Reading SCR, only read %d words\r\n",numRead);
      return SD_TIMEOUT;
    }

  // Parse out the SCR.  Only interested in values in scr[0], scr[1] is mfr specific.
  if( sdCard.scr[0] & SCR_SD_BUS_WIDTH_4 ) sdCard.support |= SD_SUPP_BUS_WIDTH_4;
  if( sdCard.scr[0] & SCR_SD_BUS_WIDTH_1 ) sdCard.support |= SD_SUPP_BUS_WIDTH_1;
  if( sdCard.scr[0] & SCR_CMD_SUPP_SET_BLKCNT ) sdCard.support |= SD_SUPP_SET_BLOCK_COUNT;
  if( sdCard.scr[0] & SCR_CMD_SUPP_SPEED_CLASS ) sdCard.support |= SD_SUPP_SPEED_CLASS;

  return SD_OK;
}

int fls_long (unsigned long x) {
  int r = 32;
  if (!x)  return 0;
  if (!(x & 0xffff0000u)) {
    x <<= 16;
    r -= 16;
  }
  if (!(x & 0xff000000u)) {
    x <<= 8;
    r -= 8;
  }
  if (!(x & 0xf0000000u)) {
    x <<= 4;
    r -= 4;
  }
  if (!(x & 0xc0000000u)) {
    x <<= 2;
    r -= 2;
  }
  if (!(x & 0x80000000u)) {
    x <<= 1;
    r -= 1;
  }
  return r;
}

unsigned long roundup_pow_of_two (unsigned long x) {
  return 1UL << fls_long(x - 1);
}

/* Get the clock divider for the given requested frequency.
 * This is calculated relative to the SD base clock.
 */
static uint32_t sdGetClockDivider ( uint32_t freq ) {
  uint32_t divisor;
  uint32_t closest = 41666666 / freq;               // Pi SD frequency is always 41.66667Mhz on baremetal
  uint32_t shiftcount = fls_long(closest - 1);      // Get the raw shiftcount
  if (shiftcount > 0) shiftcount--;               // Note the offset of shift by 1 (look at the spec)
  if (shiftcount > 7) shiftcount = 7;               // It's only 8 bits maximum on HOST_SPEC_V2
  if (sdHostVer > HOST_SPEC_V2) divisor = closest;   // Version 3 take closest
  else divisor = (1 << shiftcount);            // Version 2 take power 2

  if (divisor <= 2) {                           // Too dangerous to go for divisor 1 unless you test
    divisor = 2;                           // You can't take divisor below 2 on slow cards
    shiftcount = 0;                           // Match shift to above just for debug notification
  }

  LOG_DEBUG("Divisor selected = %u, pow 2 shift count = %u\r\n", divisor, shiftcount);
  uint32_t hi = 0;
  if (sdHostVer > HOST_SPEC_V2) hi = (divisor & 0x300) >> 2; // Only 10 bits on Hosts specs above 2
  uint32_t lo = (divisor & 0x0ff);               // Low part always valid
  uint32_t cdiv = (lo << 8) + hi;                  // Join and roll to position
  return cdiv;                              // Return cdiv
}

/* Get the clock divider for the given requested frequency.
 * This is calculated relative to the SD base clock.
 */
static int sdGetClockDivider_old( int freq )
{
  // Work out the closest divider which will result in a frequency
  // equal or less than that requested.
  // Maximum possible divider is 1024.
  int closest = 0;
  if( freq > sdBaseClock ) closest = 1;
  else
    {
      closest = sdBaseClock/freq;
      if( sdBaseClock%freq ) closest++;
    }
  if( closest > 1024 ) closest = 1024;
  // Now find the nearest valid divider value, again that will result in a
  // frequency equal to or less than that requested.
  // For V2, the divider is supposed to be a power of 2
  // For V3, the divider is a multiple of 2, with a value of 0 indicating 1.
  // TODO: currently only V2 algorithm appears to work.
  int div = 1;
  if( 0 && sdHostVer > HOST_SPEC_V2 )
    div = closest;
  else
    for( div = 1; div < closest; div *= 2 );
  div >>= 1;
  // TODO: Don't allow divider > 15 - does not seem to work.
  if( div > 15 ) div = 15;
  LOG_DEBUG("EMMC: Clock divider for freq %d is %d.\r\n",freq,div);
  int hi = (div & 0x300) >> 2;
  int lo = (div & 0x0ff);
  int cdiv = (lo << 8) + hi;
  return cdiv;
}

/* Set the SD clock to the given frequency.
 */
static int sdSetClock( int freq )
{
	  emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
  // Wait for any pending inhibit bits
  int count = 100000;
  while( (emmc->status & (SR_CMD_INHIBIT|SR_DAT_INHIBIT)) && --count )
    udelay(1);
  if( count <= 0 )
    {
      LOG_ERROR("EMMC: Set clock: timeout waiting for inhibit flags. Status %08x.\r\n",emmc->status);
      return SD_ERROR_CLOCK;
    }

  // Switch clock off.
  emmc->control1 &= ~C1_CLK_EN;
  udelay(10);

  // Request the new clock setting and enable the clock
  int cdiv = sdGetClockDivider(freq);
  emmc->control1 = (emmc->control1 & 0xffff003f) | cdiv;
  udelay(10);

  // Enable the clock.
  emmc->control1 |= C1_CLK_EN;
  udelay(10);

  // Wait for clock to be stable.
  count = 10000;
  while( !(emmc->control1 & C1_CLK_STABLE) && count-- )
    udelay(10);
  if( count <= 0 )
    {
      LOG_ERROR("EMMC: ERROR: failed to get stable clock.\r\n");
      return SD_ERROR_CLOCK;
    }

  LOG_DEBUG("EMMC: Set clock, status %08x CONTROL1: %08x\r\n",emmc->status,emmc->control1);

  return SD_OK;
}

/* Reset card.
 */
static int sdResetCard( int resetType )
{
	  emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
  int resp, count;

  // Send reset host controller and wait for complete.
  emmc->control0 = 0; // C0_SPI_MODE_EN;
  //  emmc->CONTROL2 = 0;
  emmc->control1 |= resetType;
  //emmc->CONTROL1 &= ~(C1_CLK_EN|C1_CLK_INTLEN);
  udelay(10);
  count = 10000;
  while( (emmc->control1 & resetType) && count-- )
    udelay(10);
  if( count <= 0 )
    {
      LOG_ERROR("EMMC: ERROR: failed to reset.\r\n");
      return SD_ERROR_RESET;
    }

  // Enable internal clock and set data timeout.
  // TODO: Correct value for timeout?
  emmc->control1 |= C1_CLK_INTLEN | C1_TOUNIT_MAX;
  udelay(10);

  // Set clock to setup frequency.
  if( (resp = sdSetClock(FREQ_SETUP)) ) return resp;

  // Enable interrupts for command completion values.
  //emmc->IRPT_EN   = INT_ALL_MASK;
  //emmc->IRPT_MASK = INT_ALL_MASK;
  emmc->irpt_en   = 0xffffffff;
  emmc->irpt_mask = 0xffffffff;
  LOG_DEBUG("EMMC: Interrupt enable/mask registers: %08x %08x\r\n",emmc->irpt_en,emmc->irpt_mask);
  LOG_DEBUG("EMMC: Status: %08x, control: %08x %08x %08x\r\n",emmc->status,emmc->control0,emmc->control1,emmc->control2);

  // Reset card registers.
  sdCard.rca = 0;
  sdCard.ocr = 0;
  sdCard.lastArg = 0;
  sdCard.lastCmd = 0;
  sdCard.status = 0;
  sdCard.type = 0;
  sdCard.uhsi = 0;

  // Send GO_IDLE_STATE
  LOG_DEBUG("---- Send IX_GO_IDLE_STATE command\r\n");
  resp = sdSendCommand(IX_GO_IDLE_STATE);

  return resp;
}

/* Common routine for APP_SEND_OP_COND.
 * This is used for both SC and HC cards based on the parameter.
 */
static int sdAppSendOpCond( int arg )
{
		  emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
  // Send APP_SEND_OP_COND with the given argument (for SC or HC cards).
  // Note: The host shall set ACMD41 timeout more than 1 second to abort repeat of issuing ACMD41
  // TODO: how to set ACMD41 timeout? Is that the wait?
  LOG_DEBUG("EMMC: Sending ACMD41 SEND_OP_COND status %08x\r\n",emmc->status);
  int resp, count;
  if( (resp = sdSendCommandA(IX_APP_SEND_OP_COND,arg)) && resp != SD_TIMEOUT )
    {
      LOG_ERROR("EMMC: ACMD41 returned non-timeout error %d\r\n",resp);
      return resp;
    }
  count = 6;
  while( !(sdCard.ocr & R3_COMPLETE) && count-- )
    {
      LOG_DEBUG("EMMC: Retrying ACMD SEND_OP_COND status %08x\r\n",emmc->status);
      wait(50000);
      if( (resp = sdSendCommandA(IX_APP_SEND_OP_COND,arg)) && resp != SD_TIMEOUT )
        {
          LOG_ERROR("EMMC: ACMD41 returned non-timeout error %d\r\n",resp);
          return resp;
        }
    }

  // Return timeout error if still not busy.
  if( !(sdCard.ocr & R3_COMPLETE) )
    return SD_TIMEOUT;

  // Check that at least one voltage value was returned.
  if( !(sdCard.ocr & ACMD41_VOLTAGE) )
    return SD_ERROR_VOLTAGE;

  return SD_OK;
}

/* Switch voltage to 1.8v where the card supports it.
 */
static int sdSwitchVoltage()
{
  LOG_DEBUG("EMMC: Pi does not support switch voltage, fixed at 3.3volt\r\n");
  return SD_OK;
}

/* Transfer multiple contiguous blocks between the given address on the card and the buffer.
 */
int sdTransferBlocks( long long address, int numBlocks, unsigned char* buffer, int write )
{
	  emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
  LOG_DEBUG("check sdCard.init\r\n"); // TEST
  if( !sdCard.init ) return SD_NO_RESP;

  LOG_DEBUG("sdWaitForData() .init\r\n"); // TEST
  // Ensure that any data operation has completed before doing the transfer.
  if( sdWaitForData() ) return SD_TIMEOUT;

  // Work out the status, interrupt and command values for the transfer.
  int readyInt = write ? INT_WRITE_RDY : INT_READ_RDY;
  //int readyData = write ? SR_WRITE_AVAILABLE : SR_READ_AVAILABLE;
  int transferCmd = write ? ( numBlocks == 1 ? IX_WRITE_SINGLE : IX_WRITE_MULTI) :
    ( numBlocks == 1 ? IX_READ_SINGLE : IX_READ_MULTI);

  // If more than one block to transfer, and the card supports it,
  // send SET_BLOCK_COUNT command to indicate the number of blocks to transfer.
  int resp;
  if( numBlocks > 1 &&
      (sdCard.support & SD_SUPP_SET_BLOCK_COUNT) &&
      (resp = sdSendCommandA(IX_SET_BLOCKCNT,numBlocks)) ) return sdDebugResponse(resp);

  // Address is different depending on the card type.
  // HC pass address as block # which is just address/512.
  // SC pass address straight through.
  int blockAddress = sdCard.type == SD_TYPE_2_HC ? (int)(address>>9) : (int)address;

  // Set BLKSIZECNT to number of blocks * 512 bytes, send the read or write command.
  // Once the data transfer has started and the TM_BLKCNT_EN bit in the CMDTM register is
  // set the EMMC module automatically decreases the BLKCNT value as the data blocks
  // are transferred and stops the transfer once BLKCNT reaches 0.
  // TODO: TM_AUTO_CMD12 - is this needed?  What effect does it have?
  emmc->blksizecnt = (numBlocks << 16) | 512;
  LOG_DEBUG("sdSendCommandA() .init\r\n"); // TEST
  if( (resp = sdSendCommandA(transferCmd,blockAddress)) ) return sdDebugResponse(resp);

  // Transfer all blocks.
  int blocksDone = 0;
  while( blocksDone < numBlocks )
    {
      // Wait for ready interrupt for the next block.
      if( (resp = sdWaitForInterrupt(readyInt)) )
        {
          LOG_DEBUG("EMMC: Timeout waiting for ready to read\r\n"); //TEST
          LOG_ERROR("EMMC: Timeout waiting for ready to read\r\n");
          return sdDebugResponse(resp);
        }

      // Handle non-word-aligned buffers byte-by-byte.
      // Note: the entire block is sent without looking at status registers.
      int done = 0;
      if( (int)buffer & 0x03 )
        {
          while( done < 512 )
            {
              if( write )
                {
                  int data = (buffer[done++]      );
                  data +=    (buffer[done++] << 8 );
                  data +=    (buffer[done++] << 16);
                  data +=    (buffer[done++] << 24);
                  emmc->data = data;
                }
              else
                {
                  int data = emmc->data;
                  buffer[done++] = (data      ) & 0xff;
                  buffer[done++] = (data >> 8 ) & 0xff;
                  buffer[done++] = (data >> 16) & 0xff;
                  buffer[done++] = (data >> 24) & 0xff;
                }
            }
        }

      // Handle word-aligned buffers more efficiently.
      else
        {
          unsigned int* intbuff = (unsigned int*)buffer;
          while( done < 128 )
            {
              if( write )
                emmc->data = intbuff[done++];
              else
                intbuff[done++] = emmc->data;
            }
        }

      blocksDone++;
      buffer += 512;
    }

  // If not all bytes were read, the operation timed out.
  if( blocksDone != numBlocks )
    {
      LOG_ERROR("EMMC: Transfer error only done %d/%d blocks\r\n",blocksDone,numBlocks);
      LOG_DEBUG("EMMC: Transfer: %08x %08x %08x %08x\r\n",emmc->status,emmc->interrupt,emmc->resp0,emmc->blksizecnt);
      if( !write && numBlocks > 1 && (resp = sdSendCommand(IX_STOP_TRANS)) )
        LOG_DEBUG("EMMC: Error response from stop transmission: %d\r\n",resp);

      return SD_TIMEOUT;
    }

  // For a write operation, ensure DATA_DONE interrupt before we stop transmission.
  if( write && (resp = sdWaitForInterrupt(INT_DATA_DONE)) )
    {
      LOG_ERROR("EMMC: Timeout waiting for data done\r\n");
      return sdDebugResponse(resp);
    }

  // For a multi-block operation, if SET_BLOCKCNT is not supported, we need to indicate
  // that there are no more blocks to be transferred.
  if( numBlocks > 1 && !(sdCard.support & SD_SUPP_SET_BLOCK_COUNT) &&
      (resp = sdSendCommand(IX_STOP_TRANS)) ) return sdDebugResponse(resp);

  return SD_OK;
}

/* Clear multiple contiguous blocks.
 * Assumes that the erase operation writes zeros to the file.
 */
int sdClearBlocks( long long address, int numBlocks )
{
	  emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
  if( !sdCard.init ) return SD_NO_RESP;

  // Ensure that any data operation has completed before doing the transfer.
  if( sdWaitForData() ) return SD_TIMEOUT;

  // Address is different depending on the card type.
  // HC pass address as block # which is just address/512.
  // SC pass address straight through.
  int startAddress = sdCard.type == SD_TYPE_2_HC ? (int)(address>>9) : (int)address;
  int endAddress = startAddress + (sdCard.type == SD_TYPE_2_HC ? (numBlocks - 1) : ((numBlocks-1)*512));

  int resp;
  LOG_DEBUG("EMMC: erasing blocks from %d to %d\r\n",startAddress,endAddress); /*  */
  if( (resp = sdSendCommandA(IX_ERASE_WR_ST,startAddress)) ) return sdDebugResponse(resp);
  if( (resp = sdSendCommandA(IX_ERASE_WR_END,endAddress)) ) return sdDebugResponse(resp);
  if( (resp = sdSendCommand(IX_ERASE)) ) return sdDebugResponse(resp);
  LOG_DEBUG("EMMC: sent erase command, status %08x int %08x\r\n",emmc->status,emmc->interrupt);

  // Wait for data inhibit status to drop.
  int count = 1000000;
  while( emmc->status & SR_DAT_INHIBIT )
    {
      if( --count == 0 )
        {
          LOG_ERROR("EMMC: Timeout waiting for erase: %08x %08x\r\n",emmc->status,emmc->interrupt);
          return SD_TIMEOUT;
        }

      udelay(10);
    }

  LOG_DEBUG("EMMC: completed erase command int %08x\r\n",emmc->interrupt);

  return SD_OK;
}

#define GPIO_FUNC_INPUT 0b000
#define GPIO_FUNC_OUTPUT 0b001
#define GPIO_FUNC_ALT0 0b100
#define GPIO_FUNC_ALT1 0b101
#define GPIO_FUNC_ALT2 0b110
#define GPIO_FUNC_ALT3 0b111
#define GPIO_FUNC_ALT4 0b010
#define GPIO_FUNC_ALT5 0b011
#define GPIO_NO_PULL 0b00
#define GPIO_PULL_DOWN 0b01
#define GPIO_PULL_UP 0b10

void gpioSetFunction(int32_t pin, uint32_t val)
{
    gpio_reg_t *gpio = (gpio_reg_t *)(MMIO_BASE_VA+GPIO_REG);

	uint32_t reg;
	int32_t base=0;
	//	printf("Input  val: %03x\r\n", val);
	if (0<=pin && pin<10) {
		reg = gpio->gpfsel0;//GPFSEL0;
		base = 0;

		reg = reg & (~(7 << ((pin-base)*3)));
		reg = reg | ((val & 7) << ((pin-base)*3));

		gpio->gpfsel0 = reg;
	} else if (10<=pin && pin<20) {
		reg = gpio->gpfsel1;//GPFSEL1;
		base = 10;

		reg = reg & (~(7 << ((pin-base)*3)));
		reg = reg | ((val & 7) << ((pin-base)*3));

		gpio->gpfsel1 = reg;
	} else if (20<=pin && pin<30) {
		reg = gpio->gpfsel2;//GPFSEL2;
		base = 20;

		reg = reg & (~(7 << ((pin-base)*3)));
		reg = reg | ((val & 7) << ((pin-base)*3));

		gpio->gpfsel2 = reg;
	} else if (30<=pin && pin<40) {
		reg = gpio->gpfsel3;//GPFSEL3;
		base = 30;

		reg = reg & (~(7 << ((pin-base)*3)));
		reg = reg | ((val & 7) << ((pin-base)*3));

		gpio->gpfsel3 = reg;
	} else if (40<=pin && pin<50) {
		reg = gpio->gpfsel4;//GPFSEL4;
		base = 40;

		reg = reg & (~(7 << ((pin-base)*3)));
		reg = reg | ((val & 7) << ((pin-base)*3));

		gpio->gpfsel4 = reg;
	} else if (50<=pin && pin<=53) {
		reg = gpio->gpfsel5;//GPFSEL5;
		base = 50;

		reg = reg & (~(7 << ((pin-base)*3)));
		reg = reg | ((val & 7) << ((pin-base)*3));

		gpio->gpfsel5 = reg;
	} else {
		LOG_ERROR("Error gpioSetFunction, pin:%d\r\n", pin);
		return;
	}
}

void gpioSetPull(int32_t pin, uint32_t val)
{
	gpio_reg_t *gpio = (gpio_reg_t *)(MMIO_BASE_VA+GPIO_REG);
	uint32_t enbit;

	gpio->gppud = val;
	wait(150);

	if (pin<32) {
		enbit = 1 << pin;
		gpio->gppudclk0 = enbit;
		wait(150);
		gpio->gppud = 0;
		gpio->gppudclk0 = 0;
	} else {
		enbit = 1 << (pin - 32);
		gpio->gppudclk1 = enbit;
		wait(150);
		gpio->gppud = 0;
		gpio->gppudclk1 = 0;
	}
}

// GPIO pins used for EMMC.
#define GPIO_DAT3  53
#define GPIO_DAT2  52
#define GPIO_DAT1  51
#define GPIO_DAT0  50
#define GPIO_CMD   49
#define GPIO_CLK   48
#define GPIO_CD    47

/* Routine to initialize GPIO registers.
 */
static void sdInitGPIO()
{
  gpio_reg_t *gpio = (gpio_reg_t *)(MMIO_BASE_VA+GPIO_REG);
  unsigned int reg;
  //printf("EMMC: Init. Entry state of GPFSEL4,5: %08x %08x\r\n",*(unsigned int*)0x20200010,*(unsigned int*)0x20200014);

  // Card detect GPIO
  gpioSetFunction(GPIO_CD,GPIO_FUNC_INPUT);
  gpioSetPull(GPIO_CD,GPIO_PULL_UP);
  //  gpioSetDetectHighEvent(GPIO_CD,1);
  reg = gpio->gphen1;//mmio_read(GPHEN1);
  reg = reg | 1<<(47-32);
  gpio->gphen1=reg;//mmio_write(GPHEN1, reg);


  gpioSetFunction(GPIO_DAT3,GPIO_FUNC_ALT3);
  gpioSetPull(GPIO_DAT3,GPIO_PULL_UP);
  gpioSetFunction(GPIO_DAT2,GPIO_FUNC_ALT3);
  gpioSetPull(GPIO_DAT2,GPIO_PULL_UP);
  gpioSetFunction(GPIO_DAT1,GPIO_FUNC_ALT3);
  gpioSetPull(GPIO_DAT1,GPIO_PULL_UP);
  gpioSetFunction(GPIO_DAT0,GPIO_FUNC_ALT3);
  gpioSetPull(GPIO_DAT0,GPIO_PULL_UP);
  gpioSetFunction(GPIO_CMD,GPIO_FUNC_ALT3);
  gpioSetPull(GPIO_CMD,GPIO_PULL_UP);
  gpioSetFunction(GPIO_CLK,GPIO_FUNC_ALT3);
  gpioSetPull(GPIO_CLK,GPIO_PULL_UP);


  //  printf("EMMC: Init. Complete state of GPFSEL4,5: %08x %08x\r\n",*(unsigned int*)0x20200010,*(unsigned int*)0x20200014);
}

/* Initialize SD card.
 * Returns zero if initialization was successful, non-zero otherwise.
 */
int sdInitCard()
{
	  emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
  gpio_reg_t *gpio = (gpio_reg_t *)(MMIO_BASE_VA+GPIO_REG);
  // Ensure we've initialized GPIO.
  if( !sdCard.init ) sdInitGPIO();
  //  wait(50);

  // Check GPIO 47 status
  //  int cardAbsent = gpioGetPinLevel(GPIO_CD);
  //  int cardAbsent = mmio_read(GPLEV1) & (1 << (47-32)); // TEST
  int cardAbsent = 0;
  //  int cardEjected = gpioGetEventDetected(GPIO_CD);
  int cardEjected = gpio->gpeds1/*mmio_read(GPEDS1)*/ & (1 << (47-32));
  int oldCID[4];
  LOG_DEBUG("In SD init card, status %08x interrupt %08x card absent %d ejected %d\r\n",emmc->status,emmc->interrupt,cardAbsent,cardEjected);

  // No card present, nothing can be done.
  // Only log the fact that the card is absent the first time we discover it.
  if( cardAbsent )
    {
      sdCard.init = 0;
      int wasAbsent = sdCard.absent;
      sdCard.absent = 1;
      if( !wasAbsent ) LOG_ERROR("EMMC: no SD card detected");
      return SD_CARD_ABSENT;
    }

  // If initialized before, check status of the card.
  // Card present, but removed since last call to init, then need to
  // go back through init, and indicate that in the return value.
  // In this case we would expect INT_CARD_INSERT to be set.
  // Clear the insert and remove interrupts
  sdCard.absent = 0;
  if( cardEjected && sdCard.init )
    {
      sdCard.init = 0;
      memcpy(oldCID,sdCard.cid,sizeof(int)*4);
    }
  else if( !sdCard.init )
    memset(oldCID,0,sizeof(int)*4);

  // If already initialized and card not replaced, nothing to do.
  if( sdCard.init ) return SD_OK;

  // TODO: check version >= 1 and <= 3?
  sdHostVer = (emmc->slotisr_ver & HOST_SPEC_NUM) >> HOST_SPEC_NUM_SHIFT;

  // Get base clock speed.
  //  sdDebug = 0;
  int resp;
  if( (resp = sdGetBaseClock()) ) return resp;

  // Reset the card.
  LOG_DEBUG("Reset the card\r\n"); // TEST
  if( (resp = sdResetCard(C1_SRST_HC)) ) return resp;

  // Send SEND_IF_COND,0x000001AA (CMD8) voltage range 0x1 check pattern 0xAA
  // If voltage range and check pattern don't match, look for older card.
  resp = sdSendCommandA(IX_SEND_IF_COND,0x000001AA);
  LOG_DEBUG("sdSendCommandA response: %d\r\n", resp);
  if( resp == SD_OK )
    {
      // Card responded with voltage and check pattern.
      // Resolve voltage and check for high capacity card.
      if( (resp = sdAppSendOpCond(ACMD41_ARG_HC)) ) return sdDebugResponse(resp);

      // Check for high or standard capacity.
      if( sdCard.ocr & R3_CCS )
        sdCard.type = SD_TYPE_2_HC;
      else
        sdCard.type = SD_TYPE_2_SC;
    }

  else if( resp == SD_BUSY ) return resp;

  // No response to SEND_IF_COND, treat as an old card.
  else
    {
      // If there appears to be a command in progress, reset the card.
      if( (emmc->status & SR_CMD_INHIBIT) &&
          (resp = sdResetCard(C1_SRST_HC)) )
        return resp;

      //      wait(50);
      // Resolve voltage.
      if( (resp = sdAppSendOpCond(ACMD41_ARG_SC)) ) return sdDebugResponse(resp);

      sdCard.type = SD_TYPE_1;
    }

  // If the switch to 1.8A is accepted, then we need to send a CMD11.
  // CMD11: Completion of voltage switch sequence is checked by high level of DAT[3:0].
  // Any bit of DAT[3:0] can be checked depends on ability of the host.
  // Appears for PI its any/all bits.
  if( (sdCard.ocr & R3_S18A) &&
      (resp = sdSwitchVoltage()) ) return resp;

  // Send ALL_SEND_CID (CMD2)
  if( (resp = sdSendCommand(IX_ALL_SEND_CID)) ) return sdDebugResponse(resp);

  // Send SEND_REL_ADDR (CMD3)
  // TODO: In theory, loop back to SEND_IF_COND to find additional cards.
  if( (resp = sdSendCommand(IX_SEND_REL_ADDR)) ) return sdDebugResponse(resp);

  // From now on the card should be in standby state.
  // Actually cards seem to respond in identify state at this point.
  // Check this with a SEND_STATUS (CMD13)
  //if( (resp = sdSendCommand(IX_SEND_STATUS)) ) return sdDebugResponse(resp);
  LOG_DEBUG("Card current state: %08x %s\r\n",sdCard.status,STATUS_NAME[sdCard.cardState]);

  // Send SEND_CSD (CMD9) and parse the result.
  if( (resp = sdSendCommand(IX_SEND_CSD)) ) return sdDebugResponse(resp);
  sdParseCSD();
  if( sdCard.fileFormat != CSD3VN_FILE_FORMAT_DOSFAT &&
      sdCard.fileFormat != CSD3VN_FILE_FORMAT_HDD )
    {
      LOG_ERROR("EMMC: Error, unrecognised file format %02x\r\n",sdCard.fileFormat);
      return SD_ERROR;
    }

  // At this point, set the clock to full speed.
  if( (resp = sdSetClock(FREQ_NORMAL)) ) return sdDebugResponse(resp);

  // Send CARD_SELECT  (CMD7)
  // TODO: Check card_is_locked status in the R1 response from CMD7 [bit 25], if so, use CMD42 to unlock
  // CMD42 structure [4.3.7] same as a single block write; data block includes
  // PWD setting mode, PWD len, PWD data.
  if( (resp = sdSendCommand(IX_CARD_SELECT)) ) return sdDebugResponse(resp);

  // Get the SCR as well.
  // Need to do this before sending ACMD6 so that allowed bus widths are known.
  if( (resp = sdReadSCR()) ) return sdDebugResponse(resp);

  // Send APP_SET_BUS_WIDTH (ACMD6)
  // If supported, set 4 bit bus width and update the CONTROL0 register.
  if( sdCard.support & SD_SUPP_BUS_WIDTH_4 )
    {
      if( (resp = sdSendCommandA(IX_SET_BUS_WIDTH,sdCard.rca|2)) ) return sdDebugResponse(resp);
      emmc->control0 |= C0_HCTL_DWITDH;
    }

  // Send SET_BLOCKLEN (CMD16)
  // TODO: only needs to be sent for SDSC cards.  For SDHC and SDXC cards block length is fixed
  // at 512 anyway.
  if( (resp = sdSendCommandA(IX_SET_BLOCKLEN,512)) ) return sdDebugResponse(resp);

  // Print out the CID having got this far.
  sdParseCID();

  // Initialisation complete.
  sdCard.init = 1;

  // Return value indicates whether the card was reinserted or replaced.
  if( memcmp(oldCID,sdCard.cid,sizeof(int)*4) == 0 )
    return SD_OK;//SD_CARD_REINSERTED;

  return SD_OK;//SD_CARD_CHANGED;
}

/* Parse CID
 */
static void sdParseCID()
{
  // For some reason cards I have looked at seem to have everything
  // shifted 8 bits down.
  int manId = (sdCard.cid[0]&0x00ff0000) >> 16;
  char appId[3];
  appId[0] = (sdCard.cid[0]&0x0000ff00) >> 8;
  appId[1] = (sdCard.cid[0]&0x000000ff);
  appId[2] = 0;
  char name[6];
  name[0] = (sdCard.cid[1]&0xff000000) >> 24;
  name[1] = (sdCard.cid[1]&0x00ff0000) >> 16;
  name[2] = (sdCard.cid[1]&0x0000ff00) >> 8;
  name[3] = (sdCard.cid[1]&0x000000ff);
  name[4] = (sdCard.cid[2]&0xff000000) >> 24;
  name[5] = 0;
  int revH = (sdCard.cid[2]&0x00f00000) >> 20;
  int revL = (sdCard.cid[2]&0x000f0000) >> 16;
  int serial = ((sdCard.cid[2]&0x0000ffff) << 16) +
    ((sdCard.cid[3]&0xffff0000) >> 16);

  // For some reason cards I have looked at seem to have the Y/M in
  // bits 11:0 whereas the spec says they should be in bits 19:8
  int dateY = ((sdCard.cid[3]&0x00000ff0) >> 4) + 2000;
  int dateM = (sdCard.cid[3]&0x0000000f);

  LOG_DEBUG("EMMC: SD Card %s %dMb UHS-I %d mfr %d '%s:%s' r%d.%d %d/%d, #%08x RCA %04x",
            SD_TYPE_NAME[sdCard.type], (int)(sdCard.capacity>>20),sdCard.uhsi,
            manId, appId, name, revH, revL, dateM, dateY, serial, sdCard.rca>>16);
}

/* Parse CSD
 */
static void sdParseCSD()
{
  int csdVersion = sdCard.csd[0] & CSD0_VERSION;

  // For now just work out the size.
  if( csdVersion == CSD0_V1 )
    {
      int csize = ((sdCard.csd[1] & CSD1V1_C_SIZEH) << CSD1V1_C_SIZEH_SHIFT) +
        ((sdCard.csd[2] & CSD2V1_C_SIZEL) >> CSD2V1_C_SIZEL_SHIFT);
      int mult = 1 << (((sdCard.csd[2] & CSD2V1_C_SIZE_MULT) >> CSD2V1_C_SIZE_MULT_SHIFT) + 2);
      long long blockSize = 1 << ((sdCard.csd[1] & CSD1VN_READ_BL_LEN) >> CSD1VN_READ_BL_LEN_SHIFT);
      long long numBlocks = (csize+1LL)*mult;

      sdCard.capacity = numBlocks * blockSize;
    }
  else // if( csdVersion == CSD0_V2 )
    {
      long long csize = (sdCard.csd[2] & CSD2V2_C_SIZE) >> CSD2V2_C_SIZE_SHIFT;

      sdCard.capacity = (csize+1LL)*512LL*1024LL;
    }

  // Get other attributes of the card.
  sdCard.fileFormat = sdCard.csd[3] & CSD3VN_FILE_FORMAT;
}

/*****************************************************************************/
#include "kernel.h"

static int sd_attach(struct dev *dp)
{
	struct sd_dev *sd_dev = (struct sd_dev *)dp;
	int ret = sdInitCard();
	if(ret == SD_OK)
		return 0;
	return -ret;
}

static void sd_detach(struct dev *dp)
{
}

#define min(x,y) (((x)>(y))?(y):(x))
static int sd_read(struct dev *dp, uint32_t addr, uint8_t *buf, size_t buf_size)
{
	struct sd_dev *sd_dev = (struct sd_dev *)dp;
	int ret;
	uint8_t tmpbuf[512];
	uint8_t *oldbuf = buf;
	uint32_t rem;

	if(buf_size == 0)
		return buf-oldbuf;

	rem = addr & (512-1);
	if(rem) {
		ret = sdTransferBlocks(addr-rem, 1, tmpbuf, 0);
		if(ret != SD_OK)
			return buf-oldbuf;

		uint32_t len = min(512-rem, buf_size);
		memcpy(buf, &tmpbuf[rem], len);

		buf_size -= len;
		buf += len;
		addr += len;
	}

	rem = buf_size & (~(512-1));
	if(rem) {
		ret = sdTransferBlocks( addr, buf_size/512, buf, 0 );
		if(ret != SD_OK)
			return buf-oldbuf;

		buf_size -= rem;
		buf += rem;
		addr += rem;
	}

	if(buf_size) {
		ret = sdTransferBlocks( addr, 1, tmpbuf, 0 );
		if(ret != SD_OK)
			return buf-oldbuf;

		memcpy(buf, &tmpbuf[0], buf_size);
		buf += buf_size;
	}

	return buf-oldbuf;
}

static int sd_write(struct dev *dp, uint32_t addr, uint8_t *buf, size_t buf_size)
{
	struct sd_dev *sd_dev = (struct sd_dev *)dp;
	int ret;
	uint8_t tmpbuf[512];
	uint8_t *oldbuf = buf;
	uint32_t rem;

	if(buf_size == 0)
		return buf-oldbuf;

	rem = addr & (512-1);
	if(rem) {
		ret = sdTransferBlocks(addr-rem, 1, tmpbuf, 0);
		if(ret != SD_OK)
			return buf-oldbuf;

		uint32_t len = min(512-rem, buf_size);
		memcpy(&tmpbuf[rem], buf, len);

		ret = sdTransferBlocks( addr-rem, 1, tmpbuf, 1 );
		if(ret != SD_OK)
			return buf-oldbuf;

		buf_size -= len;
		buf += len;
		addr += len;
	}

	rem = buf_size & (~(512-1));
	if(rem) {
		ret = sdTransferBlocks( addr, buf_size/512, buf, 1 );
		if(ret != SD_OK)
			return buf-oldbuf;

		buf_size -= rem;
		buf += rem;
		addr += rem;
	}

	if(buf_size) {
		ret = sdTransferBlocks(addr, 1, tmpbuf, 0);
		if(ret != SD_OK)
			return buf-oldbuf;

		memcpy(&tmpbuf[0], buf, buf_size);

		ret = sdTransferBlocks( addr, 1, tmpbuf, 1 );
		if(ret != SD_OK)
			return buf-oldbuf;

		buf += buf_size;
	}

	return buf-oldbuf;
}

static int sd_poll(struct dev *dp, int events)
{
	return 1;
}

static int sd_ioctl(struct dev *dp, int cmd, void *arg)
{
	return -1;
}

static struct driver sd_driver = {
	.major = "sd",
	.attach = sd_attach,
	.detach = sd_detach,
	.read = sd_read,
	.write = sd_write,
	.poll = sd_poll,
	.ioctl = sd_ioctl
};

struct sd_dev {
	struct dev dev;
} sd_dev = {
	{
	.drv = &sd_driver,
	.minor = 0
	}
};
/*****************************************************************************/
