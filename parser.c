#include "main.h"

int offset;
int scope = 0; //temp
TokenType token_type;

TokenType next_token_type()
{
	TokenType t;
	int i = offset;
	while ((t = get_token_type(++i)) == NEWLINE);
	return t;
}

void read_token()
{
	while ((token_type = get_token_type(++offset)) == NEWLINE)
	{
		vm_writebyte(SET_LINE);
		vm_writeint(++line_no);
	}
}

char *token_value()
{
	return (char *)get_token_value(offset);
}

void expression();
void block();

void name_list()
{
	if (token_type != NAME)
		return;
	// put on stack
	read_token();
	while (token_type == COMMA)
	{
		read_token();
		if (token_type != NAME)
			ERROR("SyntaxError: unterminated name list at line %d", line_no);
		// put on stack
		read_token();
	}
}

void unr_expr()
{
	if (token_type == ADD_OP)
	{
		char oper = *token_value();
		read_token();
		unr_expr();
		if (oper == '+')
			vm_writebyte(UNARY_PLUS);
		else if (oper == '-')
			vm_writebyte(UNARY_MINUS);
	}
	else if (token_type == LPAREN)
	{
		read_token();
		expression();
		if (token_type != RPAREN)
			ERROR("SyntaxError: unmatched parenthesis at line %d", line_no);
		read_token();
	}
	else
	{
		char *str = token_value();
		if (token_type == NAME)
		{
			vm_writebyte(LOAD);
			vm_writeint(sym_lookup(str, scope));
			read_token();
			if (token_type == LPAREN)
			{
				read_token();
				name_list();
				// call function
				if (token_type != RPAREN)
					ERROR("SyntaxError: unterminated function call at line %d", line_no);
				read_token();
			}
		}
		else if (token_type == NUMERAL)
		{
			vm_writebyte(LOAD_CONST);
			vm_writebyte(NUMBER);
			vm_write(str, strlen(str) + 1);
			read_token();
		}
		else if (token_type == LITERAL)
		{
			vm_writebyte(LOAD_CONST);
			vm_writebyte(STRING);
			vm_write(str, strlen(str) + 1);
			read_token();
		}
	}
}

void mul_expr()
{
	unr_expr();
	while (token_type == MUL_OP)
	{
		char oper = *token_value();
		read_token();
		unr_expr();
		if (oper == '*')
			vm_writebyte(MULT);
		else if (oper == '/')
			vm_writebyte(DIV);
	}
}

void add_expr()
{
	mul_expr();
	while (token_type == ADD_OP)
	{
		char oper = *token_value();
		read_token();
		mul_expr();
		if (oper == '+')
			vm_writebyte(ADD);
		else if (oper == '-')
			vm_writebyte(SUB);
	}
}

void cmp_expr()
{
	add_expr();
	if (token_type == CMP_OP)
	{
		char *oper = token_value();
		read_token();
		add_expr();
		if (oper[1] == '=')
		{
			if (*oper == '>')
				vm_writebyte(GREATER_THAN_EQUAL);
			else if (*oper == '<')
				vm_writebyte(LESS_THAN_EQUAL);
		}
		else if (*oper == '>')
			vm_writebyte(GREATER_THAN);
		else if (*oper == '<')
			vm_writebyte(LESS_THAN);
	}
}

void eql_expr()
{
	cmp_expr();
	if (token_type == EQL_OP)
	{
		char oper = *token_value();
		read_token();
		cmp_expr();
		if (oper == '=')
			vm_writebyte(EQUAL);
		else if (oper == '!')
			vm_writebyte(NOT_EQUAL);
	}
}

void expression()
{
	if (token_type == NAME && next_token_type() == ASG_OP)
	{
		char *ident, oper;

		ident = token_value();
		read_token();
		oper = *token_value();
		if (oper != '=')	// compound assignment
		{
			vm_writebyte(LOAD);
			vm_writeint(sym_lookup(ident, scope));
		}
		read_token();
		expression();
		if (oper != '=')
		{
			if (oper == '+')
				vm_writebyte(ADD);
			else if (oper == '-')
				vm_writebyte(SUB);
			else if (oper == '*')
				vm_writebyte(MULT);
			else if (oper == '/')
				vm_writebyte(DIV);
		}
		vm_writebyte(STORE);
		vm_writeint(sym_addlookup(ident, scope));
	}	
	else
		eql_expr();
}

void statement()
{
	if (token_type == PRINT_KW)
	{
		read_token();
		expression();
		vm_writebyte(PRINT);
	}
	else if (token_type != SEMICOLON)
	{
		expression();
		vm_writebyte(POP);
	}
	if (token_type != SEMICOLON)
		ERROR("SyntaxError: unterminated statement at line %d", line_no);
	read_token();	
}

void function()
{
	read_token();
	if (token_type != NAME)
		ERROR("SyntaxError: invalid function identifier at line %d", line_no);
	// add symbol
	read_token();
	if (token_type != LPAREN)
		ERROR("SyntaxError: invalid function definition at line %d", line_no);
	name_list();
	if (token_type != RPAREN)
		ERROR("SyntaxError: unterminated function definition at line %d", line_no);
	read_token();
	if (token_type != BLOCK_START)
		ERROR("SyntaxError: missing colon at line %d", line_no);
	// handle function definition
	block();
	if (token_type != BLOCK_END)
		ERROR("SyntaxError: unterminated block at line %d", line_no);
	read_token();
}

void block()
{
	TokenType kw_type;
	int offset, target;
	while (1)
	{
		switch (kw_type = token_type)
		{
		case IF_KW:
		case WHILE_KW:
			read_token();
			expression();
			if (token_type != BLOCK_START)
				ERROR("SyntaxError: missing colon at line %d", line_no);
			vm_writebyte(JUMP_IF_FALSE);
			offset = vm_getoffset();
			vm_writeint(0);
			read_token();
			block();
			if (token_type != BLOCK_END)	// can this even happen
				ERROR("SyntaxError: unterminated block at line %d", line_no);
			read_token();
			if (kw_type == WHILE_KW)
			{
				vm_writebyte(JUMP);
				vm_writeint(offset - 1);
				target = vm_getoffset();
			}
			else if (token_type == ELSE_KW)
			{
				int else_offset;
				vm_writebyte(JUMP);
				else_offset = vm_getoffset();
				vm_writeint(0);
				target = vm_getoffset();
				read_token();
				if (token_type != BLOCK_START)
					ERROR("SyntaxError: missing colon at line %d", line_no);
				block();
				if (token_type != BLOCK_END)
					ERROR("SyntaxError: unterminated block at line %d", line_no);
				vm_writeintat(vm_getoffset(), else_offset);
				read_token();
			}
			else
				target = vm_getoffset();
			vm_writeintat(target, offset);
			vm_writebyte(POP);
			break;
		case DEF_KW:
			function();
			break;
		case PRINT_KW:
		case NAME:
		case LITERAL:
		case NUMERAL:
		case LPAREN:
		case ADD_OP:
		case SEMICOLON:
			statement();
			break;
		default:
			return;	
		}
	}
}

void parse()
{
	offset = -1;
	line_no = 1;
	read_token();
	block();
	if (token_type != END)
		ERROR("SyntaxError: invalid syntax at line %d", line_no);
	vm_writebyte(STOP);
}
