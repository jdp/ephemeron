#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "util.h"

void
error(FILE *fp, int fatal, const char *format, ...)
{
	va_list args;
	
	va_start(args, format);
	vfprintf(fp, format, args);
	va_end(args);
	if (fatal > 0) {
		exit(fatal);
	}
}