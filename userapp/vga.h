/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 *
 * Copyright (C) 2015 Hong MingJian<hongmingjian@gmail.com>
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
#ifndef _VGA_H
#define _VGA_H

#include <inttypes.h>

struct vga_dev {
    uint16_t XResolution;
    uint16_t YResolution;
    uint16_t BytesPerScanLine;
    uint8_t  BitsPerPixel;
    uint8_t  NumberOfPlanes;
    uint8_t *FrameBuffer;
    uint32_t FrameBufferSize;
    int      Linear;
    int    (*pfnSwitchBank)(int bank);
};

extern struct vga_dev g_vga_dev;

/**
 * 列出系统支持的图形模式，包括分辨率和像素位数
 *
 * 注意：该函数只能在文本模式下面运行！
 *       否则，看不到任何输出！
 *
 */
int list_vga_modes();

/**
 * 进入由mode指定的图形模式
 */
int init_vga(int mode);

/**
 * 退回到原来的模式
 */
int exit_vga();

#endif /*_VGA_H*/
