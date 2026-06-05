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

const char *
convert_type(TYPE type)
{
	switch (type) {
	case T_CHAR:
	case T_INT:
	case T_BYTE:
	case T_WORD:
	case T_PTR:
	case T_OFFSET:
		return "i32";
	default:
		return "???";
	}
}

void
print_function(struct function *func)
{
	BOOLEAN export;
	size_t i;

	export = strlen(func->body);

	if (export)
		printf("\t(func $%s (export \"%s\")", func->name + 1,
			func->name + 1);
	else
		printf("\t(import \"js\" \"%s\" (func $%s", func->name + 1);

	for (i = 0; i < func->pcount; i++)
		printf(" (param %s %s)", func->pnames[i],
			convert_type(func->ptypes[i]));

	printf(" (result %s)", convert_type(func->return_type));

	for (i = 0; i < func->vcount; i++)
		printf(" (local %s %s)", func->vnames[i],
			convert_type(func->vtypes[i]));

	if (export)
		printf("\n%s\t)", func->body);
	else
		printf("))");
	printf("\n");
}

int
main(void)
{
	struct lexer lexer;
	struct context context = {0};
	struct function *func;
	size_t i, j;

	create_lexer(&lexer);

	context.lexer = &lexer;
	create_functions(&context.functions);


	while (next_token(&lexer)) {
		if(!compile_function(&context)) {
			p_assert(compile_constant(&context),
				"Expected function or constant definition"
				" on line %u.\n", lexer.line);
		}
	}

	printf("(module\n\t(memory 1)\n");
	for (i = 0; i < context.functions.length; i++)
		print_function(context.functions.contents + i);
	printf(")\n");

	destroy_functions(&context.functions);
	destroy_lexer(&lexer);
	exit(0);
}
