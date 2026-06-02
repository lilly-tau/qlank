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
	TYPE return_type;
	const char *name, *body;
};

struct functions {
	struct function *contents;
	char *strings;
	size_t capacity, length, strings_capacity, strings_length;
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

void
add_function(struct functions *ret, const char *name, const char *body,
TYPE return_type);

BOOLEAN
compile_expression(struct strbuilder *ret, TYPE return_type,
struct context *ctx);

BOOLEAN
compile_statement(struct strbuilder *ret, TYPE return_type,
struct context *ctx);

BOOLEAN
compile_function(struct context *context);

#endif
