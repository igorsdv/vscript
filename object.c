#include "main.h"

Object *object_new(ObjectType type, char *string)
{
	Object *obj = malloc(sizeof(Object));
	if (type == INTEGER)
	{
		obj->value = malloc(sizeof(long));
		*(long *)obj->value = strtol(string, 0, 10);
		if (errno == ERANGE)
			ERROR("OverflowError: integer value out of bounds at line %d", line_no);
	}
	else if (type == FLOAT)
	{
		obj->value = malloc(sizeof(double));
		*(double *)obj->value = strtod(string, 0);
		if (errno == ERANGE)
			ERROR("OverflowError: floating-point value out of bounds at line %d", line_no);
	}
	else if (type == STRING)
	{
		obj->value = malloc(strlen(string) + 1);
		strcpy(obj->value, string);
	}
	else
		obj->value = 0;
	obj->type = type;
	obj->refcount = 0;
	return obj;
}

void object_free(Object *obj)
{
	free(obj->value);
	free(obj);
}
