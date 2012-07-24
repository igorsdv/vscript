#include "main.h"

int main(int argc, char *argv[])
{
	char *source;

	repl_mode = 0;
	if (argc > 1)
	{
		// put inside while loop for other options
		if (!strcmp(argv[1], "-i") || !strcmp(argv[1], "--repl"))
			repl_mode = 1;
		else
			source = argv[1];
	}
	else
		repl_mode = 1;	// default to interactive mode
	
	if (repl_mode)
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
		FILE *f = fopen(source, "r");
		if (f == NULL)
			ERROR("FileError: could not open file %s", source);
		tokenize(f);
		fclose(f);
		parse();
		dis();
	}

	return 0;
}
