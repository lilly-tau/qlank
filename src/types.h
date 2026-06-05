#ifndef X__TYPES_H__X
#define X__TYPES_H__X

typedef unsigned char BOOLEAN;
#define TRUE 0x01
#define FALSE 0x00

typedef unsigned long NUMCONST;

typedef enum {
	TYPE__NULL,
	T_VOID,
	T_CHAR,
	T_INT,
	T_BYTE,
	T_WORD,
	T_PTR,
	T_OFFSET,
	TYPE__MAX
} TYPE;

static const char *const TYPE_STRINGS[TYPE__MAX] = {
	"<???>", "VOID", "CHAR", "INT", "BYTE", "WORD", "PTR", "OFFSET"
};

static const unsigned char TYPE_SIZEOF[TYPE__MAX] =
	{ 0x00, 0x00, 0x01, 0x04, 0x01, 0x04, 0x04, 0x04 };

BOOLEAN
type_associable(TYPE a, TYPE b);

#endif

#ifdef TYPE_IMPL
#undef TYPE_IMPL

BOOLEAN
type_associable(TYPE a, TYPE b)
{
	if (a == b || b == TYPE__MAX)
		return TRUE;

	if (a == T_VOID || b == T_VOID)
		return FALSE;

	if (a == T_CHAR && b == T_INT)
		return TRUE;

	if (a == T_BYTE && b == T_WORD)
		return TRUE;

	if (a == T_OFFSET && b == T_PTR)
		return TRUE;

	return FALSE;
}

#endif
