#ifndef X__COMPILE_H__X
#define X__COMPILE_H__X
#include <stddef.h>
#include "lexer.h"
#include "passert.h"
#include "strbuilder.h"
#include "types.h"

typedef enum {
	E__NULL,
	E_NUMCONST,
	E__MAX
} EXPR_TYPE;

struct expr {
	EXPR_TYPE type;
	NUMCONST value;
};

struct function {
	const char *name, *body;
	const char **pnames, **vnames;
	const TYPE *ptypes, *vtypes;
	size_t pcount, vcount;
	TYPE return_type;
};

struct block {
	unsigned char *memory;
	size_t length, capacity;
};

struct functions {
	struct function *contents;
	struct block *data;
	char *body;
	char **pnames, **vnames;
	TYPE *ptypes, *vtypes;
	size_t capacity, length, pcapacity, plength, body_length,
		body_capacity, vcapacity, vlength, dlength, dcapacity;
};

struct constant {
	char *name;
	TYPE type;
	NUMCONST value;
};

struct context {
	struct functions functions;
	struct constant *constants;
	struct lexer *lexer;
	size_t loop_count, const_length, const_capacity;
	BOOLEAN in_function;
};

void
create_context(struct context *ctx, struct lexer *lexer);

void
destroy_context(struct context *ctx);

void
create_constant(struct context *ctx, const char *name, TYPE type,
NUMCONST value);

void
create_functions(struct functions *ret);

void
destroy_functions(struct functions *ret);

void *
add_function_data(struct functions *ret, const void *data, size_t length);

void
add_function(struct functions *ret, const char *name);

void
set_function_type(struct functions *ret, TYPE type);

void
append_body(struct functions *ret, const char *body);

void
add_parameter(struct functions *ret, const char *name, TYPE type);

void
add_constant(struct functions *ret, const char *name);

void
constant_type(struct functions *ret, TYPE type);

void
constant_value(struct functions *ret, NUMCONST value);

void
finalise_function(struct functions *ret);

TYPE
compile_expression(struct context *ctx, BOOLEAN drop);

BOOLEAN
compile_statement(struct context *ctx, TYPE return_type);

BOOLEAN
compile_function(struct context *context);

#endif
