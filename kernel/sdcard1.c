/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
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
/**
 * @file sd.c
 * https://github.com/vanvught/rpidmx512/tree/master/lib-bcm2835
 */
/* Copyright (C) 2015-2020 by Arjan van Vught mailto:info@orangepi-dmx.nl
 * Based on
 * https://github.com/jncronin/rpi-boot/blob/master/emmc.c
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <string.h>
#include "kernel.h"

#define SD_TRACE(...) do { printk(__VA_ARGS__); } while(0)
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



uint32_t bcm2835_mailbox_read(uint8_t channel)
{
	mailbox_reg_t *mbox = (mailbox_reg_t *)(MMIO_BASE_VA+MAILBOX_REG);

	uint32_t data;
	do {
		while (mbox->status & 0x40000000)
			; // do nothing

		data = mbox->read;

	} while ((uint8_t) (data & 0xf) != channel);

	return (data & ~0xf);
}

void bcm2835_mailbox_write(uint8_t channel, uint32_t data)
{
	mailbox_reg_t *mbox = (mailbox_reg_t *)(MMIO_BASE_VA+MAILBOX_REG);
	while (mbox->status & 0x80000000)
		; // do nothing
	mbox->write = (data & ~0xf) | (uint32_t) (channel & 0xf);
}

struct vc_msg_tag_uint32 {
	uint32_t tag_id;				///< the message id
	uint32_t buffer_size;			///< size of the buffer (which in this case is always 8 bytes)
	uint32_t data_size;				///< amount of data being sent or received
	uint32_t dev_id;				///< the ID of the clock/voltage to get or set
	uint32_t val;					///< the value (e.g. rate (in Hz)) to set
};

struct vc_msg_uint32 {
	uint32_t msg_size;				///< simply, sizeof(struct vc_msg)
	uint32_t request_code;			///< holds various information like the success and number of bytes returned (refer to mailboxes wiki)
	struct vc_msg_tag_uint32 tag;	///< the tag structure above to make
	uint32_t end_tag;				///< an end identifier, should be set to NULL
};


#define BCM2835_MAILBOX_SUCCESS	(uint32_t)0x80000000	///< Request successful
#define BCM2835_MAILBOX_ERROR	(uint32_t)0x80000001	///< Error parsing request buffer (partial response)
#define BCM2835_MAILBOX_PROP_CHANNEL 	 8		///< https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface


#define	BCM2835_VC_CLOCK_ID_EMMC  1
#define BCM2835_VC_TAG_GET_CLOCK_RATE 			0x00030002
inline static int32_t bcm2835_vc_get(uint32_t tag_id, uint32_t dev_id)
 {
	struct vc_msg_uint32 *vc_msg = (struct vc_msg_uint32 *)0xc0003000; /*XXX*/

	vc_msg->msg_size = sizeof(struct vc_msg_uint32);
	vc_msg->request_code = 0;
	vc_msg->tag.tag_id = tag_id;
	vc_msg->tag.buffer_size = 8;
	vc_msg->tag.data_size = 4;
	vc_msg->tag.dev_id = dev_id;
	vc_msg->tag.val = 0;
	vc_msg->end_tag = 0;

	bcm2835_mailbox_write(BCM2835_MAILBOX_PROP_CHANNEL, 0x3000/*XXX*/);
	bcm2835_mailbox_read(BCM2835_MAILBOX_PROP_CHANNEL);

	if (vc_msg->request_code != BCM2835_MAILBOX_SUCCESS) {
		SD_TRACE("bcm2835_vc_get1\r\n");
		return -1;
	}

	if (vc_msg->tag.dev_id != dev_id) {
		SD_TRACE("bcm2835_vc_get2\r\n");
		return -1;
	}

	return (int32_t) vc_msg->tag.val;
}








#define SUCCESS(a)          	(a->last_cmd_success)
#define FAIL(a)            	 	(a->last_cmd_success == 0)
#define TIMEOUT(a)         		(FAIL(a) && (a->last_error == 0))
#define CMD_TIMEOUT(a)     		(FAIL(a) && (a->last_error & (1 << 16)))
#define CMD_CRC(a)        	 	(FAIL(a) && (a->last_error & (1 << 17)))
#define CMD_END_BIT(a)     		(FAIL(a) && (a->last_error & (1 << 18)))
#define CMD_INDEX(a)       		(FAIL(a) && (a->last_error & (1 << 19)))
#define DATA_TIMEOUT(a)  		(FAIL(a) && (a->last_error & (1 << 20)))
#define DATA_CRC(a)				(FAIL(a) && (a->last_error & (1 << 21)))
#define DATA_END_BIT(a)			(FAIL(a) && (a->last_error & (1 << 22)))
#define CURRENT_LIMIT(a)		(FAIL(a) && (a->last_error & (1 << 23)))
#define ACMD12_ERROR(a)			(FAIL(a) && (a->last_error & (1 << 24)))
#define ADMA_ERROR(a)			(FAIL(a) && (a->last_error & (1 << 25)))
#define TUNING_ERROR(a)			(FAIL(a) && (a->last_error & (1 << 26)))

#define SD_OK                0
#define SD_ERROR             -1
#define SD_TIMEOUT           -2
#define SD_BUSY              -3
#define SD_NO_RESP           -5
#define SD_ERROR_RESET       -6
#define SD_ERROR_CLOCK       -7
#define SD_ERROR_VOLTAGE     -8
#define SD_ERROR_APP_CMD     -9
#define SD_CARD_CHANGED      -10
#define SD_CARD_ABSENT       -11
#define SD_CARD_REINSERTED   -12

#define SD_CLOCK_ID         	4000000
#define SD_CLOCK_NORMAL     	25000000
#define SD_CLOCK_HIGH       	50000000
#define SD_CLOCK_100        	100000000
#define SD_CLOCK_208        	208000000

#define SD_VER_UNKNOWN      	0
#define SD_VER_1            	1
#define SD_VER_1_1          	2
#define SD_VER_2            	3
#define SD_VER_3            	4
#define SD_VER_4            	5

#define GO_IDLE_STATE           0
#define ALL_SEND_CID            2
#define SEND_RELATIVE_ADDR      3
#define SET_DSR                 4
#define IO_SET_OP_COND          5
#define SWITCH_FUNC             6
#define SELECT_CARD             7
#define DESELECT_CARD           7
#define SELECT_DESELECT_CARD    7
#define SEND_IF_COND            8
#define SEND_CSD                9
#define SEND_CID                10
#define VOLTAGE_SWITCH          11
#define STOP_TRANSMISSION       12
#define SEND_STATUS             13
#define GO_INACTIVE_STATE       15
#define SET_BLOCKLEN            16
#define READ_SINGLE_BLOCK       17
#define READ_MULTIPLE_BLOCK     18
#define SEND_TUNING_BLOCK       19
#define SPEED_CLASS_CONTROL     20
#define SET_BLOCK_COUNT         23
#define WRITE_BLOCK             24
#define WRITE_MULTIPLE_BLOCK    25
#define PROGRAM_CSD             27
#define SET_WRITE_PROT          28
#define CLR_WRITE_PROT          29
#define SEND_WRITE_PROT         30
#define ERASE_WR_BLK_START      32
#define ERASE_WR_BLK_END        33
#define ERASE                   38
#define LOCK_UNLOCK             42
#define APP_CMD                 55
#define GEN_CMD                 56


