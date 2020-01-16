#ifndef	_SYS_UMUTEX_H_
#define	_SYS_UMUTEX_H_

#ifndef _KERNEL
#include <sys/cdefs.h>

__BEGIN_DECLS
void * umutex(void **, int);
__END_DECLS
#endif /* ! _KERNEL */

#define	UMUTEX_LOCK		1
#define	UMUTEX_UNLOCK	2
#define UMUTEX_INIT		3

#endif
