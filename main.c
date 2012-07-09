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
	vm_init();
	if (i_mode)
	{
		while (!feof(stdin))
		{
			tokens_init();
			tokenize_input();
			parse();
			vm_run();	// needs vm_reset or something
			tokens_free();
		}
		printf("\n");
	}
	else
	{
		tokens_init();
		tokenize_file(source);
		parse();
		vm_run();
		tokens_free();
	}
	sym_free();
	vm_free();
	return 0;
}