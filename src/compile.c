#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compile.h"
#include "lexer.h"
#include "passert.h"
#include "types.h"

void
create_context(struct context *ctx, struct lexer *lexer)
{
	create_functions(&ctx->functions);

	ctx->constants = malloc(0x20 * sizeof(*ctx->constants));
	ctx->const_capacity = 0x20;
	ctx->const_length = 0x00;
	ctx->loop_count = 0;
	ctx->in_function = FALSE;

	ctx->lexer = lexer;
}

void
destroy_context(struct context *ctx)
{
	destroy_functions(&ctx->functions);
	free(ctx->constants);
}

void
create_constant(struct context *ctx, const char *name, TYPE type,
NUMCONST value)
{
	ctx->const_length += 1;
	if (ctx->const_length > ctx->const_capacity) {
		ctx->const_capacity += 0x20;
		ctx->constants = realloc(ctx->constants,
			ctx->const_capacity * sizeof(*ctx->constants));
	}
	ctx->constants[ctx->const_length - 1].name = add_function_data(
		&ctx->functions, name, strlen(name) + 1);
	ctx->constants[ctx->const_length - 1].type = type;
	ctx->constants[ctx->const_length - 1].value = value;
}

void
create_functions(struct functions *ret)
{
	ret->contents = malloc(sizeof(*ret->contents) * 0x10);
	ret->capacity = 0x10;
	ret->length = 0x00;

	ret->data = malloc(0x08 * sizeof(*ret->data));
	ret->data[0].memory = malloc(0x100000);
	ret->data[0].capacity = 0x100000;
	ret->data[0].length = 0x00;
	ret->dcapacity = 0x08;
	ret->dlength = 0x01;

	ret->pnames = malloc(0x10 * sizeof(*ret->pnames));
	ret->ptypes = malloc(0x10 * sizeof(*ret->ptypes));
	ret->pcapacity = 0x10;
	ret->plength = 0x00;

	ret->vnames = malloc(0x10 * sizeof(*ret->vnames));
	ret->vtypes = malloc(0x10 * sizeof(*ret->vtypes));
	ret->vcapacity = 0x10;
	ret->vlength = 0x00;

	ret->body = malloc(0x200);
	ret->body[0] = '\0';
	ret->body_capacity = 0x200;
	ret->body_length = 1;
}

void
destroy_functions(struct functions *ret)
{
	size_t i;

	for (i = 0 ; i < ret->dlength; i++)
		free(ret->data[i].memory);

	free(ret->data);
	free(ret->body);
	free(ret->pnames);
	free(ret->ptypes);
	free(ret->vnames);
	free(ret->vtypes);
	free(ret->contents);
}

