#include "main.h"

int main(int argc, char *argv[])
{
	int i;
	byte files = 0;

	flags.repl = 0;
	flags.allow_empty_blocks = 0;

	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--repl"))
			flags.repl = 1;
		else if (!strcmp(argv[i], "--allow-empty-blocks"))
			flags.allow_empty_blocks = 1;
		if (*argv[i] == '-')
			*argv[i] = '\0';
		else
			files = 1;
	}
	if (!files)
		flags.repl = 1;		// default to interactive mode
	
	if (flags.repl)
	{
		while (1)
		{
			printf(">>> ");
			tokenize(stdin);
			if (feof(stdin))
				break;
			parse();
			dis();
			reset_tokens();	
			reset_vm();
		}
		printf("\n");
	}
	else
	{
		FILE *f;

		for (i = 1; i < argc; i++)
		{
			if (*argv[i] == '\0')
				continue;
			f = fopen(argv[i], "r");
			if (f == NULL)
				ERROR("FileError: could not open file %s", argv[i]);
			tokenize(f);
			fclose(f);
			parse();
			dis();
			reset_tokens();	
			reset_vm();
		}
	}

	return 0;
}
