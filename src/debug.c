/*
 * debug - various debugging stuff (using syscalls)
 */

#include <stdio.h>

#include <debug.h>

void sc_vprintf(const char *fmt, va_list ap)
{
	static char buf[128];

	vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
	buf[sizeof(buf) - 1] = '\0';
	sc_puts(buf);
}

void sc_printf(const char *fmt, ...)
{
	va_list ap;
	
	 va_start(ap, fmt);
	 sc_vprintf(fmt, ap);
	 va_end(ap);
}