void *
add_function_data(struct functions *ret, const void *data, size_t length)
{
	struct block *block;

	block = ret->data + ret->dlength - 1;
	if (block->length + length < block->capacity) {
		ret->dlength += 1;
		if (ret->dlength > ret->dcapacity) {
			ret->dcapacity += 0x08;
			ret->data = realloc(ret->data,
				sizeof(*ret->data) * ret->dcapacity);
		}
		block = ret->data + ret->dlength - 1;
		block->length = 0;
		block->capacity = 0x100000 > length ? 0x100000 : length;
		block->memory = malloc(block->capacity);
	}

	block->length += length;
	memcpy(block->memory + block->length - length, data, length);
	return block->memory + block->length - length;
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
	ret->vlength = 0;
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
add_variable(struct functions *ret, const char *name, TYPE type)
{
	ret->vlength += 1;
	if (ret->vlength > ret->vcapacity) {
		ret->vcapacity = (1 + ret->vlength / 0x04) * 0x04;
		ret->vnames = realloc(ret->vnames,
			ret->vcapacity * sizeof(*ret->vnames));
		ret->vtypes = realloc(ret->vtypes,
			ret->vcapacity * sizeof(*ret->vtypes));
	}
	ret->vnames[ret->vlength - 1] = add_function_data(ret, name,
		strlen(name) + 1);
	ret->vtypes[ret->vlength - 1] = type;
}

void
finalise_function(struct functions *ret)
{
	ret->contents[ret->length - 1].pnames = add_function_data(ret,
		ret->pnames, ret->plength * sizeof(*ret->pnames));
	ret->contents[ret->length - 1].ptypes = add_function_data(ret,
		ret->ptypes, ret->plength * sizeof(*ret->ptypes));
	ret->contents[ret->length - 1].vnames = add_function_data(ret,
		ret->vnames, ret->vlength * sizeof(*ret->vnames));
	ret->contents[ret->length - 1].vtypes = add_function_data(ret,
		ret->vtypes, ret->vlength * sizeof(*ret->vtypes));
	ret->contents[ret->length - 1].body = add_function_data(ret, ret->body,
		strlen(ret->body) + 1);
	ret->contents[ret->length - 1].pcount = ret->plength;
	ret->contents[ret->length - 1].vcount = ret->vlength;
}

NUMCONST
constexpr(struct context *ctx, TYPE *return_type)
{
	/*
		The following expressions are constant:
		integer constant
		constant identifier
		addition of constexpr
		parenthetical of constexpr
	*/
	NUMCONST ret;
	TYPE expr_type, type;
	BOOLEAN isword, negative;
	size_t i;

	if (!strcmp(ctx->lexer->token, "(")) {
		next_token(ctx->lexer);
		if (return_type != NULL)
			expr_type = *return_type;
		else
			expr_type = TYPE__NULL;

		ret = constexpr(ctx, &expr_type);

		next_token(ctx->lexer);

		if (is_keyword(ctx->lexer->token, "AS")) {
			expr_type = expect_type(FALSE, ctx->lexer);
			next_token(ctx->lexer);
		}

		p_assert(!strcmp(ctx->lexer->token, ")"), "Expected end of"
			" parenthetical in constexpr on line %u.\n",
			ctx->lexer->line);
	} else if (!strcmp(ctx->lexer->token, "+")) {
		next_token(ctx->lexer);
		if (return_type != NULL)
			expr_type = *return_type;
		else
			expr_type = TYPE__NULL;

		ret = constexpr(ctx, &expr_type);

		next_token(ctx->lexer);
		type = expr_type;
		ret += constexpr(ctx, &type);

		if (return_type != NULL && *return_type != TYPE__NULL) {
			p_assert(type_associable(type, expr_type), 
				"Expected constexpr of type %s got %s in"
				" addition on line %u.\n",
				TYPE_STRINGS[expr_type], TYPE_STRINGS[type],
				ctx->lexer->line);
		}
	} else if (is_number(ctx->lexer->token)) {
		ret = as_number(ctx->lexer->token, &isword, &negative);
	
		if (ret < 0x100 && isword)
			expr_type = T_BYTE;
		else if (ret < 0x100)
			expr_type = T_CHAR;
		else if (isword)
			expr_type = T_WORD;
		else if (ret < 0x80000000)
			expr_type = T_INT;
		else if (negative)
			expr_type = T_INT;
		else
			expr_type = T_WORD;
	} else if (is_constant(ctx->lexer->token)) {
		for (i = 0; i < ctx->const_length; i++)
			if (!strcmp(ctx->constants[i].name, ctx->lexer->token))
				break;

		p_assert(i != ctx->const_length, "constant %s does not exist"
			" in constexpr on line %u.\n", ctx->lexer->token,
			ctx->lexer->line);

		ret = ctx->constants[i].value;
		expr_type = ctx->constants[i].type;
	}

	if (return_type != NULL && *return_type != TYPE__NULL) {
		p_assert(type_associable(expr_type, *return_type), 
			"Expected constexpr of type %s got %s in addition"
			" on line %u.\n", TYPE_STRINGS[*return_type],
			TYPE_STRINGS[expr_type], ctx->lexer->line);
	}

	if (return_type != NULL)
		*return_type = expr_type;

	return ret;
}

TYPE
compile_expression(struct context *ctx, BOOLEAN drop)
{
	NUMCONST constant;
	BOOLEAN isword, negative;
	TYPE expr_type, at, bt;
	char number[0x09] = {0};
	
	size_t i, j;

	if (!strcmp(ctx->lexer->token, "(")) {
		next_token(ctx->lexer);
		p_assert(expr_type = compile_expression(ctx, drop),
			"Expected expression in parenthesis on line %u.\n",
			ctx->lexer->line);
		next_token(ctx->lexer);

		if (is_keyword(ctx->lexer->token, "AS")) {
			expr_type = expect_type(FALSE, ctx->lexer);
			next_token(ctx->lexer);
		}

		p_assert(!strcmp(ctx->lexer->token, ")"),
			"Expected closing parenthesis on line %u.\n",
			ctx->lexer->line);
	} else if (!strcmp(ctx->lexer->token, "+")) {
		next_token(ctx->lexer);
		p_assert(at = compile_expression(ctx, drop),
			"Expected expression in addition on line %u.\n",
			ctx->lexer->line);
		next_token(ctx->lexer);
		p_assert(bt = compile_expression(ctx, drop),
			"Expected expression in addition on line %u.\n",
			ctx->lexer->line);
		p_assert(type_associable(bt, at), "Type %s not associable"
			" to %s for addition on line %u.\n", TYPE_STRINGS[bt],
			TYPE_STRINGS[at], ctx->lexer->line);
		append_body(&ctx->functions, "\t\t(i32.add)\n");
		expr_type = at;
	} else if (is_number(ctx->lexer->token)) {
		constant = as_number(ctx->lexer->token, &isword, &negative);
	
		if (constant < 0x100 && isword)
			expr_type = T_BYTE;
		else if (constant < 0x100)
			expr_type = T_CHAR;
		else if (isword)
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
			p_assert(expr_type = compile_expression(ctx, FALSE),
				"Expected parameter %s for function %s on"
				" line %u.\n",
				ctx->functions.contents[i].pnames[j],
				ctx->functions.contents[i].name,
				ctx->lexer->line);
			p_assert(type_associable(expr_type,
				ctx->functions.contents[i].ptypes[j]),
				"Expected type %s for parameter %u of"
				" function %s on line %u.\n",
				TYPE_STRINGS[ctx->functions.contents[i]
				.ptypes[j]], j,
				ctx->functions.contents[i].name,
				ctx->lexer->line);
		}

		expr_type = ctx->functions.contents[i].return_type;
		append_body(&ctx->functions, "\t\t(call $");
		append_body(&ctx->functions,
			ctx->functions.contents[i].name + 1);
		append_body(&ctx->functions, ")\n");

		if (drop && expr_type != T_VOID)
			append_body(&ctx->functions, "\t\t(drop)\n");

	} else if (is_variable(ctx->lexer->token)) {
		for (i = 0; i < ctx->functions.plength; i++) {
			if (!strcmp(ctx->lexer->token,
			ctx->functions.pnames[i]))
				break;
		}
		if (i == ctx->functions.plength) {
			for (i = 0; i < ctx->functions.vlength; i++) {
				if (!strcmp(ctx->lexer->token,
				ctx->functions.vnames[i]))
					break;
			}
			p_assert(i != ctx->functions.vlength, "Local variable"
				" %s does not exist on line %u.\n",
				ctx->lexer->token, ctx->lexer->line);
			expr_type = ctx->functions.vtypes[i];
		} else
			expr_type = ctx->functions.ptypes[i];

		if (!drop) {
			append_body(&ctx->functions, "\t\t(local.get ");
			append_body(&ctx->functions, ctx->lexer->token);
			append_body(&ctx->functions, ")\n");
		}
	} else if (is_keyword(ctx->lexer->token, "CONSTEXPR")) {
		next_token(ctx->lexer);
		expr_type = TYPE__NULL;
		constant = constexpr(ctx, &expr_type);

		if (!drop) {
			append_body(&ctx->functions, "\t\t(i32.const 0x");
			sprintf(number, "%08.08x", constant);
			append_body(&ctx->functions, number);
			append_body(&ctx->functions, ")\n");
		}
	} else if (is_constant(ctx->lexer->token)) {
		for (i = 0; i < ctx->const_length; i++)
			if (!strcmp(ctx->constants[i].name, ctx->lexer->token))
				break;

		p_assert(i != ctx->const_length, "constant %s does not exist"
			" in expression on line %u.\n", ctx->lexer->token,
			ctx->lexer->line);

		constant = ctx->constants[i].value;
		expr_type = ctx->constants[i].type;
		if (!drop) {
			append_body(&ctx->functions, "\t\t(i32.const 0x");
			sprintf(number, "%08.08x", constant);
			append_body(&ctx->functions, number);
			append_body(&ctx->functions, ")\n");
		}

		next_token(ctx->lexer);
		compile_expression(ctx, drop);

		if (!drop)
			append_body(&ctx->functions, "\t\t(i32.add)\n");
	} else if (at = expect_type(TRUE, ctx->lexer)) {
		next_token(ctx->lexer);
		if (!is_keyword(ctx->lexer->token, "OF"))
			return TYPE__NULL;
		next_token(ctx->lexer);

		p_assert(type_associable(bt = compile_expression(ctx, FALSE),
			T_PTR), "Expected PTR expression in 'OF' statement on"
			" line %u.\n", ctx->lexer->line);

		if (!drop) {
			switch (at) {
			case T_CHAR:
				append_body(&ctx->functions,
					"\t\t(i32.load8_s)\n");
				break;
			case T_BYTE:
				append_body(&ctx->functions,
					"\t\t(i32.load8_u)\n");
				break;
			case T_INT:
			case T_WORD:
				append_body(&ctx->functions,
					"\t\t(i32.load)\n");
				break;
			}
		}
	} else
		return TYPE__NULL;

	return expr_type;
}

