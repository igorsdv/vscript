#include "main.h"
#define SYMBOL_ARRAY_SIZE 64
#define DATA_ARRAY_SIZE 64

int get_symbol(struct code *co, char *name, enum scope scope)
{
	struct symbols *symbols = &co->symbols;
	int i;

	for (i = 0; i < symbols->length; i++)
	{
		if (!strcmp(name, symbols->array[i].name))
		{
			if (scope == SCOPE_GLOBAL && symbols->array[i].scope != SCOPE_GLOBAL)
			{
				log("NameWarning: global variable '%s' referenced before declaration at line %u", name, line_no);
				symbols->array[i].scope = SCOPE_GLOBAL;
			}
			else if (scope == SCOPE_LOCAL && symbols->array[i].scope == SCOPE_NONLOCAL)
				error("NameError: local variable '%s' referenced before initialization", name, line_no);
			return i;
		}
	}

	if (i % SYMBOL_ARRAY_SIZE == 0)
		symbols->array = safe_realloc(symbols->array, (i + SYMBOL_ARRAY_SIZE) * sizeof *symbols->array);
	symbols->array[i].name = safe_malloc(strlen(name) + 1);
	strcpy(symbols->array[i].name, name);
	symbols->array[i].scope = scope;
	return symbols->length++;
}

int get_const(struct code *co, struct object *object)
{
	struct data *data = &co->data;
	if (data->length % DATA_ARRAY_SIZE == 0)
		data->array = safe_realloc(data->array, (data->length + DATA_ARRAY_SIZE) * sizeof *data->array);
	data->array[data->length] = object;
	++object->refcount;
	return data->length++;
}

void load_symbol(struct env *env, int s)
{
	char *name = env->co->symbols.array[s].name;
	enum scope scope = env->co->symbols.array[s].scope;
	int i;

	if (scope == SCOPE_LOCAL)
	{
		push_object(env->objects.array[s]);
		return;
	}

	if (scope == SCOPE_GLOBAL)
		while (env->parent) env = env->parent;

	do {
		for (i = 0; i < env->co->symbols.length; i++)
		{
			if (env->co->symbols.array[i].scope != SCOPE_NONLOCAL && !strcmp(name, env->co->symbols.array[i].name))
			{
				push_object(env->objects.array[i]);
				return;
			}
		}
	} while (env = env->parent);

	error("NameError: undefined variable '%s'", name);
}

void store_symbol(struct env *env, int s)
{
	char *name = env->co->symbols.array[s].name;
	enum scope scope = env->co->symbols.array[s].scope;

	if (scope == SCOPE_GLOBAL)
	{
		int length;

		while (env->parent) env = env->parent;

		length = env->co->symbols.length;
		s = get_symbol(env->co, name, SCOPE_LOCAL);
		
		if (env->co->symbols.length > env->objects.length)
		{
			env->objects.array = safe_realloc(env->objects.array, (length + 1) * sizeof *env->objects.array);
			env->objects.array[env->objects.length++] = 0;
		}
	}

	if (env->objects.array[s]) --env->objects.array[s]->refcount;
	env->objects.array[s] = peek_stack();
	++env->objects.array[s]->refcount;
}
