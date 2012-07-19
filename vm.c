#include "main.h"
#define SIZE_INCREMENT 256

byte *program = 0;
int program_length, program_size, offset;

void vm_reset()
{
	program = realloc(program, program_size = SIZE_INCREMENT);
	program_length = 0;
}

void vm_free()
{
	free(program);
}

void extend()
{
	program = realloc(program, program_size += SIZE_INCREMENT);
}

int vm_getoffset()
{
	return program_length;
}

void vm_write(byte *value, int size)
{
	int i = -1;

	if (program_length + size > program_size)
		extend();
	while (++i < size)
		*(byte*)(program + program_length + i) = *(byte*)(value + i);
	program_length += size;
}

void vm_writebyte(byte value)
{
	if (program_length == program_size)
		extend();
	*(byte*)(program + program_length++) = value;
}

void vm_writeint(int value)
{
	if (program_length + sizeof(int) > program_size)
		extend();
	*(int*)(program + program_length) = value;
	program_length += sizeof(int);
}

void vm_writeintat(int value, int target)
{
	// assuming target <= program_length
	*(int*)(program + target) = value;
}

byte *vm_read(int size)
{
	// assuming offset <= program_length
	byte *value = program + offset;
	offset += size;
	return value;
}

void vm_run()
{
	char opcodes[][20] = { "STOP","SET_LINE","POP","PRINT","STORE","LOAD","LOAD_CONST","JUMP","JUMP_IF_TRUE","JUMP_IF_FALSE","UNARY_PLUS","UNARY_MINUS","EQUAL","NOT_EQUAL","GREATER_THAN","LESS_THAN","GREATER_THAN_EQUAL","LESS_THAN_EQUAL","ADD","SUB","MULT","DIV" };

	offset = 0;

	while (offset < program_length)
	{
		int opcode;
		printf("%d ", offset);
		printf("%s", opcodes[opcode = *vm_read(sizeof(byte))]);
		switch (opcode)
		{
		case STOP:
			printf("\n");
			return;
		case STORE:
		case LOAD:
		case JUMP:
		case JUMP_IF_TRUE:
		case JUMP_IF_FALSE:
		case SET_LINE:
			printf(" %d\n", *vm_read(sizeof(int)));
			break;
		case LOAD_CONST:
			switch (*vm_read(sizeof(byte)))
			{
				case NONE:
					printf(" NONE\n");
					break;
				case NUMBER:
					printf(" NUMBER %s\n", vm_read(0));
					offset += strlen(vm_read(0)) + 1;
					break;
				case STRING:
					printf(" STRING %s\n", vm_read(0));
					offset += strlen(vm_read(0)) + 1;
					break;
			}
			break;
		default:
			printf("\n");
			break;
		}
	}
}