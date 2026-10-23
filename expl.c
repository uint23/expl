#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "expl.h"

typedef struct {
	const char* src;
	size_t      pos;
	expl_error  err;
} parser;

static expl_node* atom(parser* p);
static char* _strndup(const char* src, size_t len);
static expl_node* node_new(expl_type type);
static void node_free(expl_node* node);
static void skip_whitespace(parser* p);

static expl_node* atom(parser* p)
{
	size_t start = p->pos;
	expl_node* node = NULL;

	while (p->src[p->pos] &&
	       !isspace((unsigned char)p->src[p->pos]) &&
	       p->src[p->pos] != '(' &&
	       p->src[p->pos] != ')' &&
	       p->src[p->pos] != '"' &&
	       p->src[p->pos] != ';')
		p->pos++;
	
	if (start == p->pos) {
		p->err = EXPL_ERR_EXPECTED_ATOM;
		return NULL;
	}
	
	node = node_new(EXPL_ATOM);
	if (!node) {
		p->err = EXPL_ERR_OUT_OF_MEMORY;
		return NULL;
	}

	node->value = _strndup(p->src + start, p->pos - start);
	if (!node->value) {
		node_free(node);
		p->err = EXPL_ERR_OUT_OF_MEMORY;
		return NULL;
	}
	
	return node;
}

static char*
_strndup(const char* src, size_t len)
{
	char* str = malloc(len + 1);
	if (!str)
		return NULL;

	memcpy(str, src, len);
	str[len] = '\0';

	return str;
}

static expl_node*
node_new(expl_type type)
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

static void skip_whitespace(parser* p)
{
	while (isspace((unsigned char)p->src[p->pos]) || p->src[p->pos] == ';') {
		/* comment */
		if (p->src[p->pos] == ';') {
			while (p->src[p->pos] != '\n' && p->src[p->pos] != '\0')
				p->pos++;
		}

		/* whitespace */
		else
			p->pos++;
	}
}

int
main(void)
{
	printf("hello world\n");
	return EXIT_SUCCESS;
}

