#include "main.h"

int bool_value(struct object *object)
{
	switch (object->type)
	{
		case TYPE_NONE:
			return 0;
		case TYPE_CODE:
			return 1;
		case TYPE_INT:
			return *(int *)object->value != 0;
		case TYPE_FLOAT:
			return *(double *)object->value != 0;
		case TYPE_STRING:
			return *(char *)object->value != '\0';
	}
}

int compare(struct object *a, struct object *b)
{
	if (a->type != b->type)
		error("TypeError: comparison of incompatible types");

	switch (a->type)
	{
		case TYPE_NONE:
			return 0;
		case TYPE_CODE:
			return a->value != b->value;
		case TYPE_INT:
			if (*(int *)a->value > *(int *)b->value)
				return 1;
			else if (*(int *)a->value < *(int *)b->value)
				return -1;
			else
				return 0;
		case TYPE_FLOAT:
			if (*(double *)a->value > *(double *)b->value)
				return 1;
			else if (*(double *)a->value < *(double *)b->value)
				return -1;
			else
				return 0;
		case TYPE_STRING:
			return strcmp(a->value, b->value);
	}
}

struct object * add(struct object *a, struct object *b)
{
	if (a->type != b->type)
		error("TypeError: addition of incompatible types");

	if (a->type == TYPE_NONE)
		error("TypeError: addition is not defined on null objects");
	else if (a->type == TYPE_CODE)
		error("TypeError: addition is not defined on code objects");
	else if (a->type == TYPE_INT)
	{
		long long result = (long long)*(int *)a->value + *(int *)b->value;
		int value = (int)result;
		
		if (result > INT_MAX || result < INT_MIN)
			error("OverflowError: integer addition result out of bounds");

		return new_object(TYPE_INT, &value);
	}
	else if (a->type == TYPE_FLOAT)
	{
		double value = *(double *)a->value + *(double *)b->value;

		if (isinf(value))
			error("OverflowError: floating-point addition result out of bounds");

		return new_object(TYPE_FLOAT, &value);
	}
	else if (a->type == TYPE_STRING)
	{
		struct object *object;
		
		char *value = safe_malloc(strlen(a->value) + strlen(b->value) + 1);
		strcpy(value, a->value);
		strcat(value, b->value);

		object = new_object(TYPE_STRING, value);
		free(value);

		return object;
	}
}

struct object * subtract(struct object *a, struct object *b)
{
	
}

struct object * multiply(struct object *a, struct object *b)
{
	
}

struct object * divide(struct object *a, struct object *b)
{
	
}
