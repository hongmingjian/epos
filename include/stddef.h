#ifndef _STDDEF_H
#define _STDDEF_H

#ifndef NULL
#define NULL ((void *) 0)
#endif
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *) 0)->MEMBER)

#endif /* _STDDEF_H */
