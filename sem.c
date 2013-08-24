/**
 *
 * Copyright (C) 2008, 2013 Hong MingJian
 * All rights reserved.
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
#include "kernel.h"
#include "utils.h"

struct sem {
    int32_t value;
    struct wait_queue *wait_head;
};

void *sem_create(int32_t value)
{
    struct sem *sem;

    sem = (struct sem *)kmalloc(sizeof(struct sem));
    if(sem == NULL) {
        return NULL;
    }

    sem->value = value;
    sem->wait_head = NULL;

    return sem;
}

int sem_destroy(void *s)
{
    uint32_t flags;
    struct sem *sem = (struct sem *)s;

    save_flags_cli(flags);

    if(sem->wait_head != NULL) {
        restore_flags(flags);
        return -1;
    }

    restore_flags(flags);

    kfree(sem);
    return 0;
}

int sem_wait(void *s)
{
    uint32_t flags;
    struct sem *sem = (struct sem *)s;

    save_flags_cli(flags);

    sem->value--;
    if(sem->value < 0)
      sleep_on(&sem->wait_head);

    restore_flags(flags);
    return 0;
}

int sem_signal(void *s)
{
    uint32_t flags;
    struct sem *sem = (struct sem *)s;

    save_flags_cli(flags);

    sem->value++;
    if(sem->value <= 0)
      wake_up(&sem->wait_head, 1);

    restore_flags(flags);
    return 0;
}
