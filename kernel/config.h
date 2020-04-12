/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 *
 * Copyright (C) 2005, 2008, 2013, 2020 Hong MingJian<hongmingjian@gmail.com>
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
#ifndef _CONFIG_H
#define _CONFIG_H

#if RPI_QEMU == 1
#define LOADADDR 0x10000
#else
#define LOADADDR 0x8000
#endif

#define KERNBASE 0xC0000000
#define R(x) ((x)-KERNBASE)

#define MMIO_BASE_VA 0xC4000000

#endif /* _CONFIG_H */
