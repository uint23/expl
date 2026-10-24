#include <stdio.h>
#include <stdlib.h>

#include "expl.h"

static expl_value
birds_who_hit_my_window(int argc, const expl_value* argv, void* data)
{
	int* count = (int*)data;
	expl_value result = { 0 };

	if (argc == 1 && argv[0].type == EXPL_VALUE_NUMBER)
		*count += (int)argv[0].as.number;

	result.type = EXPL_VALUE_NUMBER;
	result.as.number = *count;

	return result;
}

#define FIND_THE_BIRDS \
"(fn foo (x) (birds_who_hit_my_window x))\n" \
"(foo 134)\n"

int
main(void)
{
	int birds_who_hit_my_window_c = 0;
	expl* ctx = expl_init();

	if (!ctx)
		return EXIT_FAILURE;

	expl_register(ctx, "birds_who_hit_my_window", birds_who_hit_my_window, &birds_who_hit_my_window_c);
	expl_run(ctx, FIND_THE_BIRDS);
	printf("Stupid birds: %d\n", birds_who_hit_my_window_c);

	expl_close(ctx);
	return EXIT_SUCCESS;
}
