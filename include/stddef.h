#ifndef _STDDEF_H
#define _STDDEF_H

#define NULL ((void *) 0)
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *) 0)->MEMBER)

#endif /* _STDDEF_H */
