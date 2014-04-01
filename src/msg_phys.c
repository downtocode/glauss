#include <stdio.h>
#include <stdarg.h>
#include "msg_phys.h"
#include "options.h"

/* pprintf - a priority printing function. */

void pprintf(unsigned int priority, const char *format, ...)
{
	va_list args;
	if(priority <= option->verbosity) {
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
		if(option->logenable) {
			va_start(args, format);
			vfprintf(option->logfile, format, args);
			va_end(args);
		}
	}
}
