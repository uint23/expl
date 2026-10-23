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

static expl_node* newnode(expl_type type);
static void freenode(expl_node* node);

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
	
	node = newnode(EXPL_ATOM);
	if (!node) {
		p->err = EXPL_ERR_OUT_OF_MEMORY;
		return NULL;
	}

	node->value = _strndup(p->src + start, p->pos - start);
	if (!node->value) {
		freenode(node);
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
		return list(p);
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
	expl_node* node = NULL;
	expl_node* tail = NULL;
	expl_node* subnode = NULL;

	skipspace(p);
	if (!current_char(p, '(')) {
		p->err = EXPL_ERR_EXPECTED_LIST;
		return NULL;
	}

	advance(p);

	node = newnode(EXPL_LIST);
	if (!node) {
		p->err = EXPL_ERR_OUT_OF_MEMORY;
		return NULL;
	}

	skipspace(p);
	while (!current_char(p, ')') && !current_char(p, '\0')) {
		subnode = expression(p);
		if (!subnode) {
			freenode(node);
			return NULL;
		}

		if (tail)
			tail->next = subnode;
		else
			node->child = subnode;

		tail = subnode;
		skipspace(p);
	}

	if (!current_char(p, ')')) {
		freenode(node);
		p->err = EXPL_ERR_EXPECTED_LIST;
		return NULL;
	}

	advance(p);
	return node;
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

	advance(p); /* skip opening " */
	start = p->pos;

	while (!current_char(p, '"') && !current_char(p, '\0')) {
		if (current_char(p, '\\') && p->src[p->pos + 1])
			p->pos++;
		p->pos++;
	}

	if (current_char(p, '\0')) {
		p->err = EXPL_ERR_EXPECTED_STRING;
		return NULL;
	}

	node = newnode(EXPL_STRING);
	if (!node) {
		p->err = EXPL_ERR_OUT_OF_MEMORY;
		return NULL;
	}

	node->value = _strndup(p->src + start, p->pos - start);
	advance(p); /* skip closing " */
	if (!node->value) {
		freenode(node);
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
newnode(expl_type type)
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
freenode(expl_node* node)
{
	if (!node)
		return;

	if (node->value)
		free(node->value);

	freenode(node->child);
	freenode(node->next);

	free(node);
}

expl_node*
expl_parse(const char* source)
{
	parser p = { 0 };
	expl_node* head = NULL;
	expl_node** next = &head;

	p.src = source;

	skipspace(&p);
	while (!current_char(&p, '\0')) {
		*next = expression(&p);
		if (!*next) {
			freenode(head);
			return NULL;
		}

		next = &(*next)->next;
		skipspace(&p);
	}

	return head;
}

void
expl_print(const expl_node* node, int depth)
{
	int i;

	while (node) {
		for (i = 0; i < depth; i++)
			printf(EXPL_SPACES);

		switch (node->type) {
		case EXPL_ATOM:
			printf("ATOM: %s\n", node->value);
			break;
		case EXPL_STRING:
			printf("STRING: %s\n", node->value);
			break;
		case EXPL_LIST:
			printf("LIST:\n");
			expl_print(node->child, depth + 1);
			break;
		default:
			printf("UNKNOWN TYPE\n");
		}

		node = node->next;
	}
}

#if 1
int
main(void)
{
	expl_node* node = expl_parse("(fn (\"hi\" x) (* x x))");
	expl_print(node, 0);
	freenode(node);
	return EXIT_SUCCESS;
}
#endif

