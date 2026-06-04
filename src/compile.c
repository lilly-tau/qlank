#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compile.h"
#include "lexer.h"
#include "passert.h"
#include "types.h"

void
create_functions(struct functions *ret)
{
	ret->contents = malloc(sizeof(*ret->contents) * 0x10);
	ret->capacity = 0x10;
	ret->length = 0x00;

	ret->data = malloc(0x200);
	ret->data_capacity = 0x200;
	ret->data_length = 0x00;

	ret->pnames = malloc(0x10 * sizeof(*ret->pnames));
	ret->ptypes = malloc(0x10 * sizeof(*ret->ptypes));
	ret->pcapacity = 0x10;
	ret->plength = 0x00;

	ret->body = malloc(0x200);
	ret->body[0] = '\0';
	ret->body_capacity = 0x200;
	ret->body_length = 1;
}

void
destroy_functions(struct functions *ret)
{
	free(ret->body);
	free(ret->pnames);
	free(ret->ptypes);
	free(ret->data);
	free(ret->contents);
}

void *
add_function_data(struct functions *ret, const void *data, size_t length)
{
	ret->data_length += length;
	if (ret->data_capacity < ret->data_length) {
		ret->data_capacity =
			(1 + ret->data_length / 0x200) * 0x200;
		ret->data = realloc(ret->data, ret->data_capacity);
	}

	memcpy(ret->data + ret->data_length - length, data, length);
	return ret->data + ret->data_length - length;
}

void
add_function(struct functions *ret, const char *name)
{
	size_t length, tl, i;
	char *s;

	ret->length += 1;
	if (ret->length > ret->capacity) {
		ret->capacity += 0x10;
		ret->contents = realloc(ret->contents,
			sizeof(*ret->contents) * ret->capacity);
	}

	ret->plength = 0;
	ret->body[0] = '\0';
	ret->body_length = 1;
	ret->contents[ret->length - 1].return_type = T_VOID;
	ret->contents[ret->length - 1].name = add_function_data(ret, name,
		strlen(name) + 1);

	for (s = (char *)ret->contents[ret->length - 1].name; *s; s++)
		*s = tolower(*s);
}

void
set_function_type(struct functions *ret, TYPE type) {
	ret->contents[ret->length - 1].return_type = type;
}

void
append_body(struct functions *ret, const char *body)
{
	ret->body_length += strlen(body);
	if(ret->body_length > ret->body_capacity) {
		ret->body_capacity = (1 + ret->body_length / 0x200) * 0x200;
		ret->body = realloc(ret->body, ret->body_capacity);
	}

	strcat(ret->body, body);
}

void
add_parameter(struct functions *ret, const char *name, TYPE type)
{
	ret->plength += 1;
	if (ret->plength > ret->pcapacity) {
		ret->pcapacity = (1 + ret->plength / 0x04) * 0x04;
		ret->pnames = realloc(ret->pnames,
			ret->pcapacity * sizeof(*ret->pnames));
		ret->ptypes = realloc(ret->ptypes,
			ret->pcapacity * sizeof(*ret->ptypes));
	}
	ret->pnames[ret->plength - 1] = add_function_data(ret, name,
		strlen(name) + 1);
	ret->ptypes[ret->plength - 1] = type;
}

void
finalise_function(struct functions *ret)
{
	ret->contents[ret->length - 1].pnames = add_function_data(ret,
		ret->pnames, ret->plength * sizeof(*ret->pnames));
	ret->contents[ret->length - 1].ptypes = add_function_data(ret,
		ret->ptypes, ret->plength * sizeof(*ret->ptypes));
	ret->contents[ret->length - 1].body = add_function_data(ret, ret->body,
		strlen(ret->body) + 1);
	ret->contents[ret->length - 1].pcount = ret->plength;
}