#define SD_CMD_INDEX(a)				(uint32_t)((a) << 24)
#define SD_CMD_TYPE_NORMAL			0x0
#define SD_CMD_TYPE_SUSPEND			(1 << 22)
#define SD_CMD_TYPE_RESUME			(2 << 22)
#define SD_CMD_TYPE_ABORT			(3 << 22)
#define SD_CMD_TYPE_MASK			(3 << 22)
#define SD_CMD_ISDATA				(1 << 21)
#define SD_CMD_IXCHK_EN				(1 << 20)
#define SD_CMD_CRCCHK_EN			(1 << 19)
#define SD_CMD_RSPNS_TYPE_NONE		0			// For no response
#define SD_CMD_RSPNS_TYPE_136		(1 << 16)	// For response R2 (with CRC), R3,4 (no CRC)
#define SD_CMD_RSPNS_TYPE_48		(2 << 16)	// For responses R1, R5, R6, R7 (with CRC)
#define SD_CMD_RSPNS_TYPE_48B		(3 << 16)	// For responses R1b, R5b (with CRC)
#define SD_CMD_RSPNS_TYPE_MASK 		(3 << 16)
#define SD_CMD_MULTI_BLOCK			(1 << 5)
#define SD_CMD_DAT_DIR_HC			0
#define SD_CMD_DAT_DIR_CH			(1 << 4)
#define SD_CMD_AUTO_CMD_EN_NONE		0
#define SD_CMD_AUTO_CMD_EN_CMD12	(1 << 2)
#define SD_CMD_AUTO_CMD_EN_CMD23	(2 << 2)
#define SD_CMD_BLKCNT_EN			(1 << 1)
#define SD_CMD_DMA          		1

#define SD_ERR_CMD_TIMEOUT		0
#define SD_ERR_CMD_CRC			1
#define SD_ERR_CMD_END_BIT		2
#define SD_ERR_CMD_INDEX		3
#define SD_ERR_DATA_TIMEOUT		4
#define SD_ERR_DATA_CRC			5
#define SD_ERR_DATA_END_BIT		6
#define SD_ERR_CURRENT_LIMIT	7
#define SD_ERR_AUTO_CMD12		8
#define SD_ERR_ADMA				9
#define SD_ERR_TUNING			10
#define SD_ERR_RSVD				11

#define SD_ERR_MASK_CMD_TIMEOUT		(1 << (16 + SD_ERR_CMD_TIMEOUT))
#define SD_ERR_MASK_CMD_CRC			(1 << (16 + SD_ERR_CMD_CRC))
#define SD_ERR_MASK_CMD_END_BIT		(1 << (16 + SD_ERR_CMD_END_BIT))
#define SD_ERR_MASK_CMD_INDEX		(1 << (16 + SD_ERR_CMD_INDEX))
#define SD_ERR_MASK_DATA_TIMEOUT	(1 << (16 + SD_ERR_CMD_TIMEOUT))
#define SD_ERR_MASK_DATA_CRC		(1 << (16 + SD_ERR_CMD_CRC))
#define SD_ERR_MASK_DATA_END_BIT	(1 << (16 + SD_ERR_CMD_END_BIT))
#define SD_ERR_MASK_CURRENT_LIMIT	(1 << (16 + SD_ERR_CMD_CURRENT_LIMIT))
#define SD_ERR_MASK_AUTO_CMD12		(1 << (16 + SD_ERR_CMD_AUTO_CMD12))
#define SD_ERR_MASK_ADMA			(1 << (16 + SD_ERR_CMD_ADMA))
#define SD_ERR_MASK_TUNING			(1 << (16 + SD_ERR_CMD_TUNING))

#define SD_RESP_NONE        SD_CMD_RSPNS_TYPE_NONE
#define SD_RESP_R1          (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R1b         (SD_CMD_RSPNS_TYPE_48B | SD_CMD_CRCCHK_EN)
#define SD_RESP_R2          (SD_CMD_RSPNS_TYPE_136 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R3          SD_CMD_RSPNS_TYPE_48
#define SD_RESP_R4          SD_CMD_RSPNS_TYPE_136
#define SD_RESP_R5          (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R5b         (SD_CMD_RSPNS_TYPE_48B | SD_CMD_CRCCHK_EN)
#define SD_RESP_R6          (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R7          (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)

#define SD_DATA_READ        (SD_CMD_ISDATA | SD_CMD_DAT_DIR_CH)
#define SD_DATA_WRITE       (SD_CMD_ISDATA | SD_CMD_DAT_DIR_HC)

#define SD_CMD_RESERVED(a)  (uint32_t)0xffffffff

const uint32_t sd_commands[] __attribute__((aligned(4))) = {
    SD_CMD_INDEX(0),
    SD_CMD_RESERVED(1),
    SD_CMD_INDEX(2) | SD_RESP_R2,
    SD_CMD_INDEX(3) | SD_RESP_R6,
    SD_CMD_INDEX(4),
    SD_CMD_INDEX(5) | SD_RESP_R4,
    SD_CMD_INDEX(6) | SD_RESP_R1,
    SD_CMD_INDEX(7) | SD_RESP_R1b,
    SD_CMD_INDEX(8) | SD_RESP_R7,
    SD_CMD_INDEX(9) | SD_RESP_R2,
    SD_CMD_INDEX(10) | SD_RESP_R2,
    SD_CMD_INDEX(11) | SD_RESP_R1,
    SD_CMD_INDEX(12) | SD_RESP_R1b | SD_CMD_TYPE_ABORT,
    SD_CMD_INDEX(13) | SD_RESP_R1,
    SD_CMD_RESERVED(14),
    SD_CMD_INDEX(15),
    SD_CMD_INDEX(16) | SD_RESP_R1,
    SD_CMD_INDEX(17) | SD_RESP_R1 | SD_DATA_READ,
    SD_CMD_INDEX(18) | SD_RESP_R1 | SD_DATA_READ | SD_CMD_MULTI_BLOCK | SD_CMD_BLKCNT_EN,
    SD_CMD_INDEX(19) | SD_RESP_R1 | SD_DATA_READ,
    SD_CMD_INDEX(20) | SD_RESP_R1b,
    SD_CMD_RESERVED(21),
    SD_CMD_RESERVED(22),
    SD_CMD_INDEX(23) | SD_RESP_R1,
    SD_CMD_INDEX(24) | SD_RESP_R1 | SD_DATA_WRITE,
    SD_CMD_INDEX(25) | SD_RESP_R1 | SD_DATA_WRITE | SD_CMD_MULTI_BLOCK | SD_CMD_BLKCNT_EN,
    SD_CMD_RESERVED(26),
    SD_CMD_INDEX(27) | SD_RESP_R1 | SD_DATA_WRITE,
    SD_CMD_INDEX(28) | SD_RESP_R1b,
    SD_CMD_INDEX(29) | SD_RESP_R1b,
    SD_CMD_INDEX(30) | SD_RESP_R1 | SD_DATA_READ,
    SD_CMD_RESERVED(31),
    SD_CMD_INDEX(32) | SD_RESP_R1,
    SD_CMD_INDEX(33) | SD_RESP_R1,
    SD_CMD_RESERVED(34),
    SD_CMD_RESERVED(35),
    SD_CMD_RESERVED(36),
    SD_CMD_RESERVED(37),
    SD_CMD_INDEX(38) | SD_RESP_R1b,
    SD_CMD_RESERVED(39),
    SD_CMD_RESERVED(40),
    SD_CMD_RESERVED(41),
    SD_CMD_RESERVED(42) | SD_RESP_R1,
    SD_CMD_RESERVED(43),
    SD_CMD_RESERVED(44),
    SD_CMD_RESERVED(45),
    SD_CMD_RESERVED(46),
    SD_CMD_RESERVED(47),
    SD_CMD_RESERVED(48),
    SD_CMD_RESERVED(49),
    SD_CMD_RESERVED(50),
    SD_CMD_RESERVED(51),
    SD_CMD_RESERVED(52),
    SD_CMD_RESERVED(53),
    SD_CMD_RESERVED(54),
    SD_CMD_INDEX(55) | SD_RESP_R1,
    SD_CMD_INDEX(56) | SD_RESP_R1 | SD_CMD_ISDATA,
    SD_CMD_RESERVED(57),
    SD_CMD_RESERVED(58),
    SD_CMD_RESERVED(59),
    SD_CMD_RESERVED(60),
    SD_CMD_RESERVED(61),
    SD_CMD_RESERVED(62),
    SD_CMD_RESERVED(63)
};

