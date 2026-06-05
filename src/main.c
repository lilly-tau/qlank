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
	struct function *func;
	size_t i, j;

	create_lexer(&lexer);

	context.lexer = &lexer;
	create_functions(&context.functions);


	while (next_token(&lexer)) {
		compile_function(&context);
	}

	printf("(module\n");
	for (i = 0; i < context.functions.length; i++) {
		func = context.functions.contents + i;
		if (strlen(func->body)) {
			printf("\t(func $%s (export \"%s\") ", func->name + 1,
				func->name + 1);
			for (j = 0; j < func->pcount; j++) {
				printf("(param %s ", func->pnames[j]);
				switch (func->ptypes[j]) {
				case T_CHAR:
				case T_BYTE:
				case T_INT:
				case T_WORD:
					printf("i32) ");
				}
			}

			switch (func->return_type) {
			case T_CHAR:
			case T_BYTE:
			case T_INT:
			case T_WORD:
				printf("(result i32) ");
			}

			for (j = 0; j < func->vcount; j++) {
				printf("(local %s ", func->vnames[j]);
				switch (func->vtypes[j]) {
				case T_CHAR:
				case T_BYTE:
				case T_INT:
				case T_WORD:
					printf("i32) ");
				}
			}
			printf("\n%s\t)\n", func->body);
		} else {
			printf("\t(import \"js\" \"%s\" (func $%s ",
				func->name + 1, func->name + 1);
			for (j = 0; j < func->pcount; j++) {
				printf("(param %s ", func->pnames[j]);
				switch (func->ptypes[j]) {
				case T_CHAR:
				case T_BYTE:
				case T_INT:
				case T_WORD:
					printf("i32) ");
					break;
				}
			}
			switch (func->return_type) {
			case T_CHAR:
			case T_BYTE:
			case T_INT:
			case T_WORD:
				printf("(result i32)");
				break;
			}
			printf("))\n");
		}
	}
	printf(")\n");

	destroy_functions(&context.functions);
	destroy_lexer(&lexer);
	exit(0);
}
