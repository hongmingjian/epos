/*
GazOS Operating System
Copyright (C) 1999  Gareth Owen <gaz@athene.co.uk>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "loader.h"

/* Defines for accessing the upper and lower byte of an integer. */
#define LO_BYTE(x)           ((x) & 0xFF)
#define HI_BYTE(x)          (((x) & 0xFF00) >> 8)

/* Quick-access registers and ports for each DMA channel. */
static unsigned char MaskReg[8]   = { 0x0A, 0x0A, 0x0A, 0x0A, 0xD4, 0xD4, 0xD4, 0xD4 };
static unsigned char ModeReg[8]   = { 0x0B, 0x0B, 0x0B, 0x0B, 0xD6, 0xD6, 0xD6, 0xD6 };
static unsigned char ClearReg[8]  = { 0x0C, 0x0C, 0x0C, 0x0C, 0xD8, 0xD8, 0xD8, 0xD8 };

static unsigned char PagePort[8]  = { 0x87, 0x83, 0x81, 0x82, 0x8F, 0x8B, 0x89, 0x8A };
static unsigned char AddrPort[8]  = { 0x00, 0x02, 0x04, 0x06, 0xC0, 0xC4, 0xC8, 0xCC };
static unsigned char CountPort[8] = { 0x01, 0x03, 0x05, 0x07, 0xC2, 0xC6, 0xCA, 0xCE };

static void _dma_xfer(unsigned char DMA_channel, unsigned char page, unsigned int offset, unsigned int length, unsigned char mode)
{
    /* Don't let anyone else mess up what we're doing. */
    cli();

    /* Set up the DMA channel so we can use it.  This tells the DMA */
    /* that we're going to be using this channel.  (It's masked) */
    outportb(MaskReg[DMA_channel], 0x04 | DMA_channel);

    /* Clear any data transfers that are currently executing. */
//    outportb(ClearReg[DMA_channel], 0x00);
    outportb (0xd8,0xff);	//reset master flip-flop

    /* Send the specified mode to the DMA. */
    outportb(ModeReg[DMA_channel], mode);

    /* Send the offset address.  The first byte is the low base offset, the */
    /* second byte is the high offset. */
    outportb(AddrPort[DMA_channel], LO_BYTE(offset));
    outportb(AddrPort[DMA_channel], HI_BYTE(offset));

    outportb (0xd8,0xff);	//reset master flip-flop

    /* Send the physical page that the data lies on. */
//    outportb(PagePort[DMA_channel], page);

    /* Send the length of the data.  Again, low byte first. */
    outportb(CountPort[DMA_channel], LO_BYTE(length));
    outportb(CountPort[DMA_channel], HI_BYTE(length));

    outportb (0x80,0);

    /* Ok, we're done.  Enable the DMA channel (clear the mask). */
    outportb(MaskReg[DMA_channel], DMA_channel);

    /* Re-enable interrupts before we leave. */
    sti();
}

void dma_xfer(uint8_t channel, uint32_t address, size_t length, int read)
{
	unsigned char page, mode;
	unsigned int offset;
	
	if(read)
		mode = 0x54 + channel;
	else
		mode = 0x58 + channel;
		
	page = address >> 16;
	offset = address & 0xFFFF;
	length--;
	
	_dma_xfer(channel, page, offset, length, mode);	
}	
	
