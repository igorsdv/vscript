#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

typedef unsigned char byte;

#define MAX_NESTED_BLOCKS 256
#define MAX_NAME_LENGTH 255

struct {
	byte repl;
	byte allow_empty_blocks;
	byte bytecode;
} flags;
int line_no;	// line number

#define ERROR(...) \
    do { \
        fprintf(stderr, ##__VA_ARGS__); \
        putc('\n', stderr); \
        exit(1); \
    } while (0)

#define log(...) \
    do { \
        fprintf(stderr, "line %d: ", line_no); \
        fprintf(stderr, ##__VA_ARGS__); \
        putc('\n', stderr); \
    } while (0)

/* enums */

typedef enum {
	END,
	NEWLINE,		// used for line number
	PRINT_KW,
	DEF_KW,
	RETURN_KW,
	GLOBAL_KW,
	LOCAL_KW,
	IF_KW,
	WHILE_KW,
	ELSE_KW,
	BLOCK_START,	// colon
	BLOCK_END,		// unindent
	NAME,			// variable
	LITERAL,
	NUMERAL,
	MUL_OP,
	ADD_OP,
	CMP_OP,
	EQL_OP,
	ASG_OP,
	NOT_OP,
	LPAREN,
	RPAREN,
	LBRACKET,
	RBRACKET,
	COMMA,
	SEMICOLON
} TokenType;

typedef enum {
	NONE,
	INTEGER,	// long
	FLOAT,		// double
	STRING,
	FUNCTION
} ObjectType;

typedef enum {
	STOP,
	SET_LINE,			// SET_LINE (int line_no)
	POP,
	PRINT,				// print and pop
	STORE,				// STORE (Symbol s)
	LOAD,				// LOAD (Symbol s)
	LOAD_CONST,			// LOAD_CONST (byte type, ...)
	CALL_FUNCTION,		// CALL_FUNCTION (int argc)
	RETURN_VALUE,
	JUMP,				// JUMP (int target)
	POP_JUMP_IF_TRUE,	// jump and pop
	POP_JUMP_IF_FALSE,
	UNARY_PLUS,
	UNARY_MINUS,
	UNARY_NOT,
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
	ObjectType type;
	void *value;
	int refcount;
} Object;

typedef struct {
	int argc;
	int offset;
} Function;

typedef struct {
	int scope;
	int offset;
} Symbol;

/* prototypes */

void tokenize(FILE *);
void reset_tokens();

void parse();

#define write_string(x) write_bytes(x, strlen(x) + 1)
void write_bytes(void *, int);
void write_byte(byte);
void write_int(int);
void write_int_at(int, int);
void reset_vm();
void run_vm();
void dis();

#define add_symbol(a, b, g) lookup_add_symbol(a, b, 0, 1, g)
#define lookup_symbol(a, b) lookup_add_symbol(a, b, 1, 0, 0)
#define get_symbol(a, b) lookup_add_symbol(a, b, 1, 1, 0)
Symbol lookup_add_symbol(char *, int, byte, byte, byte);

Object *null_object();
Object *new_object(ObjectType, void *);
void collect_object(Object *);

void push_object(Object *);
Object *pop_stack();
Object *peek_stack();

void load_symbol(Symbol *);
void store_symbol(Symbol *);
