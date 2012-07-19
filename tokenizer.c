#include "main.h"
#define TOKEN_STRING_ALLOC_SIZE 1024
#define TOKEN_ARRAY_ALLOC_SIZE 256
#define INDENT_STRING_ALLOC_SIZE 256

typedef struct {
	TokenType type;
	int offset;
} Token;

static struct {
	char *value;
	int size;
	int length;
} string = { 0, 0, 0 };

static struct {
	Token *array;
	int size;
	int length;
} tokens = { 0, 0, 0 };

static struct {
	char *string;
	int *offsets;		// offset in string of end of indent for each block
	int size;
} indent = { 0, 0, 0 };

int line_no = 1;

void set_indent(int index, char c)
{
	while (index >= indent.size)
	{
		indent.size += INDENT_STRING_ALLOC_SIZE;
		indent.string = realloc(indent.string, indent.size);
		indent.offsets = realloc(indent.offsets, indent.size * sizeof(int));
	}
	indent.string[index] = c;
}

char *tkns[] = { "END", "NEWLINE", "PRINT_KW", "DEF_KW", "GLOBAL_KW", "IF_KW", "WHILE_KW", "ELSE_KW", "BLOCK_START", "BLOCK_END", "NAME", "LITERAL", "NUMERAL", "MUL_OP", "ADD_OP", "CMP_OP", "EQL_OP", "ASG_OP", "LPAREN", "RPAREN", "LBRACKET", "RBRACKET", "COMMA", "SEMICOLON" };

void add_token(TokenType type, int offset)
{
	Token t = { type, offset };
	if (tokens.length == tokens.size)
		tokens.array = realloc(tokens.array, (tokens.size += TOKEN_ARRAY_ALLOC_SIZE) * sizeof(Token));
	tokens.array[tokens.length++] = t;
}

void add_newline()
{
	add_token(NEWLINE, 0);
	line_no++;
	if (i_mode)
		printf("... ");
}

void tokens_free()
{
	free(tokens.array);
	free(string.value);
	free(indent.string);
	free(indent.offsets);
}

TokenType last_token_type()
{
	int i = tokens.length;
	while (i-- > 0)
		if (tokens.array[i].type != NEWLINE)
			return tokens.array[i].type;
	return -1;
}

TokenType get_token_type(int index)
{
	if (index < tokens.length)
		return tokens.array[index].type;
	else
		return 0;
}

char *get_token_value(int index)
{
	// assume index < tokens.length
	return (char *)(string.value + tokens.array[index].offset);
}

void append(char c)
{
	if (string.length == string.size)
		string.value = realloc(string.value, string.size += TOKEN_STRING_ALLOC_SIZE);
	string.value[string.length++] = c;
}

