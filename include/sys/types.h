#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef unsigned int size_t;
#endif

#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef int ssize_t;
#endif

#ifndef _TIME_T_DEFINED
#define _TIME_T_DEFINED
typedef long time_t;
#endif

#ifndef _CLOCK_T_DEFINED
#define _CLOCK_T_DEFINED
typedef long clock_t;
#endif

#ifndef _INO_T_DEFINED
#define _INO_T_DEFINED
typedef unsigned int ino_t;
#endif

#ifndef _DEV_T_DEFINED
#define _DEV_T_DEFINED
typedef unsigned int dev_t;
#endif

#ifndef _MODE_T_DEFINED
#define _MODE_T_DEFINED
typedef unsigned short mode_t;
#endif

#ifndef _NLINK_T_DEFINED
#define _NLINK_T_DEFINED
typedef unsigned short nlink_t;
#endif

#ifndef _UID_T_DEFINED
#define _UID_T_DEFINED
typedef unsigned short uid_t;
#endif

#ifndef _GID_T_DEFINED
#define _GID_T_DEFINED
typedef unsigned short gid_t;
#endif

#ifndef _LOFF_T_DEFINED
#define _LOFF_T_DEFINED
typedef long loff_t;
#endif

#ifndef _OFF64_T_DEFINED
#define _OFF64_T_DEFINED
typedef long long off64_t;
#endif

#ifndef _OFF_T_DEFINED
#define _OFF_T_DEFINED
#ifdef LARGEFILES
typedef off64_t off_t;
#else
typedef loff_t off_t;
#endif
#endif

#ifndef _PTRDIFF_T_DEFINED
#define _PTRDIFF_T_DEFINED
typedef int ptrdiff_t;
#endif

#endif /*_SYS_TYPES_H*/
