#include "main.h"
#define SIZE_INCREMENT 64

// int s is scope, assume s < MAX_NESTED_BLOCKS

char **symbols[MAX_NESTED_BLOCKS] = { 0 };
int symbols_length[MAX_NESTED_BLOCKS] = { 0 };
int symbols_size[MAX_NESTED_BLOCKS] = { 0 };

void sym_clear_scope(int s)
{
	while (symbols_length[s])
		free(symbols[s][--symbols_length[s]]);
}

void sym_free()
{
	int i;

	for (i = 0; i < MAX_NESTED_BLOCKS; i++)
	{
		sym_clear_scope(i);
		free(symbols[i]);
	}
}

int sym_lookup(char *sym, int s)
{
	do
	{
		int i;

		for (i = 0; i < symbols_length[s]; i++)
		{
			if (!strcmp(sym, symbols[s][i]))
				return i;
		}
	} while (s--);
	ERROR("NameError: undefined variable '%s' at line %d", sym, line_no);
	return 0;
}

int sym_add(char *sym, int s)	// might not be needed; merge with sym_addlookup
{
	if (symbols_length[s] == symbols_size[s])
		symbols[s] = realloc(symbols[s], symbols_size[s] += SIZE_INCREMENT);	// will malloc if null
	symbols[s][symbols_length[s]] = malloc(strlen(sym) + 1);
	strcpy(symbols[s][symbols_length[s]], sym);
	return symbols_length[s]++;
}

int sym_addlookup(char *sym, int s)
{
    int scope = s;

    do
	{
		int i;

		for (i = 0; i < symbols_length[s]; i++)
		{
			if (!strcmp(sym, symbols[s][i]))
				return i;
		}
	} while (s--);
    return sym_add(sym, scope);
}

