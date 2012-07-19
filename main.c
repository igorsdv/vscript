#include "main.h"

int main(int argc, char *argv[])
{
	char *source;

	i_mode = 0;
	if (argc > 1)
	{
		// put inside while loop for other options
		if (!strcmp(argv[1], "-i") || !strcmp(argv[1], "--interactive"))
			i_mode = 1;
		else
			source = argv[1];
	}
	else
		i_mode = 1;	// default to interactive mode
	
	if (i_mode)
	{
		while (1)
		{
			printf(">>> ");
			vm_reset();
			tokenize(stdin);
			if (feof(stdin))
				goto finally;
			parse();
			vm_run();
		}
		printf("\n");
	}
	else
	{
		FILE *f = fopen(source, "r");
		if (f == NULL)
			ERROR("FileError: could not open file %s", source);
		vm_reset();
		tokenize(f);
		fclose(f);
		parse();
		vm_run();
	}
finally:
	tokens_free();
	sym_free();
	vm_free();
	return 0;
}
