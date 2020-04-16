#ifndef _UNISTD_H
#define _UNISTD_H

#include <sys/types.h>

unsigned int sleep(unsigned int seconds);

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2


int close(int fd);
ssize_t read(int fd, void *buf, size_t len);
ssize_t write(int fd, const void *buf, size_t len);

off_t lseek(int fd, off_t offset, int whence);
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#endif /* _UNISTD_H*/
