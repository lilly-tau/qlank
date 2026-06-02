#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PASSERT_IMPL
#include "passert.h"
#include "compile.h"
#include "lexer.h"
#define TYPE_IMPL
#include "types.h"

int
main(void)
{
	struct lexer lexer;
	struct context context = {0};
	size_t i;

	create_lexer(&lexer);

	context.lexer = &lexer;
	create_functions(&context.functions);


	while (next_token(&lexer)) {
		compile_function(&context);
	}

	printf("(module\n");
	for (i = 0; i < context.functions.length; i++) {
		printf("\t(func %s (export \"%s\") ",
			context.functions.contents[i].name,
			context.functions.contents[i].name);
		switch (context.functions.contents[i].return_type) {
		case T_CHAR:
		case T_BYTE:
		case T_INT:
		case T_WORD:
			printf("(result i32)\n");
		}
		printf("%s\t)\n", context.functions.contents[i].body);
	}
	printf(")\n");

	destroy_functions(&context.functions);
	destroy_lexer(&lexer);
	exit(0);
}