static const uint32_t sd_acommands[] __attribute__((aligned(4))) = {
    SD_CMD_RESERVED(0),
    SD_CMD_RESERVED(1),
    SD_CMD_RESERVED(2),
    SD_CMD_RESERVED(3),
    SD_CMD_RESERVED(4),
    SD_CMD_RESERVED(5),
    SD_CMD_INDEX(6) | SD_RESP_R1,
    SD_CMD_RESERVED(7),
    SD_CMD_RESERVED(8),
    SD_CMD_RESERVED(9),
    SD_CMD_RESERVED(10),
    SD_CMD_RESERVED(11),
    SD_CMD_RESERVED(12),
    SD_CMD_INDEX(13) | SD_RESP_R1,
    SD_CMD_RESERVED(14),
    SD_CMD_RESERVED(15),
    SD_CMD_RESERVED(16),
    SD_CMD_RESERVED(17),
    SD_CMD_RESERVED(18),
    SD_CMD_RESERVED(19),
    SD_CMD_RESERVED(20),
    SD_CMD_RESERVED(21),
    SD_CMD_INDEX(22) | SD_RESP_R1 | SD_DATA_READ,
    SD_CMD_INDEX(23) | SD_RESP_R1,
    SD_CMD_RESERVED(24),
    SD_CMD_RESERVED(25),
    SD_CMD_RESERVED(26),
    SD_CMD_RESERVED(27),
    SD_CMD_RESERVED(28),
    SD_CMD_RESERVED(29),
    SD_CMD_RESERVED(30),
    SD_CMD_RESERVED(31),
    SD_CMD_RESERVED(32),
    SD_CMD_RESERVED(33),
    SD_CMD_RESERVED(34),
    SD_CMD_RESERVED(35),
    SD_CMD_RESERVED(36),
    SD_CMD_RESERVED(37),
    SD_CMD_RESERVED(38),
    SD_CMD_RESERVED(39),
    SD_CMD_RESERVED(40),
    SD_CMD_INDEX(41) | SD_RESP_R3,
    SD_CMD_INDEX(42) | SD_RESP_R1,
    SD_CMD_RESERVED(43),
    SD_CMD_RESERVED(44),
    SD_CMD_RESERVED(45),
    SD_CMD_RESERVED(46),
    SD_CMD_RESERVED(47),
    SD_CMD_RESERVED(48),
    SD_CMD_RESERVED(49),
    SD_CMD_RESERVED(50),
    SD_CMD_INDEX(51) | SD_RESP_R1 | SD_DATA_READ,
    SD_CMD_RESERVED(52),
    SD_CMD_RESERVED(53),
    SD_CMD_RESERVED(54),
    SD_CMD_RESERVED(55),
    SD_CMD_RESERVED(56),
    SD_CMD_RESERVED(57),
    SD_CMD_RESERVED(58),
    SD_CMD_RESERVED(59),
    SD_CMD_RESERVED(60),
    SD_CMD_RESERVED(61),
    SD_CMD_RESERVED(62),
    SD_CMD_RESERVED(63)
};

struct sd_scr {
	uint32_t scr[2];
	uint32_t sd_bus_widths;
	int sd_version;
};

struct block_device {
	size_t block_size;
};

struct emmc_block_dev {
	struct block_device bd;
	uint32_t card_supports_sdhc;
	uint32_t card_supports_18v;
	uint32_t card_ocr;
	uint32_t card_rca;
	uint32_t last_interrupt;
	uint32_t last_error;

	struct sd_scr *scr;

	int failed_voltage_switch;

	uint32_t last_cmd_reg;
	uint32_t last_cmd;
	uint32_t last_cmd_success;
	uint32_t last_r0;
	uint32_t last_r1;
	uint32_t last_r2;
	uint32_t last_r3;

	void *buf;
	int blocks_to_transfer;
	size_t block_size;
	int card_removal;
};

static uint32_t base_clock;
static struct emmc_block_dev block_dev __attribute__((aligned(4)));
static struct sd_scr sdcard_scr  __attribute__((aligned(4)));
static char *sd_versions[] = { "unknown", "1.0 and 1.01", "1.10", "2.00", "3.0x", "4.xx" };

static uint32_t emmc_get_version(void)
{
	emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
	return emmc->slotisr_ver;//(emmc->slotisr_ver & 0x00FF0000) >> 16;
}

#define SDHCI_DIVIDER_SHIFT		8
#define SDHCI_DIVIDER_HI_SHIFT	6
#define SDHCI_DIV_MASK			0xFF
#define SDHCI_DIV_MASK_LEN		8
#define SDHCI_DIV_HI_MASK		0x300
static uint32_t emmc_get_clock_divider(const uint32_t target_clock)
{
	uint32_t targetted_divisor = 0;

	if (target_clock > base_clock) {
		targetted_divisor = 1;
	}
	else {
		targetted_divisor = base_clock / target_clock;
		uint32_t mod = base_clock % target_clock;
		if (mod) {
			targetted_divisor++;
		}
	}

    int divisor = -1;
    int first_bit;

	for (first_bit = 31; first_bit >= 0; first_bit--) {
		uint32_t bit_test = (1 << first_bit);
		if (targetted_divisor & bit_test) {
			divisor = first_bit;
			targetted_divisor &= ~bit_test;
			if (targetted_divisor) {
				// The divisor is not a power-of-two, increase it
				divisor++;
			}
			break;
		}
	}

    if(divisor == -1) {
        divisor = 31;
    }

    if(divisor >= 32) {
        divisor = 31;
    }

    if(divisor != 0) {
        divisor = (1 << (divisor - 1));
    }

    if(divisor >= 0x400) {
        divisor = 0x3ff;
    }

	uint32_t ret = (divisor & SDHCI_DIV_MASK) << SDHCI_DIVIDER_SHIFT;
	ret |= ((divisor & SDHCI_DIV_HI_MASK) >> SDHCI_DIV_MASK_LEN) << SDHCI_DIVIDER_HI_SHIFT;

	return ret;
}

