#include "main.h"

#define write_byte(value) write_byte(co, value)
#define write_int(value) write_int(co, value)
#define write_int_at(value, offset) (*(int *)(co->program.array + offset) = value)
#define write_string(str) write_bytes(co, str, strlen(str) + 1)
#define get_symbol(name, scope) get_symbol(co, name, scope)
#define get_const(object) get_const(co, object)
#define get_offset() (co->program.length)

struct code *co;
int offset;
enum token_type token_type, next_token_type;

void read_token()
{
	while ((token_type = get_token_type(++offset)) == NEWLINE)
	{
		write_byte(SET_LINE);
		write_int(++line_no);
	}
}

void peek_token()
{
	int i = offset;
	while ((next_token_type = get_token_type(++i)) == NEWLINE);
}

/* from the because-I-feel-like-it dep't */
#define token_value (char *)(get_token_value(offset))
#define next_token_type (peek_token(), next_token_type)

void expression();
void block();

void unr_expr()
{
	if (token_type == ADD_OP)
	{
		char oper = *token_value;

		read_token();
		unr_expr();

		if (oper == '+')
			write_byte(UNARY_PLUS);
		else if (oper == '-')
			write_byte(UNARY_MINUS);
	}
	else if (token_type == LPAREN)
	{
		read_token();

		if (token_type == RPAREN)
		{
			write_byte(LOAD_CONST);
			write_int(get_const(null_object()));
		}
		else
			expression();

		if (token_type != RPAREN)
			error("SyntaxError: unmatched parenthesis");
		read_token();
	}
	else if (token_type == NAME)
	{
		int s = get_symbol(token_value, SCOPE_NONLOCAL);
		
		read_token();

		if (token_type == LPAREN)
		{
			int argc = 0;

			if (next_token_type != RPAREN)
			{
				do {
					read_token();
					expression();
					argc++;
				} while (token_type == COMMA);
			}
			else
				read_token();

			write_byte(LOAD);
			write_int(s);

			write_byte(CALL_FUNCTION);
			write_int(argc);

			if (token_type != RPAREN)
				error("SyntaxError: unterminated function call");
			read_token();
		}
		else
		{
			write_byte(LOAD);
			write_int(s);
		}
	}
	else if (token_type == NUMERAL)
	{
		char *string = token_value;

		write_byte(LOAD_CONST);
		write_int(get_const(make_object(strchr(string, '.') ? TYPE_FLOAT : TYPE_INT, string)));

		read_token();
	}
	else if (token_type == LITERAL)
	{
		write_byte(LOAD_CONST);
		write_int(get_const(make_object(TYPE_STRING, token_value)));

		read_token();
	}
	else
		error("SyntaxError: unterminated expression");
}

void not_expr()
{
	if (token_type == NOT_OP)
	{
		read_token();
		not_expr();

		write_byte(UNARY_NOT);
	}
	else
		unr_expr();
}

void mul_expr()
{
	not_expr();

	while (token_type == MUL_OP)
	{
		char oper = *token_value;

		read_token();
		not_expr();

		if (oper == '*')
			write_byte(MULT);
		else if (oper == '/')
			write_byte(DIV);
	}
}

void add_expr()
{
	mul_expr();

	while (token_type == ADD_OP)
	{
		char oper = *token_value;

		read_token();
		mul_expr();

		if (oper == '+')
			write_byte(ADD);
		else if (oper == '-')
			write_byte(SUB);
	}
}

void cmp_expr()
{
	add_expr();

	if (token_type == CMP_OP)
	{
		char *oper = token_value;

		read_token();
		add_expr();

		if (oper[1] == '=')
		{
			if (*oper == '>')
				write_byte(GREATER_THAN_EQUAL);
			else if (*oper == '<')
				write_byte(LESS_THAN_EQUAL);
		}
		else if (*oper == '>')
			write_byte(GREATER_THAN);
		else if (*oper == '<')
			write_byte(LESS_THAN);
	}
}

void eql_expr()
{
	cmp_expr();

	if (token_type == EQL_OP)
	{
		char oper = *token_value;

		read_token();
		cmp_expr();

		if (oper == '=')
			write_byte(EQUAL);
		else if (oper == '!')
			write_byte(NOT_EQUAL);
	}
}

void assignment()
{
	char oper, *name = token_value;
	
	read_token();
	oper = *token_value;

	if (oper != '=')	// compound assignment
	{
		write_byte(LOAD);
		write_int(get_symbol(name, SCOPE_NONLOCAL));
	}

	read_token();
	expression();

	if (oper != '=')
	{
		if (oper == '+')
			write_byte(ADD);
		else if (oper == '-')
			write_byte(SUB);
		else if (oper == '*')
			write_byte(MULT);
		else if (oper == '/')
			write_byte(DIV);
	}

	write_byte(STORE);
	write_int(get_symbol(name, SCOPE_LOCAL));
}

void expression()
{

	if (token_type == NAME && next_token_type == ASG_OP)
		assignment();
	else
		eql_expr();
}

