#ifndef	_SYS_TIME_H_
#define	_SYS_TIME_H_
#include <sys/types.h>

struct timeval {
	time_t	tv_sec;		/* seconds */
	long	tv_usec;	/* microseconds */
};

int gettimeofday(struct timeval *tv, void *tzp);

#endif /* _SYS_TIME_H_ */