static uint32_t emmc_set_clock(uint32_t target_clock)
{
	emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);

	uint32_t divider = emmc_get_clock_divider(target_clock);

    if (divider == -1) {
    	SD_TRACE("Couldn't get a valid divider for target clock %d Hz\r\n", target_clock);
		return -1;
	}

	// Wait for the command inhibit (CMD and DAT) bits to clear
	TIMEOUT_WAIT(!(emmc->status & 3), 1000000);

	if ((emmc->status & 3) != 0) {
		SD_TRACE("Timeout waiting for inhibit flags. Status %08x\r\n", emmc->status);
		return -1;
	}

	// Set the SD clock off
	uint32_t control1 = emmc->control1;
	control1 &= ~(1 << 2);
	emmc->control1 = control1;

	udelay(6);

	// Write the new divider
	control1 &= ~0xffe0;		// Clear old setting + clock generator select
	control1 |= divider;
	emmc->control1 = control1;

	udelay(6);

	// Enable the SD clock
	control1 |= (1 << 2);
	emmc->control1 = control1;

	udelay(6);
	TIMEOUT_WAIT(emmc->control1 & (1 << 1), 1000000);

	if ((emmc->control1 & (1 << 1)) == 0) {
		SD_TRACE("Controller's clock did not stabilize within 1 second\r\n");
		return -1;
	}

	SD_TRACE("Successfully set clock rate to %d Hz\r\n", target_clock);

	return 0;
}

static uint32_t emmc_reset(void)
{
	emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
	emmc->control0 = 0;
	emmc->control2 = 0;

	 // Send reset host controller and wait for complete.
	uint32_t control1 = emmc->control1;
	control1 |= (1 << 24);
	emmc->control1 = control1;

	udelay(6);
	TIMEOUT_WAIT((emmc->control1 & (1 << 24)) == 0, 1000000);

	if ((emmc->control1 & (1 << 24)) != 0) {
		SD_TRACE("Controller failed to reset\r\n");
		return -1;
	}

	// Enable internal clock and set data timeout.
	control1 = emmc->control1;
	control1 |= (1 << 0);
	control1 |= 0x000e0000;
	emmc->control1 = control1;

	udelay(6);
	SD_TRACE("control0: %08x, control1: %08x, control2: %08x\r\n", emmc->control0, emmc->control1, emmc->control2);

	if (emmc_set_clock(SD_CLOCK_ID) < 0) {
		return -1;
	}

	// Mask off sending interrupts to the ARM.
	// Reset interrupts.
	// Have all interrupts sent to the INTERRUPT register.
	emmc->irpt_en = 0;
	emmc->interrupt = 0xffffffff;
	emmc->irpt_mask = ~(1 << 8);

	udelay(6);

	return 0;
}

static int emmc_reset_dat(void)
{
	emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);

	uint32_t control1 = emmc->control1;
	control1 |= (1 << 26);
	emmc->control1 = control1;

	udelay(6);
	TIMEOUT_WAIT((emmc->control1 & (1 << 26)) == 0, 1000000);

	if ((emmc->control1 & (1 << 26)) != 0) {
		SD_TRACE("DAT line did not reset properly\r\n");
		return -1;
	}

	return 0;
}

static void emmc_issue_command(uint32_t cmd_reg, uint32_t argument, uint32_t timeout)
{
	emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
	struct emmc_block_dev *dev = &block_dev;

    dev->last_cmd_reg = cmd_reg;
    dev->last_cmd_success = 0;

    // This is as per HCSS 3.7.1.1/3.7.2.2

    // Check Command Inhibit
    while(emmc->status & 1)
    	;

	// Is the command with busy?
	if ((cmd_reg & SD_CMD_RSPNS_TYPE_MASK) == SD_CMD_RSPNS_TYPE_48B) {
		// With busy
		// Is is an abort command?
		if ((cmd_reg & SD_CMD_TYPE_MASK) != SD_CMD_TYPE_ABORT) {
			// Not an abort command
			// Wait for the data line to be free
			while (emmc->status & 0x2)
				;
		}
	}

    // Set block size and block count
    // For now, block size = 512 bytes, block count = 1,
	if (dev->blocks_to_transfer > 0xffff) {
		SD_TRACE("Blocks_to_transfer too great (%i)\r\n", dev->blocks_to_transfer);
		dev->last_cmd_success = 0;
		return;
	}

    uint32_t blksizecnt = dev->block_size | (dev->blocks_to_transfer << 16);
    emmc->blksizecnt = blksizecnt;

    // Set argument 1 reg
    emmc->arg1 = argument;

    // Set command reg
    emmc->cmdtm = cmd_reg;
	udelay(6);

    SD_TRACE("Wait for command complete interrupt\r\n");
    TIMEOUT_WAIT(emmc->interrupt & 0x8001, timeout);

    // Save the interrupt status
    uint32_t irpts = emmc->interrupt;

    // Clear command complete status
    emmc->interrupt = 0xffff0001;

    // Test for errors
	if ((irpts & 0xffff0001) != 0x1) {
		SD_TRACE("Error occurred whilst waiting for command complete interrupt\r\n");
		dev->last_error = irpts & 0xffff0000;
		dev->last_interrupt = irpts;
		return;
	}

    // Get response data
	switch (cmd_reg & SD_CMD_RSPNS_TYPE_MASK) {
	case SD_CMD_RSPNS_TYPE_48:
	case SD_CMD_RSPNS_TYPE_48B:
		dev->last_r0 = emmc->resp0;
		break;
	case SD_CMD_RSPNS_TYPE_136:
		dev->last_r0 = emmc->resp0;
		dev->last_r1 = emmc->resp1;
		dev->last_r2 = emmc->resp2;
		dev->last_r3 = emmc->resp3;
		break;
	}

    // If with data, wait for the appropriate interrupt
    if(cmd_reg & SD_CMD_ISDATA) {
        uint32_t wr_irpt;
        int is_write = 0;

		if (cmd_reg & SD_CMD_DAT_DIR_CH) {
			wr_irpt = (1 << 5);     // read
		} else {
			is_write = 1;
			wr_irpt = (1 << 4);     // write
		}

        int cur_block = 0;
        uint32_t *cur_buf_addr = (uint32_t *)dev->buf;
		while (cur_block < dev->blocks_to_transfer) {
#if 0//def SDCARD_DEBUG
			if(dev->blocks_to_transfer > 1)
				SD_TRACE("Multi block transfer, awaiting block %i ready\r\n", cur_block);
#endif
			TIMEOUT_WAIT(emmc->interrupt & (wr_irpt | 0x8000), timeout);
			irpts = emmc->interrupt;
			emmc->interrupt = 0xffff0000 | wr_irpt;

			if ((irpts & (0xffff0000 | wr_irpt)) != wr_irpt) {
            	SD_TRACE("Error occurred whilst waiting for data ready interrupt\r\n");
                dev->last_error = irpts & 0xffff0000;
                dev->last_interrupt = irpts;
                return;
            }

            // Transfer the block
            size_t cur_byte_no = 0;
			while (cur_byte_no < dev->block_size) {
				if (is_write) {
					emmc->data = *cur_buf_addr;
				} else {
					*cur_buf_addr = emmc->data;
				}
				cur_byte_no += 4;
				cur_buf_addr++;
			}

            SD_TRACE("block %d transfer complete\r\n", cur_block);

            cur_block++;
        }
    }

    // Wait for transfer complete (set if read/write transfer or with busy)
    if((((cmd_reg & SD_CMD_RSPNS_TYPE_MASK) == SD_CMD_RSPNS_TYPE_48B) || (cmd_reg & SD_CMD_ISDATA))) {
        // First check command inhibit (DAT) is not already 0
		if ((emmc->status & 0x2) == 0) {
			emmc->interrupt = 0xffff0002;
		} else {
        	TIMEOUT_WAIT(emmc->interrupt & 0x8002, timeout);
        	irpts = emmc->interrupt;
        	emmc->interrupt = 0xffff0002;

            // Handle the case where both data timeout and transfer complete
            //  are set - transfer complete overrides data timeout: HCSS 2.2.17
			if (((irpts & 0xffff0002) != 0x2) && ((irpts & 0xffff0002) != 0x100002)) {
            	SD_TRACE("Error occurred whilst waiting for transfer complete interrupt\r\n");
                dev->last_error = irpts & 0xffff0000;
                dev->last_interrupt = irpts;
                return;
            }

            emmc->interrupt = 0xffff0002;
        }
    }

    // Return success
    dev->last_cmd_success = 1;
}

