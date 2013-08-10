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
#include "utils.h"
#include "kernel.h"

int g_resched;
struct tcb *g_task_running;
struct tcb *g_task_all_head;
struct tcb *task0;

void schedule()
{
	struct tcb *select = NULL;

    if(g_task_running != NULL) {
	    struct tcb *tsk = g_task_running->all_next;
        do {
            if(tsk == NULL)
                tsk = g_task_all_head;
            if(tsk == g_task_running)
                break;
            if((tsk->tid != 0) && 
               (tsk->state == TASK_STATE_READY)) {
                select = tsk;
                break;
            }
            tsk = tsk->all_next;
        } while(1);
    }

    if(select == NULL)
        select = g_task_all_head;

    if((select == NULL) || 
       (select == g_task_running))
        return;

//    printk("0x%x -> 0x%x\n\r", (g_task_running == NULL) ? -1 : g_task_running->tid, select->tid);

    g_resched = 0;
    switch_to(select);
}

static
void add_task(struct tcb *tsk)
{
    if(g_task_all_head == NULL)
        g_task_all_head = tsk;
    else {
        struct tcb *p, *q;
        p = g_task_all_head;
        do {
            q = p;
            p = p->all_next;
        } while(p != NULL);
        q->all_next = tsk;
    }
}

static
void remove_task(struct tcb *tsk)
{
    if(g_task_all_head != NULL) {
        if(tsk == g_task_all_head) {
            g_task_all_head = g_task_all_head->all_next;
        } else {
            struct tcb *p, *q;
            p = g_task_all_head;
            do {
                q = p;
                p = p->all_next;
                if(p == tsk)
                    break;
            } while(p != NULL);

            if(p == tsk)
                q->all_next = p->all_next;
        }
    }
}

static
struct tcb* find_task(int tid)
{
	struct tcb *tsk;
	
    tsk = g_task_all_head;
    while(tsk != NULL) {
        if(tsk->tid == tid)
            break;
        tsk = tsk->all_next;
    }
 
	return tsk;
}

int task_create(uint32_t user_stack, 
	  void (*handler)(void *), void *param)
{
  static int tid = 0;
  struct tcb *new;
  char *p;
  uint32_t flags;

  if(user_stack & 3)
    return -1;
    
	p = (char *)kmalloc(PAGE_SIZE+PAGE_SIZE);
  if(p == NULL)
    return -2;

  p = p + PAGE_SIZE+PAGE_SIZE;
  p -= sizeof(struct tcb);
  new = (struct tcb *)p;

	memset(new, 0, sizeof(struct tcb));

	new->kern_stack = (uint32_t)new;
	new->tid = tid++;
	new->state = TASK_STATE_READY;
	new->quantum = DEFAULT_QUANTUM;
  new->wait_cnt = 0;
  new->wait_head = NULL;
  new->wait_next = NULL;
  new->sem_next = NULL;
  new->all_next = NULL;

  if(user_stack != 0) {
    PUSH_TASK_STACK(user_stack, param);
    PUSH_TASK_STACK(user_stack, 0);
  }

  INIT_TASK_CONTEXT(user_stack, new->kern_stack, handler);

	save_flags_cli(flags);
  add_task(new);
	restore_flags(flags);	

	return new->tid;
}

void task_exit(int val)
{	
    uint32_t flags;

    save_flags_cli(flags);

//    if(g_task_running->wait_cnt > 0)
    {
        struct tcb *tsk = g_task_running->wait_head;
        while(tsk != NULL) {
            tsk->state = TASK_STATE_READY;
            tsk = tsk->wait_next;
        }
    }

	g_task_running->exit_code = val;
	g_task_running->state = TASK_STATE_ZOMBIE;

	schedule();
}

int task_wait(int32_t tid, int32_t *exit_code)
{
	uint32_t flags;
	struct tcb *tsk;

  if(g_task_running == NULL)
    return -1;

  save_flags_cli(flags);

	if((tsk = find_task(tid)) == NULL) {
	  restore_flags(flags);
		return -1;
  }

	if(tsk->state != TASK_STATE_ZOMBIE) {
    struct tcb *p;

	  tsk->wait_cnt++;

    p = tsk->wait_head;
    tsk->wait_head = g_task_running;
    g_task_running->wait_next = p;

	  g_task_running->state = TASK_STATE_BLOCKED;
	  schedule();

	  tsk->wait_cnt--;
	}

  if(exit_code != NULL)
	  *exit_code = tsk->exit_code;
			
	if(tsk->wait_cnt == 0) {
    char *p;
	  remove_task(tsk);
//    printk("%d: Task %d reaped\n\r", g_task_running->tid, tsk->tid);
    restore_flags(flags);

    p = (char *)tsk;
    p += sizeof(struct tcb);
    p -= (PAGE_SIZE + PAGE_SIZE);
    kfree(p);
    return 0;
	}
		
  restore_flags(flags);
	return 0;
}

int32_t task_getid()
{
    return (g_task_running==NULL)?-1:g_task_running->tid;
}

struct tcb *task_get(int tid)
{
  uint32_t flags;
  struct tcb *p;
  save_flags_cli(flags);
  p = find_task(tid);
  restore_flags(flags);
  return p;
}

void task_yield()
{
    uint32_t flags;
    save_flags_cli(flags);
    schedule();
    restore_flags(flags);
}

static void task_sleep_callout(void *pv)
{
    struct tcb *tsk = (struct tcb *)pv;
    tsk->state = TASK_STATE_READY;
//    printk("%d: ticks=%d, wakeup %d\n\r", task_getid(), ticks, tsk->tid);
}

int task_sleep(uint32_t msec)
{
    if(NULL != set_callout(1+(HZ*msec)/1000, task_sleep_callout, g_task_running)) {
        uint32_t flags;
//        printk("%d: sleep %d ticks, ticks=%d\n\r", task_getid(), 1+(HZ*msec)/1000, ticks);
        save_flags_cli(flags);
        g_task_running->state = TASK_STATE_BLOCKED;
        schedule();
        restore_flags(flags);
        return 0;
    }

    return -1;
}

void init_task()
{
  g_resched = 0;
  g_task_running = NULL;
  g_task_all_head = NULL;

  task0 = task_get(task_create(0, 0/*to be filled by move_to_task0*/, NULL));
}
