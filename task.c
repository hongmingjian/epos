#include "utils.h"
#include "kernel.h"

#define _TESTING

int g_resched;
struct tcb *g_task_running;
struct tcb *g_task_all_head;

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

//    printk("%d -> %d\n\r", (g_task_running == NULL) ? -1 : g_task_running->tid, select->tid);

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

int task_create(void *stack_base, 
	  void (*handler)(void *), void *param)
{
	static int tid = 1;
	struct tcb *new;
  uint32_t flags;

  if(stack_base == NULL)
    return -1;

  if(((uint32_t)stack_base) & 3)
    return -1;
    
	if((new = (struct tcb *)kmalloc(sizeof(struct tcb))) == NULL) {
		return -2;
  }

	memset(new, 0, sizeof(struct tcb));

	new->tid = tid++;
	new->state = TASK_STATE_READY;
	new->stack_base = stack_base;
	new->quantum = DEFAULT_QUANTUM;
  new->wait_cnt = 0;
  new->wait_head = NULL;
  new->wait_next = NULL;
  new->sem_next = NULL;
  new->all_next = NULL;

  INIT_CONTEXT(new->context, new->stack_base, handler);

  PUSH_CONTEXT_STACK(new->context, param);
  PUSH_CONTEXT_STACK(new->context, 0);

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

    g_task_running = NULL;

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
	    remove_task(tsk);
//        printk("%d: Task %d reaped\n\r", g_task_running->tid, tsk->tid);
        restore_flags(flags);
	    kfree(tsk);
        return 0;
	}
		
    restore_flags(flags);
	return 0;
}

int32_t task_getid()
{
    return (g_task_running==NULL)?-1:g_task_running->tid;
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

void task_sleep(int32_t msec)
{
    if(NULL != set_callout(1+(HZ*msec)/1000, task_sleep_callout, g_task_running)) {
        uint32_t flags;
//        printk("%d: sleep %d ticks, ticks=%d\n\r", task_getid(), 1+(HZ*msec)/1000, ticks);
        save_flags_cli(flags);
        g_task_running->state = TASK_STATE_BLOCKED;
        schedule();
        restore_flags(flags);
    }
}

#ifdef _TESTING
static unsigned fib(unsigned n)
{
     if (n == 0)
        return 0;
     if (n == 1)
        return 1;
     return fib(n - 1) + fib(n - 2);
}

static void xxx(void *pv)
{
    printk("%d: xxx 0x%08x\n\r", task_getid(), pv);
}

static void foo(void *pv)
{
    int i;
//    printk("%d: 0x%08x\n\r", task_getid(), pv);

//    set_callout(1, xxx, (void *)0x19770802);
//    set_callout(1, xxx, (void *)0x19760206);
//    set_callout(3, xxx, (void *)0x20060907);

//    sem_wait(pv);
    for(i = 0; i < 5; i++) {
        printk("%d: %u\n\r", task_getid(), fib(30));
    }
    printk("%d: Done\n\r", task_getid());
//    sem_signal(pv);

    task_exit((int)pv);
}

static void bar(void *pv)
{
    int32_t code;
    int i, tid=4;

    printk("%d: TID = %d\n\r", task_getid(), task_create(((char *)kmalloc(4096))+4096, foo , pv));
    printk("%d: Waiting task #%d\n\r", task_getid(), tid);
    task_wait(tid, &code);
    printk("%d: Task #%d finished(0x%08x)!\n\r", task_getid(), tid, code);
//    sem_wait(pv);
    for(i = 0; i < 5; i++) {
        printk("%d: %u\n\r", task_getid(), fib(30));
    }
    printk("%d: Done\n\r", task_getid());
//    sem_signal(pv);

    task_exit((int)pv);
}

static void foobar(void *pv)
{
    int32_t code;
    int i, tid = 3;

    printk("%d: TID = %d\n\r", task_getid(), task_create(((char *)kmalloc(4096))+4096, bar , pv));

//    sem_wait(pv);
    printk("%d: Waiting task #%d\n\r", task_getid(), tid);
    task_wait(tid, &code);
    printk("%d: Task #%d finished(0x%08x)!\n\r", task_getid(), tid, code);
    for(i = 0; i < 2; i++) {
        printk("%d: %u\n\r", task_getid(), fib(30));
    }
//    tid=2;
//    printk("%d: Waiting task #%d\n\r", task_getid(), tid);
//    task_wait(tid, &code);
//    printk("%d: Task #%d finished(0x%08x)!\n\r", task_getid(), tid, code);
//    task_sleep(500000);
    for(i = 0; i < 3; i++) {
        printk("%d: %u\n\r", task_getid(), fib(30));
    }
    printk("%d: Done\n\r", task_getid());
//    sem_signal(pv);

//   printk("%d: %d\n\r", task_getid(), task_wait(1, &code));

    task_exit((int)pv);
}
#endif

static void task0run(void *pv)
{
#ifdef _TESTING
    void *sem = sem_create(1);
    printk("%d: TID = %d\n\r", task_getid(), task_create(((char *)kmalloc(4096))+4096, foobar , (void *)sem));
#endif

    while(1)
        ;
}

static struct tcb task0;
extern char tmpstk;

void init_task(void)
{
  g_resched = 1;
	g_task_running = NULL;
  g_task_all_head = NULL;
  add_task(&task0);

  task0.tid = 0;
  task0.state = TASK_STATE_READY;
  task0.quantum = DEFAULT_QUANTUM;
  task0.stack_base = &tmpstk; /*XXX*/
  task0.exit_code = 0;
  task0.wait_cnt = 0;
  task0.wait_head = NULL;
  task0.wait_next = NULL;
  task0.sem_next = NULL;
  task0.all_next = NULL;

  INIT_CONTEXT(task0.context, task0.stack_base, task0run);

  PUSH_CONTEXT_STACK(task0.context, 0);
  PUSH_CONTEXT_STACK(task0.context, 0);
}