static void emmc_handle_card_interrupt(struct emmc_block_dev *dev)
{
	emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
	SD_TRACE("Controller status: %08x\r\n", emmc->status);
	SD_TRACE("Get the card status\r\n");

	if (dev->card_rca) {
		emmc_issue_command(sd_commands[SEND_STATUS], dev->card_rca << 16, 500000);

		if (FAIL(dev)) {
			SD_TRACE("Unable to get card status\r\n");
		} else {
			SD_TRACE("Card status: %08x\r\n", dev->last_r0);
		}
	} else {
		SD_TRACE("No card currently selected\r\n");
	}
}

#define SD_COMMAND_COMPLETE		(1 << 0)
#define SD_TRANSFER_COMPLETE	(1 << 1)
#define SD_BLOCK_GAP_EVENT		(1 << 2)
#define SD_DMA_INTERRUPT		(1 << 3)
#define SD_BUFFER_WRITE_READY	(1 << 4)
#define SD_BUFFER_READ_READY	(1 << 5)
#define SD_CARD_INSERTION		(1 << 6)
#define SD_CARD_REMOVAL			(1 << 7)
#define SD_CARD_INTERRUPT		(1 << 8)
static void emmc_handle_interrupts()
{
	emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
	struct emmc_block_dev *dev = &block_dev;

	const uint32_t irpts = emmc->interrupt;
	uint32_t reset_mask = 0;

	if (irpts & SD_COMMAND_COMPLETE) {
		SD_TRACE("Spurious command complete interrupt\r\n");
		reset_mask |= SD_COMMAND_COMPLETE;
	}

	if (irpts & SD_TRANSFER_COMPLETE) {
		SD_TRACE("Spurious transfer complete interrupt\r\n");
		reset_mask |= SD_TRANSFER_COMPLETE;
	}

	if (irpts & SD_BLOCK_GAP_EVENT) {
		SD_TRACE("Spurious block gap event interrupt\r\n");
		reset_mask |= SD_BLOCK_GAP_EVENT;
	}

	if (irpts & SD_DMA_INTERRUPT) {
		SD_TRACE("Spurious DMA interrupt\r\n");
		reset_mask |= SD_DMA_INTERRUPT;
	}

	if (irpts & SD_BUFFER_WRITE_READY) {
		SD_TRACE("Spurious buffer write ready interrupt\r\n");
		reset_mask |= SD_BUFFER_WRITE_READY;
		emmc_reset_dat();
	}

	if (irpts & SD_BUFFER_READ_READY) {
		SD_TRACE("Spurious buffer read ready interrupt\r\n");
		reset_mask |= SD_BUFFER_READ_READY;
		emmc_reset_dat();
	}

	if (irpts & SD_CARD_INSERTION) {
		SD_TRACE("Card insertion detected\r\n");
		reset_mask |= SD_CARD_INSERTION;
	}

	if (irpts & SD_CARD_REMOVAL) {
		SD_TRACE("Card removal detected\r\n");
		reset_mask |= SD_CARD_REMOVAL;
		dev->card_removal = 1;
	}

	if (irpts & SD_CARD_INTERRUPT) {
		SD_TRACE("Card interrupt detected\r\n");
		emmc_handle_card_interrupt(dev);
		reset_mask |= SD_CARD_INTERRUPT;
	}

	if (irpts & 0x8000) {
		SD_TRACE("Spurious error interrupt: %08x\r\n", irpts);
		reset_mask |= 0xffff0000;
	}

	emmc->interrupt = reset_mask;
}

#define IS_APP_CMD              0x80000000
#define ACMD(a)                 (a | IS_APP_CMD)
#define SET_BUS_WIDTH           (6 | IS_APP_CMD)
#define SD_STATUS               (13 | IS_APP_CMD)
#define SEND_NUM_WR_BLOCKS      (22 | IS_APP_CMD)
#define SET_WR_BLK_ERASE_COUNT  (23 | IS_APP_CMD)
#define SD_SEND_OP_COND         (41 | IS_APP_CMD)
#define SET_CLR_CARD_DETECT     (42 | IS_APP_CMD)
#define SEND_SCR                (51 | IS_APP_CMD)
static void sd_issue_command(uint32_t command, uint32_t argument, uint32_t timeout)
{
	struct emmc_block_dev *dev = &block_dev;

	// First, handle any pending interrupts
	emmc_handle_interrupts();

	// Stop the command issue if it was the card remove interrupt that was handled
	if (dev->card_removal) {
		dev->last_cmd_success = 0;
		return;
	}

	if (command & IS_APP_CMD) {
		command &= 0xff;

		SD_TRACE("Issuing command ACMD%d\r\n", command);

		if (sd_acommands[command] == SD_CMD_RESERVED(0)) {
			SD_TRACE("Invalid command ACMD%d\r\n", command);
			dev->last_cmd_success = 0;
			return;
		}

		dev->last_cmd = APP_CMD;

		uint32_t rca = 0;

		if (dev->card_rca) {
			rca = dev->card_rca << 16;
		}

		emmc_issue_command(sd_commands[APP_CMD], rca, timeout);

		if (dev->last_cmd_success) {
			dev->last_cmd = command | IS_APP_CMD;
			emmc_issue_command(sd_acommands[command], argument, timeout);
		}
	} else {
		SD_TRACE("Issuing command CMD%d\r\n", command);

		if (sd_commands[command] == SD_CMD_RESERVED(0)) {
			SD_TRACE("Invalid command CMD%d\r\n", command);
			dev->last_cmd_success = 0;
			return;
		}

		dev->last_cmd = command;

		emmc_issue_command(sd_commands[command], argument,	timeout);
	}

#if 0//def SD_DEBUG
	if(FAIL(dev))
	{
		SD_TRACE("Error issuing command: interrupts %08x: \r\n", dev->last_interrupt);

		if(dev->last_error == 0) {
			SD_TRACE("TIMEOUT\r\n");
		}
		else
		{
			int i;
			for(i = 0; i < SD_ERR_RSVD; i++)
			{
				if(dev->last_error & (1 << (i + 16)))
				{
					SD_TRACE(err_irpts[i]);
				}
			}
		}
	}
	else
	SD_TRACE("Command completed successfully\r\n");
#endif
}

