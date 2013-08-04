#include "kernel.h"
#include "utils.h"

unsigned volatile ticks = 0;

void isr_timer(uint32_t irq, struct context ctx)
{
	ticks++;
//  putchar('.');

    if((g_task_running == NULL) || 
       (g_task_running->tid == 0)) {
        g_resched = 1;
    }
    else {
        --g_task_running->quantum;
	
        if(g_task_running->quantum <= 0) {
            g_resched = 1;
            g_task_running->quantum = DEFAULT_QUANTUM;
        }
    }

    if(g_callout_head != NULL) { 
        if(g_callout_head->expire <= 0)
            g_callout_head->expire--;
        else {
            g_callout_head->expire--;
            if(g_callout_head->expire <= 0)
                sem_signal(g_sem_callout);
        }
    }
}
