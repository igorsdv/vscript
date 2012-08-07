#include "main.h"
#define PROGRAM_ARRAY_SIZE 1024

#define read_byte() *(byte *)read_bytes(co, offset, 1)
#define peek_byte() *(byte *)peek_bytes(co, offset, 1)
#define read_int() *(int *)read_bytes(co, offset, sizeof(int))
#define peek_int() *(int *)peek_bytes(co, offset, sizeof(int))
#define read_string() (char *)read_bytes(co, offset, strlen((char *)(env->co->program.array + &offset)) + 1)

char *opcodes[] = { "RETURN", "SET_LINE", "POP", "PRINT", "STORE", "LOAD", "LOAD_CONST", "CALL_FUNCTION", "JUMP", "POP_JUMP_IF_FALSE", "UNARY_PLUS", "UNARY_MINUS", "UNARY_NOT", "EQUAL", "NOT_EQUAL", "GREATER_THAN", "LESS_THAN", "GREATER_THAN_EQUAL", "LESS_THAN_EQUAL", "ADD", "SUB", "MULT", "DIV", "MOD" };

void write_bytes(struct code *co, void *value, size_t n)
{
	struct program *program = &co->program;
	if (program->length % PROGRAM_ARRAY_SIZE == 0 || program->length % PROGRAM_ARRAY_SIZE + n > PROGRAM_ARRAY_SIZE)
		program->array = safe_realloc(program->array, ((program->length + n) / PROGRAM_ARRAY_SIZE + 1) * PROGRAM_ARRAY_SIZE);
	memcpy(program->array + program->length, value, n);
	program->length += n;
}

void write_byte(struct code *co, byte b)
{
	struct program *program = &co->program;
	if (program->length % PROGRAM_ARRAY_SIZE == 0)
		program->array = safe_realloc(program->array, program->length + PROGRAM_ARRAY_SIZE);
	program->array[program->length++] = b;
}

void write_int(struct code *co, int i)
{
	write_bytes(co, &i, sizeof i);
}

byte *peek_bytes(struct code *co, int *offset, size_t n)
{
	return co->program.array + *offset;
}

byte *read_bytes(struct code *co, int *offset, size_t n)
{
	byte *value = co->program.array + *offset;
	*offset += n;
	return value;
}

void clear_program(struct code *co)
{
	if (co->program.length)
	{
		free(co->program.array);
		co->program.length = 0;
	}
}

void dis(struct code *co, int *offset)
{
	if (!offset)
		offset = safe_calloc(1, sizeof *offset);

	while (*offset < co->program.length)
	{
		enum opcode op = read_byte();
		printf("%u %s ", *offset - 1, opcodes[op]);
		switch (op)
		{
			case RETURN:
				putchar('\n');
				return;
			case STORE:
			case LOAD:
			case LOAD_CONST:
			case CALL_FUNCTION:
			case JUMP:
			case POP_JUMP_IF_FALSE:
			case SET_LINE:
				printf("%u \n", read_int());
				break;
			default:
				putchar('\n');
				break;
		}
	}
}

