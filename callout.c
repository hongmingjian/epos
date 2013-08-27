/**
 *
 * Copyright (C) 2005, 2008, 2013 Hong MingJian
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

struct callout *g_callout_head;
void *g_sem_callout;

static void *sem_mutex;

static
void task_callout(void *pv)
{
	while(1) {
		sem_wait(g_sem_callout);
    while(1) {
      sem_wait(sem_mutex);
      if((g_callout_head != NULL) && 
         (g_callout_head->expire <= 0)) {
        struct callout *p = g_callout_head;
        g_callout_head = p->next;
        if(g_callout_head != NULL) {
          g_callout_head->expire += p->expire;
        }
//        printk("%d: expire %d\n\r", task_getid(), p->expire);
        sem_signal(sem_mutex);

        (p->fun)(p->pv);

        kfree(p);
      } else {
        sem_signal(sem_mutex);
        break;
      }
    }
	}
	
	task_exit(0);
}

void *set_callout(int32_t expire, void (*fun)(void *), void *pv)
{
  struct callout *new;
  struct callout *p, *q;

	if(expire <= 0 || fun == NULL)
		return NULL;
		
	new = (struct callout *)kmalloc(sizeof(struct callout));
	if(new == NULL)
		return NULL;
	
  new->expire = expire;
	new->fun = fun;
	new->pv = pv;
  new->next = NULL;
	
	sem_wait(sem_mutex);

    if(g_callout_head == NULL) {
        g_callout_head = new;
        sem_signal(sem_mutex);
        return new;
    }
	
    if(new->expire < g_callout_head->expire) {
        g_callout_head->expire -= new->expire;
        new->next = g_callout_head;
        g_callout_head = new;
        sem_signal(sem_mutex);
        return new;
    }

    new->expire -= g_callout_head->expire;

    q = g_callout_head;
    do {
        p = q->next;
        if((p != NULL) && (new->expire >= p->expire)) {
            new->expire -= p->expire;
        } else {
            q->next = new;
            new->next = p;
            if(p != NULL)
                p->expire -= new->expire;
            break;
        }
        q = p;
    } while(1);

	sem_signal(sem_mutex);
	return new;
}

int kill_callout(void *co)
{
	struct callout *p, *q;

    if(co == NULL)
        return -1;
	
	sem_wait(sem_mutex);

    if(g_callout_head == NULL) {
        sem_signal(sem_mutex);
        return -1;
    }

    if(co == g_callout_head) {
        p = g_callout_head;
        g_callout_head = p->next;

        if(g_callout_head != NULL)
            g_callout_head->expire += p->expire;

        kfree(p);
        sem_signal(sem_mutex);
        return 0;
    }
    
    q = g_callout_head;
    do {
        p = q->next;
        if(p == NULL)
            break;
        if(p == co) {
            if(p->next != NULL)
                p->next->expire += p->expire;
            q->next = p->next;
            kfree(p);
            break;
        }
        q = p;
    } while(1);
	
	sem_signal(sem_mutex);
	return (p==co)?0:-1;
}

void init_callout()
{
  if(sys_task_create(0, task_callout, (void*)NULL) >= 0) {
    g_sem_callout = sem_create(0);
    sem_mutex = sem_create(1);
  } else {
    printk("init_callout: failed to create a task\n\r");
  }
}

