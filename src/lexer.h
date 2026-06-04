#ifndef X__LEXER_H__X
#define X__LEXER_H__X
#include <stddef.h>
#include "types.h"

struct lexer {
	char *token;
	size_t length, capacity, line;
	BOOLEAN eof;
};

void
create_lexer(struct lexer *ret);

void
destroy_lexer(struct lexer *ret);

BOOLEAN
next_token(struct lexer *ret);

BOOLEAN
is_identifier(const char *string);

BOOLEAN
is_variable(const char *string);

/* Keywords must be in full uppercase */
BOOLEAN
is_keyword(const char *value, const char *keyword);

BOOLEAN
is_number(const char *string);

NUMCONST
as_number(const char *string, BOOLEAN *risword, BOOLEAN *rnegative);

BOOLEAN
is_type(const char *string);

TYPE
expect_type(BOOLEAN current_token, struct lexer *lexer);

#endif
