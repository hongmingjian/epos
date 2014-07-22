#include "../global.h"

///////////////////HELPERS///////////////////////
void srand(uint32_t x);
uint32_t random();

#include "../tlsf/tlsf.h"
extern char end[];
void *malloc(size_t bytes)
{ return malloc_ex(bytes, end); }
void *realloc(void *oldptr, size_t bytes)
{ return realloc_ex(oldptr, bytes, end); }
void free(void *ptr)
{ free_ex(ptr, end); }

#include <stdarg.h>
int sprintf(char *buf, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list args);
int printf(const char *fmt,...)
{
	char buf[1024];
	va_list args;
	int i, j;

	va_start(args, fmt);
	i=vsprintf(buf,fmt,args);
	va_end(args);

	for(j = 0; j < i; j++)
		putchar(buf[j]);

	return i;
}
#define DELAY(n) do { \
  unsigned __n=(n); \
  while(__n--); \
} while(0);
//////////////////////////////////////////////////


/////////////////SYSTEM CALLS/////////////////////
int task_exit(int val);
int task_create(unsigned stack, void *func, unsigned pv);
int task_getid();
void task_yield();
int task_wait(int tid, int *exit_code);
void beep(unsigned freq);
int putchar(int c);

struct vm86_context {
  uint32_t  : 32;       
  uint32_t  : 32;      
  uint32_t  : 32; 

  uint16_t  di; uint16_t  : 16;
  uint16_t  si; uint16_t  : 16;
  uint16_t  bp; uint16_t  : 16;
  uint32_t  : 32; 
  uint16_t  bx; uint16_t  : 16;
  uint16_t  dx; uint16_t  : 16;
  uint16_t  cx; uint16_t  : 16;
  uint16_t  ax; uint16_t  : 16;

  uint32_t  : 32; 
  uint32_t  : 32; 

  uint16_t  ip; uint16_t  : 16;
  uint16_t  cs; uint16_t  : 16;
  uint32_t  eflags; 
  uint16_t  sp; uint16_t  : 16;
  uint16_t  ss; uint16_t  : 16;
  uint16_t  es; uint16_t  : 16;
  uint16_t  ds; uint16_t  : 16;
  uint16_t  fs; uint16_t  : 16;
  uint16_t  gs; uint16_t  : 16;
};
int vm86(struct vm86_context *ctx);
//////////////////////////////////////////////////

#define LINEAR(seg,off) ((uint32_t)(((uint16_t)(seg)<<4)+(uint16_t)(off)))
#define EFLAGS_VM   0x20000
#define EFLAGS_IF   0x00200
int int86(int n, struct vm86_context *ctx)
{
    int fStop = 0;
    uint16_t *ivt = (uint16_t *)0;

    ctx->ip = ivt[n*2+0];
    ctx->cs = ivt[n*2+1];

    do {
    vm86(ctx);

    switch(*(uint8_t *)LINEAR(ctx->cs, ctx->ip)) {
    case 0xf4: /*HLT*/
      fStop = 1;
      ctx->ip++;

      printf("opcode=HLT\r\n");
      break;
    case 0xfa: /*CLI*/
      ctx->eflags &= ~EFLAGS_IF;
      ctx->ip++;

//      printf("opcode=CLI\r\n");
      break;
    case 0xfb: /*STI*/
      ctx->eflags |= EFLAGS_IF;
      ctx->ip++;

//      printf("opcode=STI\r\n");
      break;
    case 0x9c: /*PUSHF*/
      ctx->sp -= 2;
      *(uint16_t *)LINEAR(ctx->ss, ctx->sp) = (uint16_t)ctx->eflags;
      ctx->ip++;

//      printf("opcode=PUSHF\r\n");
      break;
    case 0x9d: /*POPF*/
      ctx->eflags = EFLAGS_VM | (*(uint16_t *)LINEAR(ctx->ss, ctx->sp));
      ctx->sp += 2;
      ctx->ip++;

//      printf("opcode=POPF\r\n");
      break;
    case 0xcd: /*INT*/
      ctx->sp -= 2;
      *(uint16_t *)LINEAR(ctx->ss, ctx->sp) = (uint16_t)ctx->eflags;

      ctx->sp -= 2;
      *(uint16_t *)LINEAR(ctx->ss, ctx->sp) = ctx->cs;

      ctx->sp -= 2;
      *(uint16_t *)LINEAR(ctx->ss, ctx->sp) = ctx->ip + 2;

      ctx->ip = ivt[(*(uint8_t *)LINEAR(ctx->cs, ctx->ip+1)) * 2];
      ctx->cs = ivt[(*(uint8_t *)LINEAR(ctx->cs, ctx->ip+1)) * 2 + 1];

//      printf("opcode=INT\r\n");
      break;
    case 0xcf: /*IRET*/
      ctx->ip = *(uint16_t *)LINEAR(ctx->ss, ctx->sp);
      ctx->sp += 2;

      ctx->cs = *(uint16_t *)LINEAR(ctx->ss, ctx->sp);
      ctx->sp += 2;

      ctx->eflags = EFLAGS_VM | (*(uint16_t *)LINEAR(ctx->ss, ctx->sp));
      ctx->sp += 2;
 
//      printf("opcode=IRET\r\n");
      break;
    default:
      fStop = 1;

      printf("Unknown opcode=0x%02x\r\n", *(uint8_t *)LINEAR(ctx->cs, ctx->ip));
      break;
    } 
    } while(!fStop);

//    printf("AX=0x%04x\r\n", ctx->ax);
    return 0;
}

