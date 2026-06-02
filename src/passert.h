#ifndef X__PASSERT_H__X
#define X__PASSERT_H__X
#include "types.h"

void
p_assert(BOOLEAN condition, const char *fmt, ...);

#endif

#ifdef PASSERT_IMPL
#undef PASSERT_IMPL
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "types.h"

void
p_assert(BOOLEAN condition, const char *fmt, ...)
{
	va_list args;

	if (!condition) {
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);
		exit(1);
	}
}

#endif