BOOLEAN 
compile_statement(struct context *ctx, TYPE return_type)
{
	size_t i, original_line, ident;
	const char *name;
	TYPE type, type2;
	BOOLEAN status;
	char number[9] = {0};

	if (is_keyword(ctx->lexer->token, "RET")) {
		p_assert(return_type != TYPE__NULL, "Unexpected return"
			" statement on line %u.\n", ctx->lexer->line);

		if (return_type != T_VOID) {
			next_token(ctx->lexer);
			p_assert(type = compile_expression(ctx, FALSE),
				"Invalid expression in return on line %u.\n",
				ctx->lexer->line);
			p_assert(type_associable(type, return_type),
				"Expected type %s but got type %s in"
				" return on line %u.",
				TYPE_STRINGS[return_type],
				TYPE_STRINGS[type], ctx->lexer->line);
		}

		append_body(&ctx->functions, "\t\t(return)\n");
	} else if (is_keyword(ctx->lexer->token, "SET")) {
		next_token(ctx->lexer);
		p_assert(is_variable(ctx->lexer->token), "Expected variable "
			"to be set on line %u.\n");

		for (i = 0; i < ctx->functions.plength; i++) {
			if (!strcmp(ctx->lexer->token,
			ctx->functions.pnames[i])) {
				name = ctx->functions.pnames[i];
				type = ctx->functions.ptypes[i];
				break;
			}
		}
		if (i == ctx->functions.plength) {
			for (i = 0; i < ctx->functions.vlength; i++)
				if (!strcmp(ctx->lexer->token,
				ctx->functions.vnames[i])) {
					name = ctx->functions.vnames[i];
					type = ctx->functions.vtypes[i];
					break;
				}
			p_assert(i != ctx->functions.vlength, "Variable "
				"%s does not exist on line %u.\n",
				ctx->lexer->token);
			
		}

		next_token(ctx->lexer);
		p_assert(type2 = compile_expression(ctx, FALSE),
			"Invalid expression on line %u.\n",
			ctx->lexer->line);

		p_assert(type_associable(type2, type), "Cannot set %s of"
			" type %s from type %s on line %u.\n", name,
			TYPE_STRINGS[type], TYPE_STRINGS[type2],
			ctx->lexer->line);

		append_body(&ctx->functions, "\t\t(local.set ");
		append_body(&ctx->functions, name);
		append_body(&ctx->functions, ")\n");
	} else if (is_keyword(ctx->lexer->token, "STORE")) {
		next_token(ctx->lexer);
		p_assert(type_associable(compile_expression(ctx, FALSE),
			T_PTR), "Expected a PTR expression for STORE on"
			" line %u.\n", ctx->lexer->line);

		p_assert(type = expect_type(FALSE, ctx->lexer),
			"Store expects an insertion type on line %u.\n",
			ctx->lexer->line);

		next_token(ctx->lexer);
		p_assert(type_associable(type2 = compile_expression(ctx,
			FALSE), type),
			"Store expects expression associable to %s"
			" got type %s on line %u.\n", TYPE_STRINGS[type],
			TYPE_STRINGS[type2], ctx->lexer->line);

		append_body(&ctx->functions, "\t\t(i32.store)\n");
	} else if (is_keyword(ctx->lexer->token, "IF")) {
		next_token(ctx->lexer);

		p_assert(type = compile_expression(ctx, FALSE), "If expects"
			" expression on line %u.\n", ctx->lexer->line);

		p_assert(type_associable(type, T_WORD), "If expects a type"
			" which is associable to WORD, got %s on line %u.\n",
			TYPE_STRINGS[type], ctx->lexer->line);

		next_token(ctx->lexer);
		p_assert(is_keyword(ctx->lexer->token, "DO"), "If expects to"
			" be followed by block on line %u.\n",
			ctx->lexer->line);

		append_body(&ctx->functions, "\t(if (then\n");

		original_line = ctx->lexer->line;
		while ((status = next_token(ctx->lexer))
		&& !is_keyword(ctx->lexer->token, "DONE")) {
			p_assert(compile_statement(ctx,
				ctx->functions.contents[ctx->functions.length
				- 1].return_type), "Expected statement"
				" on line %u.\n", ctx->lexer->line);
		}
		p_assert(status, "Unclosed block on line %u.\n",
			original_line);

		append_body(&ctx->functions, "\t))\n");
	} else if (is_keyword(ctx->lexer->token, "WHILE")) {
		next_token(ctx->lexer);

		ident = ctx->loop_count++;
		append_body(&ctx->functions, "\t(block $loop_end");
		sprintf(number, "%08.08X", ident);
		append_body(&ctx->functions, number);
		append_body(&ctx->functions, "\n\t(loop $loop");
		append_body(&ctx->functions, number);
		append_body(&ctx->functions, "\n");

		p_assert(type = compile_expression(ctx, FALSE), "While"
			" expects expression on line %u.\n", ctx->lexer->line);

		p_assert(type_associable(type, T_WORD), "While expects a type"
			" which is associable to WORD, got %s on line %u.\n",
			TYPE_STRINGS[type], ctx->lexer->line);

		append_body(&ctx->functions, "\t\t(i32.eqz)\n"
			"\t\t(if (then br $loop_end");
		sprintf(number, "%08.08X", ident);
		append_body(&ctx->functions, number);
		append_body(&ctx->functions, "))\n");

		next_token(ctx->lexer);
		p_assert(is_keyword(ctx->lexer->token, "DO"), "While expects"
			" to be followed by block on line %u.\n",
			ctx->lexer->line);


		original_line = ctx->lexer->line;
		while ((status = next_token(ctx->lexer))
		&& !is_keyword(ctx->lexer->token, "DONE")) {
			p_assert(compile_statement(ctx,
				ctx->functions.contents[ctx->functions.length
				- 1].return_type), "Expected statement"
				" on line %u.\n", ctx->lexer->line);
		}
		p_assert(status, "Unclosed block on line %u.\n",
			original_line);

		append_body(&ctx->functions, "\t\t(br $loop");
		sprintf(number, "%08.08X", ident);
		append_body(&ctx->functions, number);
		append_body(&ctx->functions, ")\n\t))\n");
	} else {
		p_assert(compile_expression(ctx, TRUE),
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
	TYPE type;
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
				type = expect_type(TRUE, ctx->lexer);
				p_assert(type != T_VOID, "Function %s has"
					" VOID parameter on line %u.\n", name,
					ctx->lexer->line);

				next_token(ctx->lexer);
				p_assert(is_variable(ctx->lexer->token),
					"Function %s has non variable"
					" parameter %s on line %u.\n",
					name, ctx->lexer->token,
					ctx->lexer->line);
				add_parameter(&ctx->functions,
					ctx->lexer->token, type);

				next_token(ctx->lexer);
			}
			hold_token = TRUE;
		} else if (is_keyword(ctx->lexer->token, "USING")) {
			next_token(ctx->lexer);
			for (i = 0; is_type(ctx->lexer->token); i++) {
				type = expect_type(TRUE, ctx->lexer);
				p_assert(type != T_VOID, "Function %s has"
					" VOID parameter on line %u.\n", name,
					ctx->lexer->line);

				next_token(ctx->lexer);
				p_assert(is_variable(ctx->lexer->token),
					"Function %s has non variable"
					" parameter %s on line %u.\n",
					name, ctx->lexer->token,
					ctx->lexer->line);
				add_variable(&ctx->functions,
					ctx->lexer->token, type);

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
	p_assert(!(ctx->functions.vlength > 0 && !*ctx->functions.body),
		"Function prototype %s has local variables on line %u.\n",
		name);
	finalise_function(&ctx->functions);

	free(name);
	return TRUE;
}

BOOLEAN
compile_constant(struct context *ctx)
{
	char *name, *tmp;
	TYPE type;
	NUMCONST offset;

	if (!is_constant(ctx->lexer->token))
		return FALSE;

	name = malloc(strlen(ctx->lexer->token) + 1);
	strcpy(name, ctx->lexer->token);

	next_token(ctx->lexer);
	if (is_type(ctx->lexer->token)) {
		type = expect_type(TRUE, ctx->lexer);
		next_token(ctx->lexer);
		create_constant(ctx, name, type, constexpr(ctx, &type));
	} else if (is_keyword(ctx->lexer->token, "WITH")) {
		offset = 0;
		tmp = malloc(0x01);
		next_token(ctx->lexer);
		while (!is_keyword(ctx->lexer->token, "DONE")) {
			type = expect_type(TRUE, ctx->lexer);
			next_token(ctx->lexer);
			p_assert(is_variable(ctx->lexer->token), "Expected"
				" variable name after constant type on"
				" line %u.\n", ctx->lexer->line);
			tmp = realloc(tmp,
				strlen(name) + strlen(ctx->lexer->token) + 1);
			sprintf(tmp, "%s.%s", name, ctx->lexer->token + 1);
			create_constant(ctx, tmp, type, offset);
			offset += TYPE_SIZEOF[type];
			next_token(ctx->lexer);
		}
		free(tmp);
	}

	free(name);
	return TRUE;
}
