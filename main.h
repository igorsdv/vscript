#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

typedef unsigned char byte;

/* globals */

int options;				// flags and options
enum options {				// bitfields
	flag_repl = 1,
	flag_allow_empty_blocks = 2,
	flag_bytecode = 4,
	option_files = 8
};

int line_no;				// line number (you don't say?)
int repl_mode;				// interactive mode

/* helpers */

#define error(...) fprintf(stderr, ##__VA_ARGS__), fprintf(stderr, " at line %u\n", line_no), exit(1)

#define log(...) fprintf(stderr, ##__VA_ARGS__), putc('\n', stderr)

/* declarations */

struct token {
	enum token_type {
		END,
		NEWLINE,		// used for line number
		PRINT_KW,
		DEF_KW,
		RETURN_KW,
		GLOBAL_KW,
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
	} type;
	int offset;
};

enum opcode {
	RETURN,
	SET_LINE,			// SET_LINE (int line_no)
	POP,
	PRINT,				// print and pop
	STORE,				// STORE (int s)
	LOAD,				// LOAD (int s)
	LOAD_CONST,			// LOAD_CONST (int c)
	CALL_FUNCTION,		// CALL_FUNCTION (int argc)
	JUMP,				// JUMP (int target)
	POP_JUMP_IF_FALSE,	// pop and jump (int target)
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
	DIV,
	MOD
};

/* combine ALL the declarations! */
struct env {							// execution environment (created at runtime)
	struct code {						// static code object (function)
		struct program {				// bytecode
			size_t length;
			byte *array;
		} program;
		struct data {					// constant values
			size_t length;
			struct object {				/* object */
				enum object_type {
					TYPE_NONE,
					TYPE_CODE,
					TYPE_INT,
					TYPE_FLOAT,
					TYPE_STRING
				} type;
				void *value;
				int refcount;
			} **array;
		} data;
		struct symbols {				// variable names
			size_t length;
			struct symbol {				/* symbol */
				char *name;
				enum scope {
					SCOPE_NONLOCAL,		// default
					SCOPE_GLOBAL,
					SCOPE_LOCAL
				} scope;
			} *array;
		} symbols;
		int argc;						// number of arguments taken by function
	} *co;
	struct objects {					// variable values corresponding to names in co->symbols
		size_t length;
		struct object **array;			// array of pointers to object
	} objects;
	struct env *parent;					// parent execution environment (call stack)
	int offset;							// offset in co->program
};

/* prototypes */

void *safe_malloc(size_t);
void *safe_calloc(size_t, size_t);
void *safe_realloc(void *, size_t);

void tokenize(FILE *);
void reset_tokens();

void parse(struct code *);

void write_bytes(struct code *, void *, size_t);
void write_byte(struct code *, byte);
void write_int(struct code *, int);
void clear_program(struct code *);
void dis(struct code *, int *);
void run(struct env *);

int get_symbol(struct code *, char *, enum scope);
int get_const(struct code *, struct object *object);
void load_symbol(struct env *, int);
void store_symbol(struct env *, int);

struct object *new_object(enum object_type, void *);
struct object *null_object();
struct object *make_object(enum object_type, char *);
void gc_ref(struct object *);
void gc_deref(struct object *);
void gc_collect(struct object *);

void push_object(struct object *);
struct object *pop_stack();
struct object *peek_stack();

int bool_value(struct object *);
int compare(struct object *, struct object *);
struct object *add(struct object *, struct object *);
struct object *subtract(struct object *, struct object *);
struct object *multiply(struct object *, struct object *);
struct object *divide(struct object *, struct object *);
struct object *modulo(struct object *, struct object *);
