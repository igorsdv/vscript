#include "main.h"

struct {
	int offset;
	int scope;
	TokenType token_type;
} p;

TokenType next_token_type()
{
	TokenType t;
	int i = p.offset;
	while ((t = get_token_type(++i)) == NEWLINE);
	return t;
}

void read_token()
{
	while ((p.token_type = get_token_type(++p.offset)) == NEWLINE)
	{
		write_byte(SET_LINE);
		write_int(++line_no);
	}
}

char *token_value()
{
	return (char *)get_token_value(p.offset);
}

void expression();
void block();

void unr_expr()
{
	if (p.token_type == ADD_OP)
	{
		char oper = *token_value();
		read_token();
		unr_expr();
		if (oper == '+')
			write_byte(UNARY_PLUS);
		else if (oper == '-')
			write_byte(UNARY_MINUS);
	}
	else if (p.token_type == LPAREN)
	{
		read_token();
		if (p.token_type == RPAREN)
		{
			write_byte(LOAD_CONST);
			write_byte(NONE);
		}
		else
			expression();
		if (p.token_type != RPAREN)
			ERROR("SyntaxError: unmatched parenthesis at line %d", line_no);
		read_token();
	}
	else if (p.token_type == NAME)
	{
		Symbol s = lookup_symbol(token_value(), p.scope);
		read_token();
		if (p.token_type == LPAREN)
		{
			int argc = 0;
			if (next_token_type() != RPAREN)
			{
				do {
					read_token();
					expression();
					argc++;
				} while (p.token_type == COMMA);
			}
			else
				read_token();
			write_byte(LOAD);
			write_bytes(&s, sizeof(Symbol));
			write_byte(CALL_FUNCTION);
			write_int(argc);
			if (p.token_type != RPAREN)
				ERROR("SyntaxError: unterminated function call at line %d", line_no);
			read_token();
		}
		else
		{
			write_byte(LOAD);
			write_bytes(&s, sizeof(Symbol));
		}
	}
	else if (p.token_type == NUMERAL)
	{
		char *str = token_value();
		write_byte(LOAD_CONST);
		write_byte(strchr(str, '.') ? FLOAT : INTEGER);
		write_string(str);
		read_token();
	}
	else if (p.token_type == LITERAL)
	{
		write_byte(LOAD_CONST);
		write_byte(STRING);
		write_string(token_value());
		read_token();
	}
	else
		ERROR("SyntaxError: unterminated expression at line %d", line_no);
}

