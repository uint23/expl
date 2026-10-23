#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "expl.h"

#define current_char(p, c) ((p)->src[(p)->pos] == (c))
#define advance(p) ((p)->pos++) /* XXX */

typedef struct {
	const char* src;
	size_t      pos;
	expl_error  err;
} parser;

static expl_node* atom(parser* p);
static expl_node* expression(parser* p);
static expl_node* list(parser* p);
static expl_node* string(parser* p);

static void skipspace(parser* p);
static char* _strndup(const char* src, size_t len);

static expl_node* node_new(expl_type type);
static void node_free(expl_node* node);

static expl_node*
atom(parser* p)
{
	size_t start = p->pos;
	expl_node* node = NULL;
	
	while (!current_char(p, '(') &&
	       !current_char(p, ')') &&
	       !current_char(p, '"') &&
	       !current_char(p, ';') &&
	       !current_char(p, '\0') &&
	       !isspace((unsigned char)p->src[p->pos]))
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

static expl_node*
expression(parser* p)
{
	skipspace(p);
	switch (p->src[p->pos]) {
	case '"':
		return string(p);
	case '(':
	case ')':
	case '\0':
		p->err = EXPL_ERR_UNEXPECTED_TOKEN;
		return NULL;
	default:
		return atom(p);
	}
}

static expl_node*
list(parser* p)
{
}

static expl_node*
string(parser* p)
{
	expl_node* node = NULL;
	size_t start = 0;

	if (!current_char(p, '"')) {
		p->err = EXPL_ERR_EXPECTED_STRING;
		return NULL;
	}

	start = p->pos;
	advance(p); /* skip opening " */

	while (!current_char(p, '"') && !current_char(p, '\0')) {
		if (current_char(p, '\\') && p->src[p->pos + 1])
			p->pos++;
		p->pos++;
	}

	if (current_char(p, '\0')) {
		p->err = EXPL_ERR_EXPECTED_STRING;
		return NULL;
	}

	node = node_new(EXPL_STRING);
	if (!node) {
		p->err = EXPL_ERR_OUT_OF_MEMORY;
		return NULL;
	}

	node->value = _strndup(p->src + start, p->pos - start);
	advance(p); /* skip closing " */
	if (!node->value) {
		node_free(node);
		p->err = EXPL_ERR_OUT_OF_MEMORY;
		return NULL;
	}

	return node;
}

static void
skipspace(parser* p)
{
	while (isspace((unsigned char)p->src[p->pos]) || current_char(p, ';')) {
		/* comment */
		if (current_char(p, ';')) {
			while (!current_char(p, '\n') && !current_char(p, '\0'))
				p->pos++;
		}

		/* whitespace */
		else
			p->pos++;
	}
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

int
main(void)
{
	printf("hello world\n");
	return EXIT_SUCCESS;
}

