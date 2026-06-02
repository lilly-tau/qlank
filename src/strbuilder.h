#ifndef X__STRBUILD_H__X
#define X__STRBUILD_H__X
#include <stddef.h>

struct strbuilder {
	char *data;
	size_t length, capacity;
};

void
create_strbuilder(struct strbuilder *ret);

void
destroy_strbuilder(struct strbuilder *ret);

void
strext(struct strbuilder *ret, char *string);

void
strext_num(struct strbuilder *ret, unsigned long number);

#endif
