#ifndef	_SYS_TIME_H_
#define	_SYS_TIME_H_

struct timeval {
	long tv_sec;
	long tv_usec;
};

struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};

int gettimeofday(struct timeval *tv, struct timezone *tz);

#endif /* _SYS_TIME_H_ */
