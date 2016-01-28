#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <sys/types.h>
#include <inttypes.h>
#include <time.h>
#include <ioctl.h>

int task_exit(int code_exit);
int task_create(void *tos, void (*func)(void *pv), void *pv);
int task_getid();
void task_yield();
int task_wait(int tid, int *pcode_exit);
int reboot(int howto);
void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
int   munmap(void *addr, size_t len);
unsigned sleep(unsigned seconds);
int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);

void beep(int freq);
int putchar(int c);
int getchar();

struct vm86_context {
  uint32_t  : 32;/*0*/
  uint32_t  : 32;/*4*/
  uint32_t  : 32;/*8*/
  uint32_t  edi; /*12*/
  uint32_t  esi; /*16*/
  uint32_t  ebp; /*20*/
  uint32_t  : 32;/*24*/
  uint32_t  ebx; /*28*/
  uint32_t  edx; /*32*/
  uint32_t  ecx; /*36*/
  uint32_t  eax; /*40*/
  uint32_t  : 32;/*44*/
  uint32_t  : 32;/*48*/
  /* below defined in x86 hardware */
  uint32_t  eip; /*52*/
  uint16_t  cs; uint16_t  : 16;/*56*/
  uint32_t  eflags;/*60*/
  uint32_t  esp; /*64*/
  uint16_t  ss; uint16_t  : 16;/*68*/
  uint16_t  es; uint16_t  : 16;/*72*/
  uint16_t  ds; uint16_t  : 16;/*76*/
  uint16_t  fs; uint16_t  : 16;/*80*/
  uint16_t  gs; uint16_t  : 16;/*84*/
} __attribute__ ((gcc_struct, packed));
int vm86(struct vm86_context *vm86ctx);

int     ioctl(int fd, uint32_t req, void *pv);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);

#endif /*_SYSCALL_H*/
