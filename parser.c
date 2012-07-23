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
			vm_writebyte(strchr(str, '.') ? FLOAT : INTEGER);
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

void not_expr()
{
	if (token_type == NOT_OP)
	{
		read_token();
		not_expr();
		vm_writebyte(UNARY_NOT);
	}
	else
		unr_expr();
}

void mul_expr()
{
	not_expr();
	while (token_type == MUL_OP)
	{
		char oper = *token_value();
		read_token();
		not_expr();
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
	switch (token_type)
	{
	case NAME:
		if (next_token_type() == ASG_OP)
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
			break;
		}
	case LPAREN:
	case LITERAL:
	case NUMERAL:
	case ADD_OP:
	case NOT_OP:
		eql_expr();
		break;
	default:
		vm_writebyte(LOAD_CONST);
		vm_writebyte(NONE);
	}
}

void statement()
{
	if (token_type == PRINT_KW)
	{
		read_token();
		expression();
		vm_writebyte(PRINT);
	}
	else if (token_type == RETURN_KW)
	{
		read_token();
		expression();
		vm_writebyte(POP); // vm_writebyte(RETURN);
	}
	else
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
	read_token();
	name_list();
	if (token_type != RPAREN)
		ERROR("SyntaxError: unterminated function definition at line %d", line_no);
	read_token();
	if (token_type != BLOCK_START)
		ERROR("SyntaxError: missing colon at line %d", line_no);
	read_token();
	// handle function definition
	while (token_type != BLOCK_END)
		block();
	read_token();
}

void block()
{
	TokenType kw_type;
	int offset, target;
	int while_start;
	switch (kw_type = token_type)
	{
	case IF_KW:
	case WHILE_KW:
		read_token();
		while_start = vm_getoffset();
		expression();
		if (token_type != BLOCK_START)
			ERROR("SyntaxError: missing colon at line %d", line_no);
		vm_writebyte(POP_JUMP_IF_FALSE);
		offset = vm_getoffset();
		vm_writeint(0);
		read_token();		
		while (token_type != BLOCK_END)
			block();
		if (kw_type == WHILE_KW)
		{
			// re-evaluate the condition in the while loop
			vm_writebyte(JUMP);
			vm_writeint(while_start);
		}
		read_token();
		if (token_type == ELSE_KW && kw_type == IF_KW)
		{
			int else_offset;
			vm_writebyte(JUMP);
			else_offset = vm_getoffset();
			vm_writeint(0);
			target = vm_getoffset();
			read_token();
			if (token_type != BLOCK_START)
				ERROR("SyntaxError: missing colon at line %d", line_no);
			read_token();
			while (token_type != BLOCK_END)
				block();
			vm_writeintat(vm_getoffset(), else_offset);
			read_token();
		}
		else
			target = vm_getoffset();
		vm_writeintat(target, offset);
		break;
	case DEF_KW:
		function();
		break;
	case PRINT_KW:
	case RETURN_KW:
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
	offset = -1;
	line_no = 1;
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
			ERROR("ReplError: invalid statement or block at line %d", line_no);
	}
	else
		while (token_type != END)
			block();
	vm_writebyte(STOP);
}
