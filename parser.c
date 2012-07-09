#include "main.h"

extern char *token_string;
extern Token *tokens;
Token t;

int current_token = 0;
int scope = 0;

void expression();

Token peek()
{
	int i = 0;
	while (tokens[current_token + i].type == NEWLINE)
		i++;
	return tokens[current_token + i];
}

Token next()
{
	while (tokens[current_token++].type == NEWLINE)
		line_no++;
	return tokens[current_token - 1];
}

void unary_expr()
{
	if (t.type == ADD_OP)
	{
		char oper = token_string[t.start];
		t = next();
		unary_expr();
		if (oper == '+')
			vm_writebyte(UNARY_PLUS);
		else if (oper == '-')
			vm_writebyte(UNARY_MINUS);
	}
	else if (t.type == LPAREN)
	{
		t = next();
		expression();
		if (t.type != RPAREN)
			ERROR("SyntaxError: unmatched parenthesis at line %d", line_no);
		t = next();
	}
	else
	{
		char *str = malloc(t.length + 1);
		strncpy(str, token_string + t.start, t.length);
		str[t.length] = '\0';
		switch (t.type)
		{
		case NAME:
			vm_writebyte(LOAD);
			vm_writeint(sym_lookup(str, scope));
			break;
		case NUMERAL:
			vm_writebyte(LOAD_CONST);
			vm_writebyte(INTEGER);
			vm_writeint(atoi(str));
			break;
		case LITERAL:
			vm_writebyte(LOAD_CONST);
			vm_writebyte(STRING);
			vm_write(str, t.length + 1);
			break;
		}
		free(str);
		t = next();
	}
}

void mult_expr()
{
	unary_expr();
	while (t.type == MULT_OP)
	{
		char oper = token_string[t.start];
		t = next();
		unary_expr();
		if (oper == '*')
			vm_writebyte(MULT);
		else if (oper == '/')
			vm_writebyte(DIV);
	}
}

void add_expr()
{
	mult_expr();
	while (t.type == ADD_OP)
	{
		char oper = token_string[t.start];
		t = next();
		mult_expr();
		if (oper == '+')
			vm_writebyte(ADD);
		else if (oper == '-')
			vm_writebyte(SUB);
	}
}

void comp_expr()
{
	add_expr();
	if (t.type == COMP_OP)
	{
		char oper[] = { token_string[t.start], token_string[t.start + t.length - 1] };
		t = next();
		add_expr();
		if (oper[1] == '=')
		{
			if (oper[0] == '>')
				vm_writebyte(GREATER_THAN_EQUAL);
			else if (oper[0] == '<')
				vm_writebyte(LESS_THAN_EQUAL);
			else if (oper[0] == '!')
				vm_writebyte(NOT_EQUAL);
			else
				vm_writebyte(EQUAL);
		}
		else if (oper[0] == '>')
			vm_writebyte(GREATER_THAN);
		else if (oper[0] == '<')
			vm_writebyte(LESS_THAN);
	}
}

void expression()
{
	if (t.type == NAME && peek().type == ASSIGN_OP)
	{
		char oper;

		char *ident = malloc(t.length + 1);
		strncpy(ident, token_string + t.start, t.length);
		ident[t.length] = '\0';
		if ((oper = token_string[(t = next()).start]) != '=')	// compound assignment
		{
			vm_writebyte(LOAD);
			vm_writeint(sym_lookup(ident, scope));
		}
		t = next();
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
		free(ident);
	}	
	else
		comp_expr();
}

void statement()
{
	if (t.type == PRINT_STMT)
	{
		t = next();
		expression();
		vm_writebyte(PRINT);
	}
	else if (t.type != SEMICOLON)
	{
		expression();
		vm_writebyte(POP);
	}
	if (t.type != SEMICOLON)
		ERROR("SyntaxError: unterminated statement at line %d", line_no);
	t = next();	
}

void block()
{
	int offset;
	TokenType tt;
	while (1)
	{
		switch (tt = t.type)
		{
		case IF_STMT:
		case WHILE_STMT:
			t = next();
			expression();
			if (t.type != BLOCK_START)
				ERROR("SyntaxError: expected colon at line %d", line_no);
			if (++scope == MAX_NESTED_BLOCKS)
				ERROR("OverflowError: nested blocks exceed %d at line %d", MAX_NESTED_BLOCKS, line_no);
			vm_writebyte(JUMP_IF_FALSE);
			offset = vm_getoffset();
			vm_writeint(0);
			t = next();
			block();
			if (t.type != BLOCK_END)
				ERROR("SyntaxError: unterminated block at line %d", line_no);
			sym_clear_scope(scope--);
			if (tt == WHILE_STMT)
			{
				vm_writebyte(JUMP);
				vm_writeint(offset - 1);
			}
			vm_writeintat(vm_getoffset(), offset);
			vm_writebyte(POP);
			t = next();
			break;
		case PRINT_STMT:
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
	// printf("%s\n", token_string);
	line_no = 1;
	t = next();
	block();
	if (t.type == END)
		vm_writebyte(STOP);
}