#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef unsigned char byte;

#define MAX_NESTED_BLOCKS 256

int i_mode;		// interactive mode
int line_no;	// line number

#define ERROR(...) \
    do \
    { \
        fprintf(stderr, ##__VA_ARGS__); \
        putc('\n', stderr); \
        exit(1); \
    } while (0)

/* enums */

typedef enum {
	END,
	NEWLINE,		// used for line number
	PRINT_STMT,	
	IF_STMT,
	WHILE_STMT,
	BLOCK_START,	// colon
	BLOCK_END,		// unindent
	NAME,			// variable
	DOT,			// (not implemented)
	LITERAL,
	NUMERAL,
	MULT_OP,
	ADD_OP,
	COMP_OP,
	ASSIGN_OP,
	LPAREN,
	RPAREN,
	LBRACKET,
	RBRACKET,
	SEMICOLON
} TokenType;

typedef enum {
	NONE,
	INTEGER,
	STRING
} ObjectType;

typedef enum {
	STOP,
	INC_LINE,		// increment line number
	POP,
	PRINT,			// print and pop
	STORE,			// STORE (int symbol)
	LOAD,			// LOAD (int symbol)
	LOAD_CONST,		// LOAD_CONST (byte type, ...)
	JUMP,			// JUMP (int target)
	JUMP_IF_TRUE,
	JUMP_IF_FALSE,
	UNARY_PLUS,
	UNARY_MINUS,
	EQUAL,
	NOT_EQUAL,
	GREATER_THAN,
	LESS_THAN,
	GREATER_THAN_EQUAL,
	LESS_THAN_EQUAL,
	ADD,
	SUB,
	MULT,
	DIV
} Opcode;

/* structs */

typedef struct {
	TokenType type;
	int start;
	int length;
} Token;

typedef struct object
{
	ObjectType type;
	byte *value;	// array of bytes
	int size;		// size of array
	int refcount;
} Object;

typedef struct node {
	struct node *next;
	Object *object;
} Node;