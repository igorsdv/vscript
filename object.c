#include "main.h"

struct object *new_object(enum object_type type, void *value)
{
	struct object *object = safe_malloc(sizeof *object);
	size_t size = 0;

	if (type == TYPE_INT) size = sizeof(long);
	else if (type == TYPE_FLOAT) size = sizeof(double);
	else if (type == TYPE_STRING) size = strlen(value) + 1;

	object->value = size ? safe_malloc(size) : value;
	if (size) memcpy(object->value, value, size);

	object->type = type;
	object->refcount = 0;
	return object;
}

struct object *null_object()
{
	static struct object none = { 0 };
	return &none;
}

struct object *make_object(enum object_type type, char *string)
{
	struct object *object = safe_malloc(sizeof *object);

	if (type == TYPE_INT)
	{
		object->value = safe_malloc(sizeof(long));
		*(long *)object->value = strtol(string, 0, 10);
		if (errno == ERANGE) error("OverflowError: integer value out of bounds");
	}
	else if (type == TYPE_FLOAT)
	{
		object->value = safe_malloc(sizeof(double));
		*(double *)object->value = strtod(string, 0);
		if (errno == ERANGE) error("OverflowError: floating-point value out of bounds");
	}
	else if (type == TYPE_STRING)
	{
		object->value = safe_malloc(strlen(string) + 1);
		strcpy(object->value, string);
	}
	else
		object->value = 0;

	object->type = type;
	object->refcount = 0;
	return object;
}

/* Garbage collection */

void gc_collect(struct object *object)
{
	if (object->refcount == 0)
	{
		free(object->value);
		free(object);
	}
}
