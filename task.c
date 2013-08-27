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
  struct tcb *select = g_task_running;
  do {
    select = select->all_next;
    if(select == NULL)
      select = g_task_all_head;
    if(select == g_task_running)
      break;
    if((select->tid != 0) && 
       (select->state == TASK_STATE_READY))
      break;
  } while(1);

  if(select == g_task_running) {
    if(select->state == TASK_STATE_READY)
      return;
    select = g_task_all_head;
  }

//  printk("0x%x -> 0x%x\n\r", (g_task_running == NULL) ? -1 : g_task_running->tid, select->tid);

  g_resched = 0;
  switch_to(select);
}

void sleep_on(struct wait_queue **head)
{
  struct wait_queue wait;

  wait.tsk = g_task_running;
  wait.next = *head;
  *head = &wait;

  g_task_running->state = TASK_STATE_BLOCKED;
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

void wake_up(struct wait_queue **head, int n)
{
  struct wait_queue *p;

  for(p = *head; (p!=NULL) && n; p = p->next, n--)
    p->tsk->state = TASK_STATE_READY;
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

static
struct tcb *task_get(int tid)
{
  uint32_t flags;
  struct tcb *p;
  save_flags_cli(flags);
  p = find_task(tid);
  restore_flags(flags);
  return p;
}

int sys_task_create(uint32_t user_stack, 
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
  new->wait_head = NULL;
  new->all_next = NULL;

  if(user_stack != 0) {
    PUSH_TASK_STACK(user_stack, param);
    PUSH_TASK_STACK(user_stack, 0);
  } else {
    PUSH_TASK_STACK(new->kern_stack, param);
    PUSH_TASK_STACK(new->kern_stack, 0);
  }

  INIT_TASK_CONTEXT(user_stack, new->kern_stack, handler);

	save_flags_cli(flags);
  add_task(new);
	restore_flags(flags);	

	return new->tid;
}

void sys_task_exit(int val)
{	
  uint32_t flags;

  save_flags_cli(flags);

  wake_up(&g_task_running->wait_head, -1);

	g_task_running->exit_code = val;
	g_task_running->state = TASK_STATE_ZOMBIE;

	schedule();
}

int sys_task_wait(int32_t tid, int32_t *exit_code)
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

	if(tsk->state != TASK_STATE_ZOMBIE)
    sleep_on(&tsk->wait_head);

  if(exit_code != NULL)
	  *exit_code = tsk->exit_code;
			
	if(tsk->wait_head == NULL) {
    char *p;
	  remove_task(tsk);
    restore_flags(flags);

//    printk("%d: Task %d reaped\n\r", sys_task_getid(), tsk->tid);

    p = (char *)tsk;
    p += sizeof(struct tcb);
    p -= (PAGE_SIZE + PAGE_SIZE);
    kfree(p);
    return 0;
	}
		
  restore_flags(flags);
	return 0;
}

int32_t sys_task_getid()
{
  return (g_task_running==NULL)?-1:g_task_running->tid;
}

void sys_task_yield()
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
//  printk("%d: ticks=%d, wakeup %d\n\r", sys_task_getid(), ticks, tsk->tid);
}

int sys_task_sleep(uint32_t msec)
{
  if(NULL != set_callout(1+(HZ*msec)/1000, task_sleep_callout, g_task_running)) {
    uint32_t flags;
//    printk("%d: sleep %d ticks, ticks=%d\n\r", sys_task_getid(), 1+(HZ*msec)/1000, ticks);
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

  task0 = task_get(sys_task_create(0, 0/*to be filled by run_as_task0*/, NULL));
}
