#ifndef _UNISTD_H
#define _UNISTD_H

#define _SC_PAGESIZE            0x0027
#define _SC_PAGE_SIZE           _SC_PAGESIZE

long         sysconf(int);
#endif /* _UNISTD_H*/
