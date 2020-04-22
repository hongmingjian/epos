/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 *
 * Copyright (C) 2005, 2008, 2013 Hong MingJian<hongmingjian@gmail.com>
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
#ifndef _BOARD_H
#define _BOARD_H

// Model within the boardid
#define MODEL_1B         0x0
#define MODEL_1B_PLUS    0x1
#define MODEL_2B         0x4
#define MODEL_3B_PLUS    0xD

#define BOARD_MODEL(x) (((x)>>4)&0xff)

#ifndef __ASSEMBLY__
#include <stdint.h>
/*--------------------------------------------------------------------------}
{                   RETURNED FROM TAG_GET_BOARD_REVISION                    }
{--------------------------------------------------------------------------*/
extern uint32_t boardid;
/*
    struct __attribute__((__packed__))
    {
        unsigned revision : 4;      //Board revison
        unsigned model: 8;          //Model
        unsigned cpu : 4;           //CPU
#define BCM2835 0
#define BCM2836 1
#define BCM2837 2

        unsigned manufacturer : 4;  //Manufacturer ID
        unsigned memorysize : 3;    //Memory size on board
        unsigned revision1 : 1;     //Revision 0 or 1
        unsigned reserved : 8;      //reserved
    };
*/

#endif /*__ASSEMBLY__*/

#endif /*_BOARD_H*/
