#include "main.h"

Object *null_object()
{
	static Object none = { NONE, 0, 0 };
	return &none;
}

Object *new_object(ObjectType type, void *value)
{
	Object *obj = malloc(sizeof(Object));
	int size = 0;

	if (type == INTEGER)
		size = sizeof(long);
	else if (type == FLOAT)
		size = sizeof(double);
	else if (type == STRING)
		size = strlen(value) + 1;
	else if (type == FUNCTION)
		size = sizeof(Function);
	obj->value = size ? malloc(size) : 0;
	if (size)
		memcpy(obj->value, value, size);
	obj->type = type;
	obj->refcount = 0;
	return obj;
}

void collect_object(Object *obj)
{
	if (!obj->refcount && obj->type != NONE)
	{
		free(obj->value);
		free(obj);
	}
}
