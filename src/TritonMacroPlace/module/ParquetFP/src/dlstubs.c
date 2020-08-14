#include <sys/types.h>
#include <dlfcn.h>

	/* dl*() stub routines for static compilation.  Prepared from
	   /usr/include/dlfcn.h by Hal Pomeranz <<EMAIL: PROTECTED>> */
void *_dlopen(const char *str, int x)    { return (void*)0; }
void *_dlsym(void *ptr, const char *str) { return (void*)0; }
int _dlclose(void *ptr) { return 0; }