static int emmc_reset_cmd(void)
{
	emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);

	uint32_t control1 = emmc->control1;
	control1 |= (1 << 25);
	emmc->control1 = control1;
	udelay(6);

	TIMEOUT_WAIT((emmc->control1 & (1 << 25)) == 0, 1000000);

	if ((emmc->control1 & (1 << 25)) != 0) {
		SD_TRACE("CMD line did not reset properly\r\n");
		return -1;
	}

	emmc->interrupt = SD_ERR_MASK_CMD_TIMEOUT;
	udelay(6);

	return 0;
}

static void emmc_set_block_size(uint32_t block_size)
{
	emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
	uint32_t controller_block_size = emmc->blksizecnt;
	controller_block_size &= ~0x3ff;
	controller_block_size |= block_size;
	emmc->blksizecnt = controller_block_size;
	udelay(6);
}
/*
static uint32_t emmc_disable_card_interrupt(void)
{
	emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
	uint32_t old_irpt_mask = emmc->irpt_mask;
	emmc->irpt_mask = old_irpt_mask & (~SD_CARD_INTERRUPT);

	return old_irpt_mask;
}*/

static void emmc_reset_interrupt_register(void)
{
	emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
	emmc->interrupt = 0xffffffff;
}

int sd_card_init(void)
{
	SD_TRACE("EMMC: vendor %x, sdversion %x, slot_status %x\n", emmc_get_version()>>24, (emmc_get_version()>>16)&0xff, emmc_get_version()&0xff);

	//emmc_reg_t *emmc = (emmc_reg_t *)(MMIO_BASE_VA+EMMC_REG);
    //base_clock = ((emmc->capabilities_0 >> 8) & 0xff) * 1000000;

	base_clock = 250000000;//bcm2835_vc_get(BCM2835_VC_TAG_GET_CLOCK_RATE, BCM2835_VC_CLOCK_ID_EMMC);

	SD_TRACE("base_clock = %u\r\n", base_clock);

	if (base_clock < 0) {
		SD_TRACE("Could not get the base clock\r\n");
		return SD_ERROR;
	}

	if (emmc_reset() != 0) {
		SD_TRACE("Controller did not reset properly\r\n");
		return SD_ERROR;
	}

	struct emmc_block_dev *ret = &block_dev;
	memset(ret, 0, sizeof(struct emmc_block_dev));

	ret->bd.block_size = 512;

    /***********************************************************************/
	// Send CMD0 to the card (reset to idle state)
	sd_issue_command(GO_IDLE_STATE, 0, 500000);

	if (FAIL(ret)) {
		SD_TRACE("No CMD0 response\r\n");
		return SD_ERROR;
	}
	// Send CMD8 to the card
	// Voltage supplied = 0x1 = 2.7-3.6V (standard)
	// Check pattern = 10101010b (as per PLSS 4.3.13) = 0xAA
	SD_TRACE("Note a timeout error on the following command (CMD8) is normal and expected if the SD card version is less than 2.0\r\n");
	sd_issue_command(SEND_IF_COND, 0x1aa, 500000);

	int v2_later = 0;

	if (TIMEOUT(ret)) {
		v2_later = 0;
	} else if (CMD_TIMEOUT(ret)) {
		if (emmc_reset_cmd() == -1) {
			return SD_ERROR;
		}
		v2_later = 0;
	} else if (FAIL(ret)) {
		SD_TRACE("Failure sending CMD8 (%08x)\r\n", ret->last_interrupt);
		return SD_ERROR;
	} else {
		if ((ret->last_r0 & 0xfff) != 0x1aa) {
			SD_TRACE("Unusable card, CMD8 response %08x\r\n", ret->last_r0);
			return SD_ERROR;
		} else {
			v2_later = 1;
		}
	}

	// Here we are supposed to check the response to CMD5 (HCSS 3.6) page 100
	// It only returns if the card is a SDIO card
	SD_TRACE("Note that a timeout error on the following command (CMD5) is normal and expected if the card is not a SDIO card\r\n");
	sd_issue_command(IO_SET_OP_COND, 0, 10000);

	if (!TIMEOUT(ret)) {
		if (CMD_TIMEOUT(ret)) {
			if (emmc_reset_cmd() == -1) {
				return SD_ERROR;
			}
		} else {
			SD_TRACE("SDIO card detected - not currently supported, CMD5 returned %08x\r\n", ret->last_r0);
			return SD_ERROR;
		}
	}

    SD_TRACE("Call an inquiry ACMD41 (voltage window = 0) to get the OCR\r\n");
    sd_issue_command(ACMD(41), 0, 500000);

	if (FAIL(ret)) {
		SD_TRACE("Inquiry ACMD41 failed\r\n");
		return SD_ERROR;
	}

    SD_TRACE("Inquiry ACMD41 returned %08x\r\n", ret->last_r0);

	// Call initialization ACMD41
	int card_is_busy = 1;
	while (card_is_busy) {
		uint32_t v2_flags = 0;
		if (v2_later) {
	        // Set SDHC support
	        v2_flags |= (1 << 30);

	        // Set 1.8v support
#ifdef SD_1_8V_SUPPORT
	        if(!ret->failed_voltage_switch)
                v2_flags |= (1 << 24);
#endif

            // Enable SDXC maximum performance
#ifdef SDXC_MAXIMUM_PERFORMANCE
            v2_flags |= (1 << 28);
#endif
	    }

	    sd_issue_command(ACMD(41), 0x00ff8000 | v2_flags, 500000);

		if (FAIL(ret)) {
			SD_TRACE("Error issuing ACMD41\r\n");
			return SD_ERROR;
		}

		if ((ret->last_r0 >> 31) & 0x1) {
			// Initialization is complete
			ret->card_ocr = (ret->last_r0 >> 8) & 0xffff;
			ret->card_supports_sdhc = (ret->last_r0 >> 30) & 0x1;

#ifdef SD_1_8V_SUPPORT
			if(!ret->failed_voltage_switch)
			ret->card_supports_18v = (ret->last_r0 >> 24) & 0x1;
#endif

			card_is_busy = 0;
		} else {
			SD_TRACE("Card is busy, retrying\r\n");
			udelay(500000);
		}
	}

	SD_TRACE("Card identified: OCR: %04x, 1.8v support: %d, SDHC support: %d\r\n", ret->card_ocr, ret->card_supports_18v, ret->card_supports_sdhc);

    // At this point, we know the card is definitely an SD card, so will definitely
	//  support SDR12 mode which runs at 25 MHz
    emmc_set_clock(SD_CLOCK_NORMAL);

#ifdef SD_1_8V_SUPPORT
	// A small wait before the voltage switch
	udelay(5000);

	// Switch to 1.8V mode if possible
	if(ret->card_supports_18v)
	{
		SD_TRACE("SD: switching to 1.8V mode\r\n");
	    // As per HCSS 3.6.1
	    // Send VOLTAGE_SWITCH
	    sd_issue_command(ret, VOLTAGE_SWITCH, 0, 500000);
	    if(FAIL(ret))
	    {
	    	SD_TRACE("SD: error issuing VOLTAGE_SWITCH\r\n");
	        ret->failed_voltage_switch = 1;
	        bcm2835_emmc_sdcard_power_off();
	        return sd_card_init((struct block_device **)&ret);
	    }

	    // Disable SD clock
	    uint32_t control1 = emmc->control1;
	    control1 &= ~BCM2835_EMMC_CLOCK_CARD_EN;
	    emmc->control1 = control1;

	    // Check DAT[3:0]
	    uint32_t status_reg = emmc->status;
	    uint32_t dat30 = (status_reg >> 20) & 0xf;

	    if(dat30 != 0)
	    {
	    	SD_TRACE("DAT[3:0] did not settle to 0\r\n");
	        ret->failed_voltage_switch = 1;
	        bcm2835_emmc_sdcard_power_off();
	        return sd_card_init((struct block_device **)&ret);
	    }

	    // Set 1.8V signal enable to 1
	    uint32_t control0 = emmc->control0;
	    control0 |= (1 << 8);
	    emmc->control0 = control0;

	    // Wait 5 ms
	    udelay(5000);

	    // Check the 1.8V signal enable is set
	    control0 = emmc->control0;
	    if(((control0 >> 8) & 0x1) == 0)
	    {
	    	SD_TRACE("Controller did not keep 1.8V signal enable high\r\n");
	        ret->failed_voltage_switch = 1;
	        bcm2835_emmc_sdcard_power_off();
	        return sd_card_init((struct block_device **)&ret);
	    }

	    // Re-enable the SD clock
	    control1 = emmc->control1;
	    control1 |= (1 << 2);
	    emmc->control1 = control1;

	    // Wait 1 ms
	    udelay(10000);

	    // Check DAT[3:0]
		status_reg = emmc->status;
	    dat30 = (status_reg >> 20) & 0xf;
	    if(dat30 != 0xf)
	    {
	    	SD_TRACE("DAT[3:0] did not settle to 1111b (%01x)\r\n", dat30);
	        ret->failed_voltage_switch = 1;
	        bcm2835_emmc_sdcard_power_off();
	        return sd_card_init((struct block_device **)&ret);
	    }
	    SD_TRACE("SD: voltage switch complete\r\n");
	}
#endif

	SD_TRACE("Send CMD2 to get the cards CID\r\n");
	sd_issue_command(ALL_SEND_CID, 0, 500000);

	if (FAIL(ret)) {
		SD_TRACE("Error sending ALL_SEND_CID\r\n");
		return SD_ERROR;
	}

#if 0//def SD_DEBUG
	uint32_t card_cid_0 = ret->last_r0;
	uint32_t card_cid_1 = ret->last_r1;
	uint32_t card_cid_2 = ret->last_r2;
	uint32_t card_cid_3 = ret->last_r3;
	SD_TRACE("Card CID: %08x%08x%08x%08x\r\n", card_cid_3, card_cid_2, card_cid_1, card_cid_0);
#endif

	SD_TRACE("Send CMD3 to enter the data state\r\n");
	sd_issue_command(SEND_RELATIVE_ADDR, 0, 500000);

	if (FAIL(ret)) {
		SD_TRACE("Error sending SEND_RELATIVE_ADDR\r\n");
		return SD_ERROR;
	}

	uint32_t cmd3_resp = ret->last_r0;
	SD_TRACE("CMD3 response: %08x\r\n", cmd3_resp);

	ret->card_rca = (cmd3_resp >> 16) & 0xffff;
	uint32_t crc_error = (cmd3_resp >> 15) & 0x1;
	uint32_t illegal_cmd = (cmd3_resp >> 14) & 0x1;
	uint32_t error = (cmd3_resp >> 13) & 0x1;
	uint32_t status = (cmd3_resp >> 9) & 0xf;
	uint32_t ready = (cmd3_resp >> 8) & 0x1;

	if (crc_error) {
		SD_TRACE("CRC error\r\n");
		return SD_ERROR;
	}

	if (illegal_cmd) {
		SD_TRACE("Illegal command\r\n");
		return SD_ERROR;
	}

	if (error) {
		SD_TRACE("Generic error\r\n");
		return SD_ERROR;
	}

	if (!ready) {
		SD_TRACE("Not ready for data\r\n");
		return SD_ERROR;
	}

	SD_TRACE("RCA: %04x\r\n", ret->card_rca);

	// Now select the card (toggles it to transfer state)
	sd_issue_command(SELECT_CARD, ret->card_rca << 16, 500000);

	if (FAIL(ret)) {
		SD_TRACE("Error sending CMD7\r\n");
		return SD_ERROR;
	}

	uint32_t cmd7_resp = ret->last_r0;
	status = (cmd7_resp >> 9) & 0xf;

	SD_TRACE("Status (%d)\r\n", status);

	if ((status != 2) && (status != 3) && (status != 4)) {
		SD_TRACE("Invalid status (%d)\r\n", status);
		return SD_ERROR;
	}

	// If not an SDHC card, ensure BLOCKLEN is 512 bytes
	if (!ret->card_supports_sdhc) {
		sd_issue_command(SET_BLOCKLEN, 512, 500000);
		SD_TRACE("CMD16 response: %08x, Statud %d\r\n", ret->last_r0, (ret->last_r0 >> 9) & 0xf);
		if (FAIL(ret)) {
			SD_TRACE("Error sending SET_BLOCKLEN\r\n");
			return SD_ERROR;
		}
	}

	ret->block_size = 512;

	emmc_set_block_size(ret->block_size);

	// Get the cards SCR register
	// SEND_SCR command is like a READ_SINGLE but for a block of 8 bytes.
	// Ensure that any data operation has completed before reading the block.
	ret->scr = &sdcard_scr;
	ret->buf = &ret->scr->scr[0];
	ret->block_size = 8;
	ret->blocks_to_transfer = 1;

	sd_issue_command(SEND_SCR, 0, 500000);
	SD_TRACE("ACMD51 response: %08x, Status %d\r\n", ret->last_r0, (ret->last_r0 >> 9) & 0xf);

	ret->block_size = 512;

	if (FAIL(ret)) {
		SD_TRACE("Error sending SEND_SCR\r\n");
		return SD_ERROR;
	}

	// Determine card version
	// Note that the SCR is big-endian
	uint32_t scr0 = __builtin_bswap32(ret->scr->scr[0]);

	ret->scr->sd_version = SD_VER_UNKNOWN;

	uint32_t sd_spec = (scr0 >> (56 - 32)) & 0xf;
	uint32_t sd_spec3 = (scr0 >> (47 - 32)) & 0x1;
	uint32_t sd_spec4 = (scr0 >> (42 - 32)) & 0x1;

	ret->scr->sd_bus_widths = (scr0 >> (48 - 32)) & 0xf;

	if (sd_spec == 0) {
		ret->scr->sd_version = SD_VER_1;
	} else if (sd_spec == 1) {
		ret->scr->sd_version = SD_VER_1_1;
	} else if (sd_spec == 2) {
		if (sd_spec3 == 0) {
			ret->scr->sd_version = SD_VER_2;
		} else if (sd_spec3 == 1) {
			if (sd_spec4 == 0) {
				ret->scr->sd_version = SD_VER_3;
			} else if (sd_spec4 == 1) {
				ret->scr->sd_version = SD_VER_4;
			}
		}
	}

	SD_TRACE("&scr: %08x\r\n", &ret->scr->scr[0]);
	SD_TRACE("SCR[0]: %08x, SCR[1]: %08x\r\n", ret->scr->scr[0], ret->scr->scr[1]);
	SD_TRACE("SCR: %08x%08x\r\n", __builtin_bswap32(ret->scr->scr[0]), __builtin_bswap32(ret->scr->scr[1]));
	SD_TRACE("SCR: version %s, bus_widths %01x\r\n", sd_versions[ret->scr->sd_version], ret->scr->sd_bus_widths);

#ifdef SD_4BIT_DATA
	if (ret->scr->sd_bus_widths & 0x4) {
		SD_TRACE("Switching to 4-bit data mode\r\n");

		uint32_t old_irpt_mask = emmc_disable_card_interrupt();

		// Send ACMD6 to change the card's bit mode
		sd_issue_command(SET_BUS_WIDTH, 0x2, 500000);

		if (FAIL(ret)) {
			SD_TRACE("Switch to 4-bit data mode failed\r\n");
		} else {
			emmc_4bit_mode_change_bit(old_irpt_mask);
			SD_TRACE("Switch to 4-bit complete\r\n");
		}
	}
#endif

    SD_TRACE("Found a valid version %s SD card\r\n", sd_versions[ret->scr->sd_version]);
	SD_TRACE("Setup successful (status %d)\r\n", (ret->last_r0 >> 9) & 0xf);

	emmc_reset_interrupt_register();

	return SD_OK;
}

