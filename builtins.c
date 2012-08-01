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
			return *(long *)object->value != 0;
		case TYPE_FLOAT:
			return *(double *)object->value != 0;
		case TYPE_STRING:
			return *(char *)object->value != '\0';
	}
}