void not_expr()
{
	if (p.token_type == NOT_OP)
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
	while (p.token_type == MUL_OP)
	{
		char oper = *token_value();
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
	while (p.token_type == ADD_OP)
	{
		char oper = *token_value();
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
	if (p.token_type == CMP_OP)
	{
		char *oper = token_value();
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
	if (p.token_type == EQL_OP)
	{
		char oper = *token_value();
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
	Symbol s;
	char oper, *str = token_value();
	read_token();
	oper = *token_value();
	if (oper != '=')	// compound assignment
	{
		s = lookup_symbol(str, p.scope);
		write_byte(LOAD);
		write_bytes(&s, sizeof(Symbol));
	}
	else
		s = get_symbol(str, p.scope);
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
	write_bytes(&s, sizeof(Symbol));
}

void expression()
{

	if (p.token_type == NAME && next_token_type() == ASG_OP)
		assignment();
	else
		eql_expr();
}

void statement()
{
	TokenType kw_type = p.token_type;

	if (kw_type == GLOBAL_KW || kw_type == LOCAL_KW)
	{
		byte global = kw_type == GLOBAL_KW;
		do {
			read_token();
			if (p.token_type != NAME)
				ERROR("SyntaxError: invalid declaration on line %d", line_no);
			(void)add_symbol(token_value(), p.scope, global);
			read_token();
		} while (p.token_type == COMMA);
	}
	else
	{
		if (kw_type == PRINT_KW || kw_type == RETURN_KW)
			read_token();
		if (p.token_type == SEMICOLON)
		{
			write_byte(LOAD_CONST);
			write_byte(NONE);
		}
		else
			expression();
		if (kw_type == PRINT_KW)
			write_byte(PRINT);
		else if (kw_type == RETURN_KW)
			write_byte(RETURN_VALUE);
		else
			write_byte(POP);
	}
	if (p.token_type != SEMICOLON)
		ERROR("SyntaxError: invalid statement at line %d", line_no);
	read_token();	
}

void name_list(int *argc)
{
	/*	The argument list is sent to the VM as STORE instructions in reverse 
		order, so that a function call may be preceded by LOAD instructions in 
		forward order.
	*/

	Symbol s;

	if (p.token_type != NAME)
		ERROR("SyntaxError: invalid function argument list at line %d", line_no);
	s = add_symbol(token_value(), p.scope, 0);
	read_token();
	if (p.token_type == COMMA)
	{
		read_token();
		name_list(argc);
	}
	write_byte(STORE);
	write_bytes(&s, sizeof(Symbol));
	(*argc)++;
}

void function()
{
	int argc_ptr, offset_ptr, jump_ptr;
	int argc = 0;
	Symbol s;

	read_token();	// "def"
	if (p.token_type != NAME)
		ERROR("SyntaxError: invalid function identifier at line %d", line_no);
	s = get_symbol(token_value(), p.scope);
	write_byte(LOAD_CONST);
	write_byte(FUNCTION);
	argc_ptr = get_offset();
	write_int(0);
	offset_ptr = get_offset();
	write_int(0);
	write_byte(STORE);
	write_bytes(&s, sizeof(Symbol));
	write_byte(JUMP);
	jump_ptr = get_offset();
	write_int(0);		
	write_int_at(get_offset(), offset_ptr);
	add_scope(++p.scope);
	read_token();
	if (p.token_type != LPAREN)
		ERROR("SyntaxError: invalid function definition at line %d", line_no);
	read_token();
	if (p.token_type != RPAREN)
		name_list(&argc);
	if (p.token_type != RPAREN)
		ERROR("SyntaxError: unterminated function definition at line %d", line_no);
	write_int_at(argc, argc_ptr);
	read_token();
	if (p.token_type != BLOCK_START)
		ERROR("SyntaxError: missing colon at line %d", line_no);
	read_token();
	if (!flags.allow_empty_blocks && p.token_type == BLOCK_END)
		ERROR("SyntaxError: empty function block at line %d", line_no);
	while (p.token_type != BLOCK_END)
		block();
	write_byte(LOAD_CONST);		// in case there was no return statement
	write_byte(NONE);
	write_byte(RETURN_VALUE);
	write_int_at(get_offset(), jump_ptr);
	read_token();
	reset_scope(p.scope--);
}

void block()
{
	int jump_ptr, target, while_start;
	TokenType kw_type = p.token_type;
	
	switch (kw_type)
	{
	case IF_KW:
	case WHILE_KW:
		read_token();
		if (kw_type == WHILE_KW)
			while_start = get_offset();
		expression();
		if (p.token_type != BLOCK_START)
			ERROR("SyntaxError: missing colon at line %d", line_no);
		write_byte(POP_JUMP_IF_FALSE);
		jump_ptr = get_offset();
		write_int(0);
		read_token();
		if (!flags.allow_empty_blocks && p.token_type == BLOCK_END)
			ERROR("SyntaxError: empty block at line %d", line_no);	
		while (p.token_type != BLOCK_END)
			block();
		if (kw_type == WHILE_KW)
		{
			write_byte(JUMP);
			write_int(while_start);
		}
		target = get_offset();
		read_token();
		if (p.token_type == ELSE_KW && kw_type == IF_KW)
		{
			int else_jump_ptr;

			write_byte(JUMP);
			else_jump_ptr = get_offset();
			write_int(0);
			target = get_offset();
			read_token();
			if (p.token_type != BLOCK_START)
				ERROR("SyntaxError: missing colon at line %d", line_no);
			read_token();
			if (!flags.allow_empty_blocks && p.token_type == BLOCK_END)
				ERROR("SyntaxError: empty block at line %d", line_no);
			while (p.token_type != BLOCK_END)
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
	case LOCAL_KW:
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
		ERROR("SyntaxError: unterminated block at line %d", line_no);
	}
}

void parse()
{
	line_no = 0;
	p.offset = -1;
	add_scope(p.scope = 0);
	read_token();
	if (flags.repl)
	{
		if (p.token_type == IF_KW || p.token_type == WHILE_KW || p.token_type == DEF_KW)
			block();
		else
			while (p.token_type != END)
			{
				if (p.token_type == IF_KW || p.token_type == WHILE_KW || p.token_type == DEF_KW)
					break;
				statement();
			}
		if (p.token_type != END)
			ERROR("ReplError: invalid statement or block at line %d", line_no);
	}
	else
		while (p.token_type != END)
			block();
	write_byte(STOP);
	reset_scope();
}
