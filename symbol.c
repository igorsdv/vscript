#include "main.h"
#define SYMBOL_ARRAY_ALLOC_SIZE 64
#define SCOPE_ARRAY_ALLOC_SIZE 64

typedef struct {
	struct {
		char *value;
		byte nonlocal;
	} *symbols;
	int size;
	int length;
} Scope;

struct {
	Scope *array;
	int size;
} scopes = { 0, 0 };

Symbol make_symbol(int scope, int offset)
{
	Symbol s = { scope, offset };
	return s;
}

Symbol add_symbol(char *sym, int s, byte nonlocal)
{
	Scope *scope;
	int i;

	if (s < 0)
		ERROR("ScopeError: invalid nonlocal symbol <%s> at line %d", sym, line_no);
	if (s >= scopes.size)
	{
		scopes.array = realloc(scopes.array, (scopes.size + SCOPE_ARRAY_ALLOC_SIZE) * sizeof(Scope));
		memset((void *)(scopes.array + scopes.size * sizeof(Scope)), 0, SCOPE_ARRAY_ALLOC_SIZE * sizeof(Scope));
		scopes.size += SCOPE_ARRAY_ALLOC_SIZE;
	}
	scope = (Scope *)(scopes.array + s * sizeof(Scope));
	for (i = 0; i < scope->length; i++)
	{
		if (!strcmp(sym, scope->symbols[i].value))
		{
			printf("%s: %d %d %s %d\n", sym, s, i, scope->symbols[i].value, scope->symbols[i].nonlocal);
			if (scope->symbols[i].nonlocal)
				return add_symbol(sym, s - 1, 0);
			else
				return make_symbol(s, i);
		}
	}
	if (scope->length == scope->size)
	{
		scope->symbols = realloc(scope->symbols, (scope->size + SYMBOL_ARRAY_ALLOC_SIZE) * sizeof(void *));
		memset((void *)(scope->symbols + scope->size * sizeof(void *)), 0, SYMBOL_ARRAY_ALLOC_SIZE * sizeof(void *));
		scope->size += SYMBOL_ARRAY_ALLOC_SIZE;
	}
	scope->symbols[scope->length].value = malloc(strlen(sym) + 1);
	strcpy(scope->symbols[scope->length].value, sym);
	scope->symbols[scope->length].nonlocal = nonlocal;
	return nonlocal ? add_symbol(sym, s - 1, 0) : make_symbol(s, scope->length++);
}
