#ifndef DEBUG_H
#define DEBUG_H

#include <stdarg.h>

#include "syscall.h"

#ifdef DEBUG
#define BUG_ON(x) \
	do { \
		if (x) { \
			sc_printf("BUG_ON @%s:%d", __FILE__, __LINE__); \
			sc_halt(); \
		} \
	} while(0)
#else
#define BUG_ON(x) 
#endif

void sc_printf(const char *fmt, ...);
void sc_vprintf(const char *fmt, va_list ap);

#endif
