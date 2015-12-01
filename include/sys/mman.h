#ifndef	_SYS_MMAN_H_
#define	_SYS_MMAN_H_

#include <sys/types.h>

/*
 * Protections
 */
#define	PROT_NONE	0x00	/* [MC2] no permissions */
#define	PROT_READ	0x01	/* [MC2] pages can be read */
#define	PROT_WRITE	0x02	/* [MC2] pages can be written */
#define	PROT_EXEC	0x04	/* [MC2] pages can be executed */

/*
 * Flags
 */
#define	MAP_SHARED	0x0001		/* [MF|SHM] share changes */
#define	MAP_PRIVATE	0x0002		/* [MF|SHM] changes are private */

#define	MAP_FIXED	0x0010		/* interpret addr exactly */
#define	MAP_FILE	0x0000		/* map from file (default) */
#define	MAP_ANON	0x1000		/* allocated from memory, swap space */

/*
 * Error return from mmap()
 */
#define MAP_FAILED	((void *)-1)	/* [MF|SHM] mmap failed */

#endif /* _SYS_MMAN_H_ */