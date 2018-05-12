/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 *
 * Copyright (C) 2008, 2013 Hong MingJian<hongmingjian@gmail.com>
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
#include <stddef.h>
#include <string.h>
#include "kernel.h"

int g_resched;
struct tcb *g_task_head;
struct tcb *g_task_running;
struct tcb *task0;
struct tcb *g_task_own_fpu;

/**
 * CPU调度器函数，这里只实现了轮转调度算法
 *
 * 注意：该函数的执行不能被中断
 */
void schedule()
{
    struct tcb *select = g_task_running;
    do {
        select = select->next;
        if(select == NULL)
            select = g_task_head;
        if(select == g_task_running)
            break;
        if((select->tid != 0) &&
           (select->state == TASK_STATE_READY))
            break;
    } while(1);

    if(select == g_task_running) {
        if(select->state == TASK_STATE_READY)
            return;
        select = task0;
    }

    //printk("0x%d -> 0x%d\r\n", (g_task_running == NULL) ? -1 : g_task_running->tid, select->tid);

    if(select->signature != TASK_SIGNATURE)
        printk("warning: kernel stack of task #%d overflow!!!", select->tid);

    g_resched = 0;
    switch_to(select);
}

/**
 * 把当前线程切换为等待状态，等待在*head队列中
 *
 * 注意：该函数的执行不能被中断
 */
void sleep_on(struct wait_queue **head)
{
    struct wait_queue wait;

    wait.tsk = g_task_running;
    wait.next = *head;
    *head = &wait;

    g_task_running->state = TASK_STATE_WAITING;
    schedule();

    if(*head == &wait)
        *head = wait.next;
    else {
        struct wait_queue *p, *q;
        p = *head;
        do {
            q = p;
            p = p->next;
            if(p == &wait) {
                q->next = p->next;
                break;
            }
        } while(p != NULL);
    }
}

/**
 * 唤醒n个等待在*head队列中的线程。
 * 如果n<0，唤醒队列中的所有线程
 *
 * 注意：该函数的执行不能被中断
 */
void wake_up(struct wait_queue **head, int n)
{
    struct wait_queue *p;

    for(p = *head; (p!=NULL) && n; p = p->next, n--)
        p->tsk->state = TASK_STATE_READY;
}

static
void add_task(struct tcb *tsk)
{
    if(g_task_head == NULL)
        g_task_head = tsk;
    else {
        struct tcb *p, *q;
        p = g_task_head;
        do {
            q = p;
            p = p->next;
        } while(p != NULL);
        q->next = tsk;
    }
}

static
void remove_task(struct tcb *tsk)
{
    if(g_task_head != NULL) {
        if(tsk == g_task_head) {
            g_task_head = g_task_head->next;
        } else {
            struct tcb *p, *q;
            p = g_task_head;
            do {
                q = p;
                p = p->next;
                if(p == tsk)
                    break;
            } while(p != NULL);

            if(p == tsk)
                q->next = p->next;
        }
    }
}

static
struct tcb* get_task(int tid)
{
    struct tcb *tsk;

    tsk = g_task_head;
    while(tsk != NULL) {
        if(tsk->tid == tid)
            break;
        tsk = tsk->next;
    }

    return tsk;
}

/**
 * 系统调用task_create的执行函数
 *
 * 创建一个新的线程，该线程执行func函数，并向新线程传递参数pv
 */
struct tcb *sys_task_create(void *tos,
                            void (*func)(void *pv), void *pv)
{
    static int tid = 0;
    struct tcb *new;
    char *p;
    uint32_t flags;
    uint32_t ustack=(uint32_t)tos;

    if(ustack & 3)
        return NULL;

    p = (char *)kmemalign(PAGE_SIZE, PAGE_SIZE);
    if(p == NULL)
        return NULL;

    new = (struct tcb *)p;

    memset(new, 0, sizeof(struct tcb));

    new->kstack = (uint32_t)(p+PAGE_SIZE);
    new->tid = tid++;
    new->state = TASK_STATE_READY;
    new->timeslice = TASK_TIMESLICE_DEFAULT;
    new->wq_exit = NULL;
    new->next = NULL;
    new->signature = TASK_SIGNATURE;

    /*XXX - should be elsewhere*/
    new->fpu.cwd = 0x37f;
    new->fpu.twd = 0xffff;

    INIT_TASK_CONTEXT(ustack, new->kstack, func, pv);

    save_flags_cli(flags);
    add_task(new);
    restore_flags(flags);

    return new;
}

/**
 * 系统调用task_exit的执行函数
 *
 * 结束当前线程，code_exit是它的退出代码
 */
void sys_task_exit(int code_exit)
{
    uint32_t flags;

    save_flags_cli(flags);

    wake_up(&g_task_running->wq_exit, -1);

    g_task_running->code_exit = code_exit;
    g_task_running->state = TASK_STATE_ZOMBIE;

    if(g_task_own_fpu == g_task_running)
        g_task_own_fpu = NULL;

    schedule();
}

/**
 * 系统调用task_wait的执行函数
 *
 * 当前线程等待线程tid结束执行。
 * 如果pcode_exit不是NULL，用于保存线程tid的退出代码
 */
int sys_task_wait(int tid, int *pcode_exit)
{
    uint32_t flags;
    struct tcb *tsk;

    if(g_task_running == NULL)
        return -1;

    if(tid == 0)
        return -1;

    save_flags_cli(flags);

    if((tsk = get_task(tid)) == NULL) {
        restore_flags(flags);
        return -1;
    }

    if(tsk->state != TASK_STATE_ZOMBIE)
        sleep_on(&tsk->wq_exit);

    if(pcode_exit != NULL)
        *pcode_exit= tsk->code_exit;

    if(tsk->wq_exit == NULL) {
        remove_task(tsk);
        //printk("%d: Task %d reaped\r\n", sys_task_getid(), tsk->tid);
        restore_flags(flags);

        kfree(tsk);
        return 0;
    }

    restore_flags(flags);
    return 0;
}

/**
 * 系统调用task_getid的执行函数
 *
 * 获取当前线程的tid
 */
int sys_task_getid()
{
    return (g_task_running==NULL)?-1:g_task_running->tid;
}

/**
 * 系统调用task_yield的执行函数
 *
 * 当前线程主动放弃CPU，让调度器调度其他线程运行
 */
void sys_task_yield()
{
    uint32_t flags;
    save_flags_cli(flags);
    schedule();
    restore_flags(flags);
}

/**
 * 初始化多线程子系统
 */
void init_task()
{
    g_resched = 0;
    g_task_running = NULL;
    g_task_head = NULL;
    g_task_own_fpu = NULL;

    /*
     * 创建线程task0，即系统空闲线程
     */
    task0 = sys_task_create(NULL, NULL/*task0执行的函数将由run_as_task0填充*/, NULL);
}