void tokenize(FILE *f)
{
	int c = getc(f);
	byte new_line = 1;		// after a line break
	byte new_block = 1;		// indent increase possible
	int parens = 0;
	int blocks = 0;

	tokens.length = 0;
	string.length = 0;

	set_indent(0, '\0');
	indent.offsets[0] = 0;
	append(0);

	while (1)
	{
		int offset = string.length;
		// take care of whitespace first
		if (new_line)
		{
			int i;
			for (i = 0; c == indent.string[i]; i++)
				c = getc(f);
			if (c == '#')										// disregard indent before comments
				do c = getc(f); while (c != '\n' && c != EOF);
			else if (c == ' ' || c == '\t')						// increase indent
			{
				if (!new_block || i < strlen(indent.string))
					ERROR("SyntaxError: unexpected indentation at line %d", line_no);
				do {
					set_indent(i++, c);
					c = getc(f);
				} while (c == ' ' || c == '\t');
				set_indent(i, '\0');
				indent.offsets[blocks] = i;
				new_block = 0;
			}
			else if (new_block || i < strlen(indent.string))	// unindent and/or single-line block: end block(s)
			{
				if (blocks)		// cannot replace with "while" as indent.offsets[blocks] may be undefined
					do add_token(BLOCK_END, 0); while (--blocks && i < indent.offsets[blocks]);
				if (i != indent.offsets[blocks])
					ERROR("SyntaxError: mismatched indentation at line %d", line_no);
				set_indent(i, '\0');
				new_block = 0;
			}
			if (!i && c == EOF)
				break; //return;
			new_line = 0;
		}
		else
			while (c == ' ' || c == '\t')
				c = getc(f);

		if (c == '\n' || c == EOF)
		{
			if (!parens)	// otherwise, multi-line expression (disregard indent)
			{
				TokenType tt = last_token_type();
				if (tt != SEMICOLON && tt != BLOCK_START && tt != BLOCK_END)
					add_token(SEMICOLON, 0);
				new_line = 1;
				if (i_mode && !blocks)
					c = EOF;
			}
		//	else if (c == EOF)
		//		ERROR("SyntaxError: unterminated parenthetical expression at line %d", line_no);
			if (c == EOF)
				continue;
			add_newline();
		}
		else if (c == '\\')
		{
			c = getc(f);
			if (c != '\n')
				ERROR("SyntaxError: unexpected character after line continuation at line %d", line_no);
			add_newline();
		}
		else if (c == '(')
		{
			add_token(LPAREN, 0);
			parens++;
		}
		else if (c == ')')
		{
			add_token(RPAREN, 0);
			parens--;
		}
	/*	else if (c == '[')
			add_token(LBRACKET, 0);
		else if (c == ']')
			add_token(RBRACKET, 0);
	*/	else if (c == ',')
			add_token(COMMA, 0);
		else if (c == ';')
			add_token(SEMICOLON, 0);
		else if (c == ':')
		{
			if (new_block)		// avoid nested blocks on the same line
				ERROR("SyntaxError: illegal colon at line %d", line_no);
			add_token(BLOCK_START, 0);
			blocks++;
			new_block = 1;
		}
		else if (isalpha(c) || c == '_' || c == '$')
		{
			char *value;
			do 
			{
				append(c);
				c = getc(f);
			} while (isalnum(c) || c == '_' || c == '$');
			append('\0');
			value = (char *)(string.value + offset);
			if (!strcmp(value, "print"))
				add_token(PRINT_KW, 0);
			else if (!strcmp(value, "if"))
				add_token(IF_KW, 0);
			else if (!strcmp(value, "while"))
				add_token(WHILE_KW, 0);
			else if (!strcmp(value, "else"))
				add_token(ELSE_KW, 0);
			else if (!strcmp(value, "def"))
				add_token(DEF_KW, 0);
			else if (!strcmp(value, "global"))
				add_token(GLOBAL_KW, 0);
			else
				add_token(NAME, offset);
			continue;
		}
		else if (isdigit(c) || c == '.')
		{
			byte dot_used = c == '.';
			do append(c); while (isdigit(c = fgetc(f)));
			if (!dot_used && c == '.')
				do append(c); while (isdigit(c = fgetc(f)));
			append('\0');
			add_token(NUMERAL, offset);
			continue;
		}
		else if (c == '"')
		{
			add_token(LITERAL, offset);	// add here, before eventual newlines
			while ((c = getc(f)) != '"')
			{
				if (c == EOF || c == '\n')
					ERROR("SyntaxError: unterminated string at line %d", line_no);
				if (c == '\\')
				{
					switch (c = getc(f))
					{
					case '\n':
						add_newline();
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
					// \x for unicode, ...
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
			append('\0');
		}
		else if (c == '#')
		{
			/*	There are no multi-line comments, as they really don't mix with 
				python-like indentation (unless they are force-indented too but 
				that's ugly).
			*/

			do c = getc(f); while (c != '\n' && c != EOF);
			continue;
		}
		else if (c == '=')
		{
			append(c);
			c = getc(f);
			if (c == '=')
			{
				append(c);
				add_token(EQL_OP, offset);
			}
			else
			{
				add_token(ASG_OP, offset);
				c = ungetc(c, f);
			}
			append('\0');
		}
		else if (c == '!')
		{
			append(c);
			c = getc(f);
			if (c != '=')
				ERROR("SyntaxError: unrecognized character <!> at line %d", line_no);
			append(c);
			append('\0');
			add_token(EQL_OP, offset);
		}
		else if (c == '<' || c == '>')
		{
			append(c);
			c = getc(f);
			add_token(CMP_OP, offset);
			if (c == '=')
				append(c);
			else
				ungetc(c, f);
			append('\0');
		}
		else if (c == '+' || c == '-')
		{
			append(c);
			c = getc(f);
			if (c == '=')
			{
				append(c);
				add_token(ASG_OP, offset);
			}
			else
			{
				add_token(ADD_OP, offset);
				ungetc(c, f);
			}
			append('\0');
		}
		else if (c == '*' || c == '/')
		{
			append(c);
			c = getc(f);
			if (c == '=')
			{
				append(c);
				add_token(ASG_OP, offset);
			}
			else
			{
				add_token(MUL_OP, offset);
				ungetc(c, f);
			}
			append('\0');
		}
		else
			ERROR("SyntaxError: unrecognized character <%c> at line %d", c, line_no);
		c = getc(f);	// where appropriate, "continue" is used to skip this instead of ungetc()
	}

	for (c = 0; c < tokens.length; c++)
		printf("%s %s\n", tkns[tokens.array[c].type], (char *)(string.value + tokens.array[c].offset));
	puts("");
}
