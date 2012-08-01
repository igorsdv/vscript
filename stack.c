#include "main.h"
#define STACK_ARRAY_SIZE 64

static struct {
	size_t length;
	size_t size;
	struct object **array;
} stack = { 0 };

void push_object(struct object *object)
{
	if (stack.length == stack.size)
		stack.array = safe_realloc(stack.array, (stack.size += STACK_ARRAY_SIZE) * sizeof *stack.array);
	stack.array[stack.length++] = object;
}

struct object *pop_stack()
{
	if (!stack.length)
		error("StackError: stack empty");
	return stack.array[--stack.length];
}

struct object *peek_stack()
{
	if (!stack.length)
		error("StackError: stack empty");
	return stack.array[stack.length - 1];
}
