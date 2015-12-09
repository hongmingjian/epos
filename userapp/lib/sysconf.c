#include <unistd.h>

long sysconf(int sc)
{
    switch(sc) {
    case _SC_PAGESIZE:
        return 4096;
    default:
        return -1;
    }
}
