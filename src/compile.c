#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compile.h"
#include "lexer.h"
#include "passert.h"
#include "strbuilder.h"
#include "types.h"

void
create_functions(struct functions *ret)
{
	ret->contents = malloc(sizeof(*ret->contents) * 0x10);
	ret->capacity = 0x10;
	ret->length = 0x00;

	ret->strings = malloc(0x200);
	ret->strings_capacity = 0x200;
	ret->strings_length = 0x00;
}

void
destroy_functions(struct functions *ret)
{
	free(ret->strings);
	free(ret->contents);
}

void
add_function(struct functions *ret, const char *name, const char *body,
TYPE return_type)
{
	size_t length;
	char *s;

	ret->length += 1;
	if (ret->length > ret->capacity) {
		ret->capacity += 0x10;
		ret->contents = realloc(ret->contents,
			sizeof(*ret->contents) * ret->capacity);
	}

	length = strlen(name) + strlen(body) + 2;
	ret->strings_length += length;
	if (ret->strings_capacity < ret->strings_length) {
		ret->strings_capacity =
			(1 + ret->strings_length / 0x200) * 0x200;
		ret->strings = realloc(ret->strings, ret->strings_capacity);
	}

	ret->contents[ret->length - 1].name = ret->strings
		+ ret->strings_length - length;
	ret->contents[ret->length - 1].body = ret->strings
		+ ret->strings_length - length + strlen(name) + 1;

	strcpy((char *)ret->contents[ret->length - 1].name, name);
	strcpy((char *)ret->contents[ret->length - 1].body, body);
	ret->contents[ret->length - 1].return_type = return_type;

	for (s = (char *)ret->contents[ret->length - 1].name; *s; s++) {
		*s = tolower(*s);
	}

}

BOOLEAN
compile_expression(struct strbuilder *ret, TYPE return_type,
struct context *ctx)
{
	NUMCONST constant;
	BOOLEAN isword, negative;
	TYPE expr_type;
	char *tmp;
	size_t i;

	if (is_number(ctx->lexer->token)) {
		constant = as_number(ctx->lexer->token, &isword, &negative);
	
		if (constant < 0x100 && isword)
			expr_type = T_BYTE;
		else if (constant < 0x100 && negative)
			expr_type = T_CHAR;
		else if (constant < 0x80000000 && isword)
			expr_type = T_WORD;
		else if (constant < 0x80000000)
			expr_type = T_INT;
		else if (negative)
			expr_type = T_INT;
		else
			expr_type = T_WORD;
	
		strext(ret, "\t\t(i32.const 0x");
		strext_num(ret, constant);
		strext(ret, ")\n");
	
		p_assert(type_associable(expr_type, return_type),
			"Expected type %s but got type %s in expression on"
			" line %u.", TYPE_STRINGS[return_type],
			TYPE_STRINGS[expr_type], ctx->lexer->line);
	} else if (is_identifier(ctx->lexer->token)) {
		tmp = malloc(strlen(ctx->lexer->token) + 1);

		for (i = 0; ctx->lexer->token[i]; i++)
			tmp[i] = tolower(ctx->lexer->token[i]);
		tmp[i] = '\0';

		for (i = 0; i < ctx->functions.length; i++)
			if (!strcmp(tmp, ctx->functions.contents[i].name))
				break;

		p_assert(i != ctx->functions.length, "Unknown function %s in"
			" expression on line %u.", ctx->lexer->token,
			ctx->lexer->line);

		expr_type = ctx->functions.contents[i].return_type;
		strext(ret, "\t\t(call $");
		strext(ret, tmp + 1);
		strext(ret, ")\n");

		free(tmp);
	} else
		p_assert(FALSE, "Invalid expression on line %u.",
			ctx->lexer->line);

	return TRUE;
}

BOOLEAN 
compile_statement(struct strbuilder *ret, TYPE return_type,
struct context *ctx)
{
	if (is_keyword(ctx->lexer->token, "RET")) {
		next_token(ctx->lexer);
		compile_expression(ret, return_type, ctx);
		strext(ret, "\t\t(return)\n");
	} else
		p_assert(FALSE, "Expected statement, got %s on line %u.",
			ctx->lexer->token, ctx->lexer->line);

	return TRUE;
}

BOOLEAN
compile_function(struct context *ctx)
{
	char *name;
	struct strbuilder body;
	BOOLEAN found_type;
	TYPE return_type;

	if (!is_identifier(ctx->lexer->token))
		return FALSE;

	name = malloc(strlen(ctx->lexer->token) + 1);
	strcpy(name, ctx->lexer->token);

	create_strbuilder(&body);

	found_type = FALSE;
	while (next_token(ctx->lexer)
	&& !is_keyword(ctx->lexer->token, "DONE")) {
		if (is_keyword(ctx->lexer->token, "OUTPUTS")) {
			p_assert(!found_type, "Function %s has multiple"
				" types on line %u.\n", name, ctx->lexer->line);
			return_type = expect_type(FALSE, ctx->lexer);
			found_type = TRUE;
		} else if (is_keyword(ctx->lexer->token, "DO")) {
			ctx->in_function = TRUE;
			while (next_token(ctx->lexer)
			&& !is_keyword(ctx->lexer->token, "DONE")) {
				p_assert(compile_statement(&body, return_type,
					ctx), "Expected statement on"
					" line %u.\n", ctx->lexer->line);
			}
			ctx->in_function = FALSE;
			break;
		} else {
			p_assert(FALSE, "Invalid token %s in function header"
				" on line %u.\n", ctx->lexer->token,
				ctx->lexer->line);
		}
	}

	add_function(&ctx->functions, name, body.data, return_type);

	free(name);
	destroy_strbuilder(&body);
}
