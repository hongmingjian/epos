#ifndef _FCNTL_H
#define _FCNTL_H

int  open(const char *path, int mode);
#define O_RDONLY	0x0
#define O_WRONLY	0x1
#define O_RDWR      0x2
#define O_APPEND	0x2000

#endif /* _FCNTL_H*/