static int sd_ensure_data_mode(void)
{
	struct emmc_block_dev *edev = &block_dev;

	if (edev->card_rca == 0) {
		int ret = sd_card_init();

		if (ret != 0) {
			return ret;
		}
	}

	SD_TRACE("Obtaining status register for edev->card_rca = %08x\r\n", edev->card_rca);

	sd_issue_command(SEND_STATUS, edev->card_rca << 16, 500000);

	if (FAIL(edev)) {
		SD_TRACE("Error sending CMD13\r\n");
		edev->card_rca = 0;
		return -1;
	}

	uint32_t status = edev->last_r0;
	uint32_t cur_state = (status >> 9) & 0xf;

	SD_TRACE("status %d", cur_state);

	if (cur_state == 3) {
		// Currently in the stand-by state - select it
		sd_issue_command(SELECT_CARD, edev->card_rca << 16, 500000);
		if (FAIL(edev)) {
			SD_TRACE("No response from CMD17\r\n");
			edev->card_rca = 0;
			return -1;
		}
	} else if (cur_state == 5) {
		// In the data transfer state - cancel the transmission
		sd_issue_command(STOP_TRANSMISSION, 0, 500000);
		if (FAIL(edev)) {
			SD_TRACE("No response from CMD12\r\n");
			edev->card_rca = 0;
			return -1;
		}

		// Reset the data circuit
		emmc_reset_dat();
	} else if (cur_state != 4) {
		// Not in the transfer state - re-initialize
		int ret = sd_card_init();

		if (ret != 0) {
			return ret;
		}
	}

	// Check again that we're now in the correct mode
	if (cur_state != 4) {
		SD_TRACE("Re-checking status: \r\n");

		sd_issue_command(SEND_STATUS, edev->card_rca << 16, 500000);

		if (FAIL(edev)) {
			SD_TRACE("No response from CMD13\r\n");
			edev->card_rca = 0;
			return -1;
		}

		status = edev->last_r0;
		cur_state = (status >> 9) & 0xf;

		SD_TRACE("%d", cur_state);

		if (cur_state != 4) {
			SD_TRACE("Unable to initialize SD card to data mode (state %d)\r\n", cur_state);
			edev->card_rca = 0;
			return -1;
		}
	}

	return 0;
}

