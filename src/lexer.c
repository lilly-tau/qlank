#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "types.h"

void
create_lexer(struct lexer *ret)
{
	ret->token = malloc(0x200);
	ret->capacity = 0x200;
	ret->length = 0;
	ret->line = 1;
	ret->eof = FALSE;
}

void
destroy_lexer(struct lexer *ret)
{
	free(ret->token);
}

BOOLEAN
next_token(struct lexer *ret)
{
	int c;

	if (ret->eof)
		return FALSE;

	while (isspace(c = getchar()))
		if (c == '\n')
			ret->line += 1;

	if (c == EOF) {
		ret->eof = TRUE;
		return FALSE;
	}

	ret->length = 0;

	do {
		if (c == ';') {
			while (c != EOF && c != '\n')
				c = getchar();
			ret->line += 1;
			if (ret->length != 0)
				break;

			while (isspace(c = getchar()))
				if (c == '\n')
					ret->line += 1;
		}
		ret->token[ret->length] = isalpha(c) ? tolower(c) : c;
		ret->length += 1;
		if (ret->length >= ret->capacity) {
			ret->capacity += 0x200;
			ret->token = realloc(ret->token, ret->capacity);
		}
	} while (!isspace(c = getchar()));
	ungetc(c, stdin);

	if (c == EOF)
		ret->eof = TRUE;

	ret->token[ret->length] = 0;
	return TRUE;
}

BOOLEAN
is_identifier(const char *string)
{
	size_t i;

	if (string[0] != '@')
		return FALSE;

	for(i = 1; string[i]; i++)
		if (!isalpha(string[i]) && string[i] != '_')
			return FALSE;

	return TRUE;
}

BOOLEAN
is_constant(const char *string)
{
	size_t i;

	if (string[0] != '%')
		return FALSE;

	for(i = 1; string[i]; i++)
		if (!isalpha(string[i]) && string[i] != '_'
		&& string[i] != '.')
			return FALSE;

	return TRUE;
}

BOOLEAN
is_variable(const char *string)
{
	size_t i;

	if (string[0] != '$')
		return FALSE;

	for(i = 1; string[i]; i++)
		if (!isalpha(string[i]) && string[i] != '_')
			return FALSE;

	return TRUE;
}

BOOLEAN
is_keyword(const char *value, const char *keyword)
{
	size_t i, length;

	length = strlen(value);
	if (length != strlen(keyword))
		return FALSE;

	for (i = 0; i < length; i++)
		if (toupper(value[i]) != keyword[i])
			return FALSE;

	return TRUE;
}

BOOLEAN
is_number(const char *string)
{
	while (*string == '-' || (isalpha(*string)
	&& toupper(*string) == 'U')) ++string;

	if (!isdigit(*string))
		return FALSE;

	for (++string; *string && isxdigit(*string); ++string);

	return !*string;
}

NUMCONST
as_number(const char *string, BOOLEAN *risword, BOOLEAN *rnegative)
{
	NUMCONST ret;
	BOOLEAN negative = FALSE, isword = FALSE;

	for (; !isdigit(*string); ++string) {
		switch(*string) {
		case '-':
			negative = TRUE;
			break;
		case 'u':
			isword = TRUE;
			break;
		}
	}

	for (ret = 0; *string; ++string) {
		ret <<= 4;
		ret |= (tolower(*string) - 0x30 - 0x7 * (*string > '9')) & 0xF;
	}

	if (negative)
		ret = ~ret + 1;

	if (risword != NULL)
		*risword = isword;

	if (rnegative != NULL)
		*rnegative = negative;

	return ret;
}

BOOLEAN
is_type(const char *string) {
	TYPE type;

	for (type = TYPE__NULL + 1; type < TYPE__MAX; type++) {
		if (is_keyword(string, TYPE_STRINGS[type]))
			return TRUE;
	}

	return FALSE;
}

TYPE
expect_type(BOOLEAN current_token, struct lexer *lexer)
{
	TYPE type;

	if (!current_token)
		next_token(lexer);

	for (type = TYPE__NULL + 1; type < TYPE__MAX; type++)
		if (is_keyword(lexer->token, TYPE_STRINGS[type]))
			return type;

	p_assert(FALSE, "Expected type, got %s on line %u.", lexer->token,
		lexer->line);
}
