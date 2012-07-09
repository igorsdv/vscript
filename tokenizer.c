#include "main.h"
#define SIZE_INCREMENT 1024
#define TOKENS_SIZE_INCREMENT 256
#define MAX_INDENT_LENGTH 255

char *token_string;
int string_size = SIZE_INCREMENT;
int string_length = 0;

Token *tokens;
int tokens_size = TOKENS_SIZE_INCREMENT;
int tokens_length = 0;

int line_no = 1;

int parens = 0;
int blocks = 0;
char indent_string[MAX_INDENT_LENGTH + 1] = "";
int block_indent[MAX_INDENT_LENGTH + 1];

void tokens_init()
{
	token_string = malloc(string_size);
	tokens = malloc(tokens_size * sizeof(Token));
}

void tokens_free()
{
	free(token_string);
	free(tokens);
}

void append(char c)
{
	if (string_length == string_size)
	{
		string_size += SIZE_INCREMENT;
		token_string = realloc(token_string, string_size);
	}
	token_string[string_length++] = c;
}

void add_token(TokenType type, int start)
{
	Token token;
	token.type = type;
	if (type == NEWLINE)
		line_no++;
	else if (type == LPAREN)
		parens++;
	else if (type == RPAREN)
		parens--;
	else if (type == BLOCK_START)
		blocks++;
	else if (type == BLOCK_END)
		blocks--;
	token.start = start;
	token.length = string_length - start;
	if (tokens_length == tokens_size)
	{
		tokens_size += TOKENS_SIZE_INCREMENT;
		tokens = realloc(tokens, tokens_size * sizeof(Token));
	}
	tokens[tokens_length++] = token;
}

void tokenize(FILE *f)
{
	int c, position;
	int new_line = 1;			// used for indentation
	int new_indent = 1;			// after BLOCK_START

	while ((c = fgetc(f)) != EOF)
	{
		position = string_length;
		if (new_line)
		{
			int i = -1;
			while (c == indent_string[++i])
				c = fgetc(f);
			if (i < strlen(indent_string))		// unindent: end block(s)
			{
				indent_string[i] = '\0';
				add_token(SEMICOLON, position);
				while (block_indent[blocks] > i)
					add_token(BLOCK_END, position);
			}
			else
			{
				while (c == ' ' || c == '\t')	// new indent
				{
					if (!new_indent)
						ERROR("SyntaxError: unexpected indentation on line %d", line_no);
					if (i >= MAX_INDENT_LENGTH)
						ERROR("SyntaxError: indentation exceeds %d characters on line %d", MAX_INDENT_LENGTH, line_no);
					indent_string[i++] = c;
					c = fgetc(f);
				}
				indent_string[i] = '\0';
				block_indent[blocks] = i;
				new_indent = 0;
			}
			new_line = 0;
		}
		if (isspace(c))
		{
			if (c == '\n')
			{
				new_line = 1;
				// multi-line expressions must be in parentheses
				if (!parens && tokens[tokens_length - 1].type != BLOCK_START && tokens[tokens_length - 1].type != SEMICOLON)
					add_token(SEMICOLON, position);
				add_token(NEWLINE, position);
				if (i_mode)
					break;
			}
		}
		else if (c == '(')
			add_token(LPAREN, position);
		else if (c == ')')
			add_token(RPAREN, position);
		else if (c == ';')
			add_token(SEMICOLON, position);
		else if (c == ':')
		{
			add_token(BLOCK_START, position);
			if (new_indent)
				ERROR("SyntaxError: illegal colon at line %d", line_no);
			new_indent = 1;
		}
		else if (isalpha(c) || c == '_' || c == '$')
		{
			append(c);
			while (isalnum(c = getc(f)) || c == '_' || c == '$')
				append(c);
			ungetc(c, f);

			if (string_length - position == 5 && !strncmp(token_string + position, "print", 5))
				add_token(PRINT_STMT, position);
			else if (string_length - position == 2 && !strncmp(token_string + position, "if", 2))
				add_token(IF_STMT, position);
			else if (string_length - position == 5 && !strncmp(token_string + position, "while", 5))
				add_token(WHILE_STMT, position);
			else
				add_token(NAME, position);
		}
		else if (c == '"')
		{
			while ((c = getc(f)) != '"')
			{
				if (c == EOF || c == '\n')
					ERROR("SyntaxError: unterminated string at line %d", line_no);
				if (c == '\\')
				{
					switch (c = getc(f))
					{
					case '\n':	// actual linebreak
						add_token(NEWLINE, position);
					case '\\':
					case '"':
						append(c);
						break;
					case 'n':
						append('\n');
						break;
					case 't':
						append('\t');
						break;
					// etc
					case EOF:
						ERROR("SyntaxError: unterminated string at line %d", line_no);
					default:
						append('\\');
						append(c);
					}
				}
				else
					append(c);
			}
			add_token(LITERAL, position);
		}
		else if (c == '/')
		{
			if ((c = getc(f)) == '/')	// single-line comment
			{
				while ((c = getc(f)) != '\n' && c != EOF) ;
				ungetc(c, f);
			}
			else if (c == '*')			// multi-line comment
			{
				while ((c = getc(f)) != '*' || (c = getc(f)) != '/')
				{
					if (c == EOF)
						ERROR("SyntaxError: unterminated comment at line %d", line_no);
					if (c == '\n')
						add_token(NEWLINE, position);
				}
			}
			else						// division
			{
				ungetc(c, f);
				c = '/';
				goto division;
			}
		}
		else if (isdigit(c))
		{
			append(c);
			while (isdigit(c = fgetc(f)))
				append(c);
			ungetc(c, f);
			add_token(NUMERAL, position);
		}
		else if (c == '=')
		{
			append(c);
			if ((c = getc(f)) == '=')
			{
				append(c);
				add_token(COMP_OP, position);
			}
			else
			{
				add_token(ASSIGN_OP, position);
				ungetc(c, f);
			}
		}
		else if (c == '!')
		{
			append(c);
			if ((c = getc(f)) != '=')
				ERROR("SyntaxError: unrecognized character <!> at line %d", line_no);
			append(c);
			add_token(COMP_OP, position);
		}
		else if (c == '<' || c == '>')
		{
			append(c);
			if ((c = getc(f)) == '=')
				append(c);
			else
				ungetc(c, f);
			add_token(COMP_OP, position);
		}
		else if (c == '+' || c == '-')
		{
			append(c);
			if ((c = getc(f)) == '=')
			{
				append(c);
				add_token(ASSIGN_OP, position);
			}
			else
			{
				ungetc(c, f);
				add_token(ADD_OP, position);
			}
		}
		else if (c == '*' || c == '/')
		{
division:
			append(c);
			if ((c = getc(f)) == '=')
			{
				append(c);
				add_token(ASSIGN_OP, position);
			}
			else
			{
				ungetc(c, f);
				add_token(MULT_OP, position);
			}
		}
		else
			ERROR("SyntaxError: unrecognized character <%c> at line %d", c, line_no);
	}
}

void tokenize_file(char *source)
{
	FILE *f = fopen(source, "r");
	if (f == NULL)
		ERROR("FileError: could not open file %s", source);
	tokenize(f);
	if (tokens[tokens_length - 1].type != SEMICOLON)
		add_token(SEMICOLON, string_length);
	while (blocks)
		add_token(BLOCK_END, string_length);
	add_token(END, string_length);
}

void tokenize_input()
{
	printf(">>> ");
	tokenize(stdin);
	while ((parens || blocks) && !feof(stdin))
	{
		printf("... ");
		tokenize(stdin);
	}
	add_token(END, string_length);
}