static int sd_do_data_command(int is_write, uint8_t *buf, size_t buf_size, uint32_t block_no)
{
	struct emmc_block_dev *edev = &block_dev;

	// PLSS table 4.20 - SDSC cards use byte addresses rather than block addresses
	if(!edev->card_supports_sdhc) {
		block_no *= 512;
	}

	// This is as per HCSS 3.7.2.1
	if (buf_size < edev->block_size) {
		SD_TRACE("buffer size (%d) is less than block size (%d)\r\n", buf_size, edev->block_size);
        return -1;
	}

	edev->blocks_to_transfer = buf_size / edev->block_size;

	if (buf_size % edev->block_size) {
		SD_TRACE("buffer size (%d) is not an exact multiple of block size (%d)\r\n", buf_size, edev->block_size);
		return -1;
	}

	edev->buf = buf;

	// Decide on the command to use
	int command;

	if (is_write) {
		if (edev->blocks_to_transfer > 1) {
			command = WRITE_MULTIPLE_BLOCK;
		} else {
			command = WRITE_BLOCK;
		}
	} else {
		if (edev->blocks_to_transfer > 1) {
			command = READ_MULTIPLE_BLOCK;
		} else {
			command = READ_SINGLE_BLOCK;
		}
	}

	int retry_count = 0;
	int max_retries = 3;

	while (retry_count < max_retries) {
		sd_issue_command(command, block_no, 5000000);

		if (SUCCESS(edev)) {
			break;
		} else {
			SD_TRACE("Error sending CMD%d, edev->last_error = %08x\r\n", command, edev->last_error);
			retry_count++;

			if (retry_count < max_retries) {
				SD_TRACE("Retrying...%d\r\n", retry_count);
			} else {
				SD_TRACE("Giving up...%d\r\n", max_retries);
			}
		}
	}

	if (retry_count == max_retries) {
		edev->card_rca = 0;
		return -1;
	}

	return 0;
}

int sd_read(uint8_t *buf, size_t buf_size, uint32_t block_no)
{
	if (sd_ensure_data_mode() != 0) {
		return -1;
	}

	SD_TRACE("Card ready, reading from block %u\r\n", block_no);

	if (sd_do_data_command(0, buf, buf_size, block_no) < 0) {
		return -1;
	}

	SD_TRACE("Data read successful\r\n");

	return buf_size;
}

/*****************************************************************************/
#include "kernel.h"

static int sd_init1(struct dev *this, int minor);
static int sd_read1(struct dev *this, uint8_t *buf, size_t buf_size, uint32_t addr);

struct sd_dev {
	struct dev dev;
} sd_dev =
{
	{
	.init = sd_init1,
	.uninit = NULL,
	.read = sd_read1,
	.write = NULL,
	.poll = NULL,
	.ioctl = NULL
	}
};

static int sd_init1(struct dev *this, int minor)
{
	struct sd_dev *sd_dev = (struct sd_dev *)this;
	return sd_card_init();
}

static int sd_read1(struct dev *this, uint8_t *buf, size_t buf_size, uint32_t addr)
{
	struct sd_dev *sd_dev = (struct sd_dev *)this;
	return sd_read(buf, buf_size, addr/block_dev.block_size);
}
/*****************************************************************************/