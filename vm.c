#include "main.h"
#define VM_ALLOC_SIZE 256

#define read_byte() *(byte *)read_bytes(1)
#define read_int() *(int *)read_bytes(sizeof(int))
#define read_string() (char *)read_bytes(strlen((char *)(vm.program + vm.offset)) + 1)

struct {
	byte *program;
	int size;
	int length;
	int offset;
} vm = { 0, 0, 0, 0 };

void reset_vm()
{
	if (vm.size > VM_ALLOC_SIZE)
		vm.program = realloc(vm.program, vm.size = VM_ALLOC_SIZE);
	vm.length = 0;
	vm.offset = 0;
}

int get_offset()	// write offset
{
	return vm.length;
}

void write_bytes(void *value, int size)
{
	if (vm.length + size > vm.size)
		vm.program = realloc(vm.program, vm.size += VM_ALLOC_SIZE);
	memcpy((void *)(vm.program + vm.length), value, size);
	vm.length += size;
}

void write_byte(byte b)
{
	write_bytes(&b, 1);
}

void write_int(int i)
{
	write_bytes(&i, sizeof(int));
}

void write_int_at(int value, int target)
{
	// assuming target <= vm.length
	*(int *)(vm.program + target) = value;
}

void *read_bytes(int size)
{
	// assuming offset <= vm.length
	void *value = (void *)(vm.program + vm.offset);
	vm.offset += size;
	return value;
}

void run_vm()
{
	while (vm.offset < vm.length)
	{
		Opcode op = read_byte();
		if (op == SET_LINE)
			line_no = read_int();
		else if (op == POP)
			collect_object(pop_stack());
		else if (op == PRINT)
		{
			Object *o = pop_stack();
			if (o->type == NONE)
				puts("None");
			else if (o->type == INTEGER)
				printf("%ld\n", *(long *)o->value);
			else if (o->type == FLOAT)
				printf("%f\n", *(double *)o->value);
			else if (o->type == STRING)
				printf("%s\n", (char *)o->value);
		}
		else if (op == STORE)
		{
			Object *o = pop_stack();

			o->refcount++;
		}
		else if (op == LOAD)
		{
			Symbol s = *(Symbol *)read_bytes(sizeof(Symbol));
		}
		else if (op == LOAD_CONST)
		{
			Object *o;
			ObjectType t = read_byte();
			if (t == INTEGER)
			{
				long l = strtol(read_string(), 0, 10);
				if (errno == ERANGE)
					ERROR("OverflowError: integer value out of bounds at line %d", line_no);
				o = new_object(INTEGER, &l);
			}
			else if (t == FLOAT)
			{
				double d = strtod(read_string(), 0);
				if (errno == ERANGE)
					ERROR("OverflowError: floating-point value out of bounds at line %d", line_no);
				o = new_object(FLOAT, &d);
			}
			else if (t == STRING)
				o = new_object(STRING, read_string());
			else if (t == FUNCTION)
			{
				Function f = { read_int(), read_int() };
				o = new_object(FUNCTION, &f);
			}
			push_object(o);
		}
		else if (op == CALL_FUNCTION)
		{

		}
		else if (op == JUMP)
			vm.offset = read_int();

	}
}

void dis()
{
	char *opcodes[] = { "STOP", "SET_LINE", "POP", "PRINT", "STORE", "LOAD", "LOAD_CONST", "CALL_FUNCTION", "RETURN_VALUE", "JUMP", "POP_JUMP_IF_TRUE", "POP_JUMP_IF_FALSE", "UNARY_PLUS", "UNARY_MINUS", "UNARY_NOT", "EQUAL", "NOT_EQUAL", "GREATER_THAN", "LESS_THAN", "GREATER_THAN_EQUAL", "LESS_THAN_EQUAL", "ADD", "SUB", "MULT", "DIV" };

	while (vm.offset < vm.length)
	{
		int offset = vm.offset;
		Opcode op = read_byte();
		printf("%d %s ", offset, opcodes[op]);
		switch (op)
		{
		case STOP:
			printf("\n");
			return;
		case STORE:
		case LOAD:
			printf("%d ", read_int());
		case JUMP:
		case POP_JUMP_IF_TRUE:
		case POP_JUMP_IF_FALSE:
		case SET_LINE:
		case CALL_FUNCTION:
			printf("%d\n", read_int());
			break;
		case LOAD_CONST:
			switch (read_byte())
			{
				case NONE:
					printf("NONE\n");
					break;
				case INTEGER:
					printf("INTEGER %s\n", read_string());
					break;
				case FLOAT:
					printf("FLOAT %s\n", read_string());
					break;
				case STRING:
					printf("STRING %s\n", read_string());
					break;
				case FUNCTION:
					printf("FUNCTION %d ", read_int());
					printf("%d\n", read_int());
					break;
			}
			break;
		default:
			printf("\n");
			break;
		}
	}
}
