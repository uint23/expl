#include <stdio.h>
#include <stdlib.h>

#include "expl.h"

static expl_node* node_new(expl_type type);
static void node_free(expl_node* node);

static expl_node*
new_node(expl_type type)
{
	expl_node* node = malloc(sizeof(*node));
	if (!node)
		return NULL;

	node->type = type;
	node->value = NULL;
	node->child = NULL;
	node->next = NULL;

	return node;
}

static void
node_free(expl_node* node)
{
	if (!node)
		return;

	if (node->value)
		free(node->value);

	node_free(node->child);
	node_free(node->next);

	free(node);
}

int
main(void)
{
	printf("hello world\n");
	return EXIT_SUCCESS;
}

