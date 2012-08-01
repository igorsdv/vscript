#include "main.h"

void *safe_malloc(size_t size)
{
	void *tmp = malloc(size);
	if (!tmp) error("MemoryError: memory allocation failed");
	return tmp;
}

void *safe_calloc(size_t num, size_t size)
{
	void *tmp = calloc(num, size);
	if (!tmp) error("MemoryError: memory allocation failed");
	return tmp;
}

void *safe_realloc(void *ptr, size_t size)
{
	void *tmp = realloc(ptr, size);
	if (!tmp) error("MemoryError: memory allocation failed");
	return tmp;
}

int main(int argc, char *argv[])
{
	int i;

	struct env main_env = { 0 };
	main_env.co = safe_calloc(1, sizeof *main_env.co);

	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--repl"))
			options |= flag_repl;
		else if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--bytecode"))
			options |= flag_bytecode;
		else if (!strcmp(argv[i], "--allow-empty-blocks"))
			options |= flag_allow_empty_blocks;
		if (*argv[i] == '-')
			*argv[i] = '\0';
		else
			options |= option_files;
	}

	if (options & option_files)
	{
		FILE *f;

		for (i = 1; i < argc; i++)
		{
			if (*argv[i] == '\0')
				continue;

			if (!(f = fopen(argv[i], "r")))
			{
				log("FileError: could not open file '%s'", argv[i]);
				break;
			}

			tokenize(f);
			fclose(f);

			parse(main_env.co);
			reset_tokens();

			if (options & flag_bytecode)
				dis(main_env.co, &main_env.offset);
			else run(&main_env);
		}
	}
	else
		options |= flag_repl;	// default to interactive mode

	if (options & flag_repl)
	{
		repl_mode = 1;

		while (1)
		{
			printf(">>> ");

			tokenize(stdin);
			if (feof(stdin)) break;

			parse(main_env.co);
			reset_tokens();

			if (options & flag_bytecode)
				dis(main_env.co, &main_env.offset);
			else run(&main_env);
		}

		printf("\n");
	}

	free(main_env.objects.array);

	return 0;
}
