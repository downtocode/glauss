#include <stdio.h>
#include <stdarg.h>
#include "msg_phys.h"
#include "options.h"

/* pprintf - a priority printing function. */

void pprintf(unsigned int priority, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	if(priority <= option->verbosity) {
		vprintf (format, args);
	}
	va_end(args);
}
