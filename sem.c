/**
 * vim: filetype=c:fenc=utf-8:ts=2:et:sw=2:sts=2
 */
#include "kernel.h"
#include "utils.h"

void *sys_sem_create(int value)
{
  return NULL;
}

int sys_sem_destroy(void *hsem)
{
  return -1;
}

int sys_sem_wait(void *hsem)
{
  return -1;
}

int sys_sem_signal(void *hsem)
{
  return -1;
}