BOOLEAN
compile_expression(struct context *ctx, TYPE return_type, BOOLEAN drop)
{
	NUMCONST constant;
	BOOLEAN isword, negative;
	TYPE expr_type;
	char number[0x09];
	
	size_t i, j;

	if (is_number(ctx->lexer->token)) {
		constant = as_number(ctx->lexer->token, &isword, &negative);
	
		if (constant < 0x100 && isword)
			expr_type = T_BYTE;
		else if (constant < 0x100)
			expr_type = T_CHAR;
		else if (constant < 0x80000000 && isword)
			expr_type = T_WORD;
		else if (constant < 0x80000000)
			expr_type = T_INT;
		else if (negative)
			expr_type = T_INT;
		else
			expr_type = T_WORD;

		if (!drop) {
			append_body(&ctx->functions, "\t\t(i32.const 0x");
			sprintf(number, "%08.08x", constant);
			append_body(&ctx->functions, number);
			append_body(&ctx->functions, ")\n");
		}
	} else if (is_identifier(ctx->lexer->token)) {
		for (i = 0; i < ctx->functions.length; i++)
			if (!strcmp(ctx->lexer->token,
			ctx->functions.contents[i].name))
				break;

		p_assert(i != ctx->functions.length, "Unknown function %s in"
			" expression on line %u.\n", ctx->lexer->token,
			ctx->lexer->line);

		for (j = 0; j < ctx->functions.contents[i].pcount; j++) {
			next_token(ctx->lexer);
			p_assert(compile_expression(ctx, return_type, FALSE),
				"Expected parameter for function %s on line"
				" %u.\n", ctx->lexer->token, ctx->lexer->line);
		}

		expr_type = ctx->functions.contents[i].return_type;
		append_body(&ctx->functions, "\t\t(call $");
		append_body(&ctx->functions,
			ctx->functions.contents[i].name + 1);
		append_body(&ctx->functions, ")\n");

		if (drop)
			append_body(&ctx->functions, "\t\t(drop)\n");

	} else if (is_variable(ctx->lexer->token)) {
		for (i = 0; i < ctx->functions.plength; i++) {
			if (!strcmp(ctx->lexer->token,
			ctx->functions.pnames[i]))
				break;
		}
		p_assert(i != ctx->functions.plength, "Local variable %s does"
			" not exist on line %u.\n", ctx->lexer->token,
			ctx->lexer->line);
		if (!drop) {
			append_body(&ctx->functions, "\t\t(local.get ");
			append_body(&ctx->functions, ctx->lexer->token);
			append_body(&ctx->functions, ")\n");
		}
		expr_type = ctx->functions.ptypes[i];
	} else
		return FALSE;

	p_assert(type_associable(expr_type, return_type),
		"Expected type %s but got type %s in expression on"
		" line %u.", TYPE_STRINGS[return_type],
		TYPE_STRINGS[expr_type], ctx->lexer->line);
	return TRUE;
}

BOOLEAN 
compile_statement(struct context *ctx, TYPE return_type)
{
	size_t count;

	if (is_keyword(ctx->lexer->token, "RET")) {
		p_assert(return_type != TYPE__NULL, "Unexpected return"
			" statement on line %u.\n", ctx->lexer->line);

		if (return_type != T_VOID) {
			next_token(ctx->lexer);
			p_assert(compile_expression(ctx, return_type, FALSE),
				"Invalid expression in return on line %u.\n",
				ctx->lexer->line);
		}
		append_body(&ctx->functions, "\t\t(return)\n");
	} else {
		p_assert(compile_expression(ctx, return_type, TRUE),
			"Expected statement or expression, got %s on"
			" line %u.", ctx->lexer->token, ctx->lexer->line);
	}

	return TRUE;
}

BOOLEAN
compile_function(struct context *ctx)
{
	struct strbuilder body;
	BOOLEAN found_type, hold_token = FALSE;
	TYPE ptype;
	size_t pcount = 0, i;
	char *name;

	if (!is_identifier(ctx->lexer->token))
		return FALSE;

	name = malloc(strlen(ctx->lexer->token) + 1);
	strcpy(name, ctx->lexer->token);

	add_function(&ctx->functions, ctx->lexer->token);

	found_type = FALSE;
	while ((hold_token || next_token(ctx->lexer))
	&& !is_keyword(ctx->lexer->token, "DONE")) {
		hold_token = FALSE;
		if (is_keyword(ctx->lexer->token, "OUTPUTS")) {
			p_assert(!found_type, "Function %s has multiple"
				" types on line %u.\n", name, ctx->lexer->line);
			set_function_type(&ctx->functions,
				expect_type(FALSE, ctx->lexer));
			found_type = TRUE;
		} else if (is_keyword(ctx->lexer->token, "WITH")) {
			next_token(ctx->lexer);
			for (i = 0; is_type(ctx->lexer->token); i++) {
				ptype = expect_type(TRUE, ctx->lexer);
				p_assert(ptype != T_VOID, "Function %s has"
					" VOID parameter on line %u.\n", name,
					ctx->lexer->line);

				next_token(ctx->lexer);
				p_assert(is_variable(ctx->lexer->token),
					"Function %s has non variable"
					" parameter %s on line %u.\n",
					name, ctx->lexer->token,
					ctx->lexer->line);
				add_parameter(&ctx->functions,
					ctx->lexer->token, ptype);

				next_token(ctx->lexer);
			}
			hold_token = TRUE;
		} else if (is_keyword(ctx->lexer->token, "DO")) {
			ctx->in_function = TRUE;
			while (next_token(ctx->lexer)
			&& !is_keyword(ctx->lexer->token, "DONE")) {
				p_assert(compile_statement(ctx,
					ctx->functions.contents[
					ctx->functions.length - 1]
					.return_type), "Expected statement"
					" on line %u.\n", ctx->lexer->line);
			}
			ctx->in_function = FALSE;
			break;
		} else {
			p_assert(FALSE, "Invalid token %s in function header"
				" on line %u.\n", ctx->lexer->token,
				ctx->lexer->line);
		}
	}
	finalise_function(&ctx->functions);


	free(name);
}