void run(struct env *env)
{
	struct code *co = env->co;
	int *offset = &env->offset;

	if (env->objects.length < co->symbols.length)
	{
		env->objects.array = safe_realloc(env->objects.array, co->symbols.length * sizeof *env->objects.array);
		while (env->objects.length < co->symbols.length)
			env->objects.array[env->objects.length++] = 0;
	}

	while (*offset < co->program.length)
	{
		enum opcode op = read_byte();

		if (op == RETURN)
			return;
		else if(op == SET_LINE)
			line_no = read_int();
		else if (op == POP)
			gc_collect(pop_stack());
		else if (op == PRINT)
		{
			struct object *object = pop_stack();
			
			switch (object->type)
			{
				case TYPE_NONE:
					puts("<null object>");
					break;
				case TYPE_CODE:
					dis((struct code *)object->value, 0);
					break;
				case TYPE_INT:
					printf("%d\n", *(int *)object->value);
					break;
				case TYPE_FLOAT:
					printf("%f\n", *(double *)object->value);
					break;
				case TYPE_STRING:
					printf("%s\n", (char *)object->value);
			}
			
			gc_collect(object);
		}
		else if (op == STORE)
			store_symbol(env, read_int());
		else if (op == LOAD)
			load_symbol(env, read_int());
		else if (op == LOAD_CONST)
			push_object(co->data.array[read_int()]);
		else if (op == CALL_FUNCTION)
		{
			int argc = read_int();

			struct object *object = pop_stack();
			struct env function = { 0 };	// create new execution environment

			if (object->type != TYPE_CODE)
				error("TypeError: object is not callable");

			function.co = (struct code *)object->value;
			function.parent = env;

			if (argc > function.co->argc)
				error("TypeError: function takes at most %d arguments (%d given)", function.co->argc, argc);
			while (argc++ < function.co->argc)
				push_object(null_object());

			// Tail-call elimination
			if(object->value == co		// same function ?
			   && peek_byte() == RETURN)	// returning call ?
				*offset = 0;
			else {
				run(&function);
				free(function.objects.array);
			}
		}
		else if (op == JUMP)
			*offset = read_int();
		else if (op == POP_JUMP_IF_FALSE)
		{
			struct object *object = pop_stack();

			if (bool_value(object))
				(void)read_int();
			else
				*offset = read_int();

			gc_collect(object);
		}
		else if (op == UNARY_PLUS)
		{
			enum object_type type = peek_stack()->type;

			if (type != TYPE_INT && type != TYPE_FLOAT)
				error("TypeError: object is not of numeric type");
		}
		else if (op == UNARY_MINUS)
		{
			struct object *object = pop_stack();

			if (object->type == TYPE_INT)
			{
				int value = -*(int *)object->value;
				if (value == INT_MIN)
					error("OverflowError: integer negation result out of bounds");
				
				push_object(new_object(TYPE_INT, &value));
			}
			else if (object->type == TYPE_FLOAT)
			{
				double value = -*(double *)object->value;
				push_object(new_object(TYPE_FLOAT, &value));
			}
			else
				error("TypeError: object is not of numeric type");

			gc_collect(object);
		}
		else if (op == UNARY_NOT)
		{
			struct object *object = pop_stack();
			
			int value = !bool_value(object);
			push_object(new_object(TYPE_INT, &value));

			gc_collect(object);
		}
		else if (op >= EQUAL && op <= LESS_THAN_EQUAL)
		{
			struct object *tos = pop_stack();
			struct object *tos2 = pop_stack();

			int result = compare(tos2, tos);

			if (result > 0)
			{
				if (op == GREATER_THAN || op == GREATER_THAN_EQUAL)
					result = 1;
				else
					result = 0;
			}
			else if (result < 0)
			{
				if (op == LESS_THAN || op == LESS_THAN_EQUAL)
					result = 1;
				else
					result = 0;
			}
			else if (op == EQUAL || op == GREATER_THAN_EQUAL || op == LESS_THAN_EQUAL)
				result = 1;

			if (op != EQUAL && op != NOT_EQUAL && tos->type == TYPE_CODE)
				error("TypeError: only equality comparison is defined on code objects");

			push_object(new_object(TYPE_INT, &result));

			gc_collect(tos);
			gc_collect(tos2);
		}
		else if (op >= ADD && op <= MOD)
		{
			struct object *(*function[])(struct object *, struct object *) = { &add, &subtract, &multiply, &divide, &modulo };

			struct object *tos = pop_stack();
			struct object *tos2 = pop_stack();

			push_object(function[op - ADD](tos2, tos));

			gc_collect(tos);
			gc_collect(tos2);
		}
		else
			error("unrecognized opcode %d", op);
	}
}
