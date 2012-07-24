#include "main.h"
#define STACK_ARRAY_ALLOC_SIZE 64

static struct {
	Object **array;
	int size;
	int length;
} stack = { 0, 0, 0 };

void push_object(Object *obj)
{
	if (stack.length == stack.size)
		stack.array = realloc(stack.array, (stack.size += STACK_ARRAY_ALLOC_SIZE) * sizeof(void *));
	stack.array[stack.length++] = obj;
}

Object *pop_stack()
{
	if (!stack.length)
		ERROR("StackError: stack empty");
	return stack.array[--stack.length];
}

Object *peek_stack()
{
	if (!stack.length)
		ERROR("StackError: stack empty");
	return stack.array[stack.length - 1];
}
