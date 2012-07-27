#include "main.h"
#define SYMBOL_ARRAY_ALLOC_SIZE 64
#define SCOPE_ARRAY_ALLOC_SIZE 64

struct {
	struct scope {
		Object **objects;
		int size;
	} *array;
	int size;
} storage;

void load_symbol(Symbol *s)
{
	if (s->scope >= storage.size || s->offset >= storage.array[s->scope].size || !storage.array[s->scope].objects[s->offset])
		push_object(null_object());
	else
		push_object(storage.array[s->scope].objects[s->offset]);
}

void store_symbol(Symbol *s)
{
	struct scope *scope;
	while (s->scope >= storage.size)
	{
		storage.array = realloc(storage.array, (storage.size + SCOPE_ARRAY_ALLOC_SIZE) * sizeof(struct scope));
		memset(storage.array + storage.size * sizeof(struct scope), 0, SCOPE_ARRAY_ALLOC_SIZE * sizeof(struct scope));
		storage.size += SCOPE_ARRAY_ALLOC_SIZE;
	}
	scope = storage.array + s->scope;
	while (s->offset >= scope->size)
	{
		scope->objects = realloc(scope->objects, (scope->size + SYMBOL_ARRAY_ALLOC_SIZE) * sizeof(Object *));
		memset(scope->objects + scope->size * sizeof(Object *), 0, SYMBOL_ARRAY_ALLOC_SIZE * sizeof(Object *));
		scope->size += SYMBOL_ARRAY_ALLOC_SIZE;
	}
	if (scope->objects[s->offset])
	{
		scope->objects[s->offset]->refcount--;
		collect_object(scope->objects[s->offset]);
	}
	scope->objects[s->offset] = peek_stack();
	scope->objects[s->offset]->refcount++;
}
