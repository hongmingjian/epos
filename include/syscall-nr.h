/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 *
 * Copyright (C) 2013 Hong MingJian<hongmingjian@gmail.com>
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
#ifndef _SYSCALLNR_H
#define _SYSCALLNR_H

#define SYSCALL_task_exit     1
#define SYSCALL_task_create   2
#define SYSCALL_task_getid    3
#define SYSCALL_task_yield    4
#define SYSCALL_task_wait     5

#define SYSCALL_mmap          7
#define SYSCALL_munmap        8
#define SYSCALL_sleep         9
#define SYSCALL_nanosleep     10
#define SYSCALL_gettimeofday  11

#define SYSCALL_open          20
#define SYSCALL_close         21
#define SYSCALL_read          22
#define SYSCALL_write         23
#define SYSCALL_lseek         24

#endif /*_SYSCALLNR_H*/
