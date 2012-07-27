#include "main.h"
#define SYMBOL_ARRAY_ALLOC_SIZE 64
#define SCOPE_ARRAY_ALLOC_SIZE 64

struct {
	struct scope {
		struct symbol {
			char *value;
			byte global;
		} *symbols;
		int size;
		int length;
	} *array;
	int size;
} scopes = { 0 };

void add_scope(int s)
{
	while (s >= scopes.size)
	{
		scopes.array = realloc(scopes.array, (scopes.size + SCOPE_ARRAY_ALLOC_SIZE) * sizeof(struct scope));
		memset(scopes.array + scopes.size, 0, SCOPE_ARRAY_ALLOC_SIZE * sizeof(struct scope));
		scopes.size += SCOPE_ARRAY_ALLOC_SIZE;
	}
}

void reset_scope(int s)
{
	struct scope *scope = scopes.array + s;
	while (scope->length)
		free(scope->symbols[--scope->length].value);
	if (scope->size > SYMBOL_ARRAY_ALLOC_SIZE)
		scope->symbols = realloc(scope->symbols, (scope->size = SYMBOL_ARRAY_ALLOC_SIZE) * sizeof(struct symbol *));
	memset(scope->symbols, 0, SYMBOL_ARRAY_ALLOC_SIZE * sizeof(struct symbol *));
}

Symbol make_symbol(int scope, int offset)
{
	Symbol s = { scope, offset };
	return s;
}

Symbol lookup_add_symbol(char *sym, int s, byte lookup, byte add, byte global)
{
	int i;
	struct scope *scope = scopes.array + s;

	for (i = 0; i < scope->length; i++)
		if (!strcmp(sym, scope->symbols[i].value))
		{
			if (!lookup)
				ERROR("NameError: duplicate declaration of symbol <%s> at line %d", sym, line_no);
			if (scope->symbols[i].global)
				return lookup_add_symbol(sym, 0, lookup, add, 0);
			else
				return make_symbol(s, i);
		}
	if (!add)
		ERROR("NameError: undefined symbol <%s> at line %d", sym, line_no);
	if (scope->length == scope->size)
	{
		scope->symbols = realloc(scope->symbols, (scope->size + SYMBOL_ARRAY_ALLOC_SIZE) * sizeof(struct symbol *));
		memset(scope->symbols + scope->size, 0, SYMBOL_ARRAY_ALLOC_SIZE * sizeof(struct symbol *));
		scope->size += SYMBOL_ARRAY_ALLOC_SIZE;
	}
	scope->symbols[scope->length].value = malloc(strlen(sym) + 1);
	strcpy(scope->symbols[scope->length].value, sym);
	scope->symbols[scope->length].global = s ? global : 0;	// already global scope; allowed but set .global = 0
	return make_symbol(s, scope->length++);
}
