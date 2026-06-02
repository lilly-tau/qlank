#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "strbuilder.h"

void
create_strbuilder(struct strbuilder *ret)
{
	ret->data = malloc(0x200);
	*ret->data = 0x00;
	ret->capacity = 0x200;
	ret->length = 0x00;
}

void
destroy_strbuilder(struct strbuilder *ret)
{
	free(ret->data);
}

void
strext(struct strbuilder *ret, char *string)
{
	ret->length += strlen(string);
	if (ret->length > ret->capacity) {
		ret->capacity = (1 + ret->length / 0x200) * 0x200;
		ret->data = realloc(ret->data, ret->capacity);
	}

	strcat(ret->data, string);
}

void
strext_num(struct strbuilder *ret, unsigned long number)
{
	char string[9] = {0};
	sprintf(string, "%08lX", number);

	strext(ret, string);
}
