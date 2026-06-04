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
	const char **pnames;
	const TYPE *ptypes;
	size_t pcount;
	TYPE return_type;
};

struct functions {
	struct function *contents;
	unsigned char *data;
	char *body;
	char **pnames;
	TYPE *ptypes;
	size_t capacity, length, data_capacity, data_length,
		pcapacity, plength, body_length, body_capacity;
};

struct context {
	struct functions functions;
	struct lexer *lexer;
	BOOLEAN in_function;
};

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
finalise_function(struct functions *ret);

BOOLEAN
compile_expression(struct context *ctx, TYPE return_type, BOOLEAN drop);

BOOLEAN
compile_statement(struct context *ctx, TYPE return_type);

BOOLEAN
compile_function(struct context *context);

#endif