static void hanoi(int d, char from, char to, char aux)
{
  unsigned n;

  if(d == 1) {
    DELAY(1000000);
    printf("%c%d%c ", from, d, to);
    return;
  }

  hanoi(d - 1, from, aux, to);
  DELAY(1000000);
  printf("%c%d%c ", from, d, to);
  hanoi(d - 1, aux, to, from);
}

static tsk_hanoi(void *pv)
{
  int n = (int)pv;
  printf("task #%d: Playing Hanoi tower with %d plates\r\n",
         task_getid(), pv);
  if(n <= 0) {
    printf("task #%d: Illegal number of plates %d\r\n", n);
    task_exit(-1);
  }

  hanoi(n, 'A', 'B', 'C');

  printf("task #%d: Exiting\r\n", task_getid());
  task_exit(n);
}

static unsigned fib(unsigned n)
{
  if (n == 0)
    return 0;
  if (n == 1)
    return 1;
  return fib(n - 1) + fib(n - 2);
}

static void tsk_fib(void *pv)
{
  int i, code;

  printf("task #%d: waiting task #%d to exit\r\n", task_getid(), (int)pv);
  task_wait((int)pv, &code);
  printf("task #%d: task #%d exited with code %d\r\n",
         task_getid(), (int)pv, code);

  for(i = 37; i < 48; i++)
    printf("task #%d: fib(%d)=%u\r\n", task_getid(), i, fib(i));

  printf("task #%d: Exiting\r\n", task_getid());

  task_exit(0);
}

void main(void *pv)
{
  printf("task #%d: Hello world! I'm the first user task(pv=0x%08x)!\r\n",
         task_getid(), pv);
  {
    struct vm86_context v86c;
    memset(&v86c, 0, sizeof(v86c));
    v86c.ss = 0x1000;
    v86c.sp = 0;

//  call BIOS
//   int86(0x12, &v86c);
//   printf("ss:sp=0x%04x:0x%04x\r\n", v86c.ss, v86c.sp);

//   call VBE
   v86c.ax=0x4f02;/*set VBE mode*/
   v86c.bx=0x0118;/*1024x768x24*/
   int86(0x10, &v86c);
  }

  if(0){
    int code;
    int tid_hanoi, tid_fib;
    char *stack_hanoi, *stack_fib;

    stack_hanoi = malloc(1024*1024);
    tid_hanoi = task_create(stack_hanoi+1024*1024, tsk_hanoi, (void *)6);
    printf("task #%d: task #%d created(stack=0x%08x, size=%d)\r\n",
           task_getid(), tid_hanoi, stack_hanoi, 1024*1024);

    stack_fib = malloc(1024*1024);
    tid_fib = task_create(stack_fib+1024*1024, tsk_fib, (void *)tid_hanoi);
    printf("task #%d: task #%d created(stack=0x%08x, size=%d)\r\n",
           task_getid(), tid_fib,   stack_fib,   1024*1024);

    printf("task #%d: waiting task #%d to exit\r\n", task_getid(), tid_hanoi);
    task_wait(tid_hanoi, &code);
    free(stack_hanoi);
    printf("task #%d: task #%d exited with code %d\r\n",
           task_getid(), tid_hanoi, code);
  }

  while(1)
    ;
  task_exit(0);
}

/**
 * GCC insists on __main
 *    http://gcc.gnu.org/onlinedocs/gccint/Collect2.html
 */
void __main()
{
  init_memory_pool(64*1024*1024, end);
}