void statement()
{
	enum token_type kw_type = token_type;

	if (kw_type == GLOBAL_KW)
	{
		do {
			read_token();
			if (token_type != NAME)
				error("SyntaxError: invalid declaration on line %d", line_no);
			(void) get_symbol(token_value, SCOPE_GLOBAL);
			read_token();
		} while (token_type == COMMA);
	}
	else
	{
		if (kw_type == PRINT_KW || kw_type == RETURN_KW)
			read_token();
		
		if (token_type == SEMICOLON)
		{
			write_byte(LOAD_CONST);
			write_byte(get_const(null_object()));
		}
		else
			expression();

		if (kw_type == PRINT_KW)
			write_byte(PRINT);
		else if (kw_type == RETURN_KW)
			write_byte(RETURN);
		else
			write_byte(POP);
	}

	if (token_type != SEMICOLON)
		error("SyntaxError: invalid statement");
	read_token();	
}

void name_list(int *argc)
{
	/*	The argument list is sent to the VM as STORE instructions in reverse 
		order, so that a function call may be preceded by LOAD instructions in 
		forward order.
	*/

	int s;

	if (token_type != NAME)
		error("SyntaxError: invalid function argument list");
	
	s = get_symbol(token_value, SCOPE_LOCAL);
	read_token();

	if (token_type == COMMA)
	{
		read_token();
		name_list(argc);
	}

	write_byte(STORE);
	write_int(s);
	write_byte(POP);

	(*argc)++;
}

void function()
{
	/*	A function represents a code object. */

	struct code *fco = safe_calloc(1, sizeof *fco);
	struct code *parent = co;

	read_token();				// consume DEF_KW
	if (token_type != NAME)
		error("SyntaxError: invalid function identifier");
	
	write_byte(LOAD_CONST);
	write_int(get_const(new_object(TYPE_CODE, fco)));

	write_byte(STORE);
	write_int(get_symbol(token_value, SCOPE_LOCAL));
	write_byte(POP);

	co = fco;

	read_token();
	if (token_type != LPAREN)
		error("SyntaxError: invalid function definition");

	read_token();
	if (token_type != RPAREN)
		name_list(&fco->argc);
	if (token_type != RPAREN)
		error("SyntaxError: unterminated function definition");

	read_token();
	if (token_type != BLOCK_START)
		error("SyntaxError: missing colon");

	read_token();
	if (!(options & flag_allow_empty_blocks) && token_type == BLOCK_END)
		error("SyntaxError: empty function block");

	while (token_type != BLOCK_END)
		block();

	write_byte(LOAD_CONST);
	write_int(get_const(null_object()));

	write_byte(RETURN);

	co = parent;

	read_token();
}

void block()
{
	int jump_ptr, target, while_start;
	enum token_type kw_type = token_type;
	
	switch (kw_type)
	{
	case IF_KW:
	case WHILE_KW:
		read_token();

		if (kw_type == WHILE_KW)
			while_start = get_offset();

		expression();

		if (token_type != BLOCK_START)
			error("SyntaxError: missing colon");

		write_byte(POP_JUMP_IF_FALSE);
		jump_ptr = get_offset();
		write_int(0);

		read_token();
		if (!(options & flag_allow_empty_blocks) && token_type == BLOCK_END)
			error("SyntaxError: empty block");	

		while (token_type != BLOCK_END)
			block();

		if (kw_type == WHILE_KW)
		{
			write_byte(JUMP);
			write_int(while_start);
		}

		target = get_offset();

		read_token();

		if (token_type == ELSE_KW && kw_type == IF_KW)
		{
			int else_jump_ptr;

			write_byte(JUMP);
			else_jump_ptr = get_offset();
			write_int(0);

			target = get_offset();

			read_token();
			if (token_type != BLOCK_START)
				error("SyntaxError: missing colon");

			read_token();
			if (!(options & flag_allow_empty_blocks) && token_type == BLOCK_END)
				error("SyntaxError: empty block");

			while (token_type != BLOCK_END)
				block();

			write_int_at(get_offset(), else_jump_ptr);
			
			read_token();
		}

		write_int_at(target, jump_ptr);
		break;
	case DEF_KW:
		function();
		break;
	case PRINT_KW:
	case RETURN_KW:
	case GLOBAL_KW:
	case NAME:
	case LITERAL:
	case NUMERAL:
	case LPAREN:
	case NOT_OP:
	case ADD_OP:
	case SEMICOLON:
		statement();
		break;
	default:
		error("SyntaxError: unterminated block");
	}
}

void parse(struct code *main_co)
{
	int orig_line_no = line_no;

	co = main_co;

	line_no = 0;
	offset = -1;
	read_token();

	if (repl_mode)
	{
		if (token_type == IF_KW || token_type == WHILE_KW || token_type == DEF_KW)
			block();
		else
			while (token_type != END)
			{
				if (token_type == IF_KW || token_type == WHILE_KW || token_type == DEF_KW)
					break;
				statement();
			}

		if (token_type != END)
			error("ReplError: invalid statement or block");
	}
	else
		while (token_type != END)
			block();

	write_byte(RETURN);
	line_no = orig_line_no;
}
