#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "expl.h"

#define current_char(p, c) ((p)->src[(p)->pos] == (c))
#define advance(p) ((p)->pos++)
#define pos(p) ((p)->pos)
#define peek(p) ((p)->src[(p)->pos + 1])

typedef struct function function;
typedef struct environment environment;
typedef struct parser parser;

struct function {
	expl_native native;
	expl_node*  args;
	expl_node*  body;
};

struct environment {
	char*        name;
	expl_value   value;
	environment* next;
};

struct parser {
	const char* src;
	size_t      pos;
	expl_error  err;
};

static int define(environment** env, const char* name, expl_value value);
static expl_node* expression(parser* p);
static expl_node* list(parser* p);
static expl_value* lookup(environment* env, const char* name);
static expl_node* value(parser* p, expl_type type);

static expl_value eval(expl_node* n, environment** env);
static expl_value eval_call(expl_node* args, environment** env);
static int register_native(environment** env, const char* name, expl_native fn);

static void skip_space(parser* p);
static char* _strndup(const char* src, size_t len);
static void unescape(char* src);

static expl_node* new_node(expl_type type);
static void free_node(expl_node* node);
static void free_environment(environment* env);

static int
define(environment** env, const char* name, expl_value value)
{
	environment* b = malloc(sizeof(*b));
	if (!b)
		return 0;

	b->name = _strndup(name, strlen(name));
	if (!b->name) {
		free(b);
		return 0;
	}

	b->value = value;
	b->next = *env;
	*env = b;

	return 1;
}

static expl_node*
expression(parser* p)
{
	skip_space(p);
	switch (p->src[p->pos]) {
	case '"':
		return value(p, EXPL_TYPE_STRING);
	case '(':
		return list(p);
	case ')':
	case '\0':
		p->err = EXPL_ERR_UNEXPECTED_TOKEN;
		return NULL;
	default:
		return value(p, EXPL_TYPE_ATOM);
	}
}

static expl_node*
list(parser* p)
{
	expl_node* node = NULL;
	expl_node** next = NULL;

	advance(p);

	node = new_node(EXPL_TYPE_LIST);
	if (!node) {
		p->err = EXPL_ERR_OUT_OF_MEMORY;
		return NULL;
	}

	next = &node->child;

	skip_space(p);
	while (!current_char(p, ')') && !current_char(p, '\0')) {
		*next = expression(p);
		if (!*next) {
			free_node(node);
			return NULL;
		}

		next = &(*next)->next;
		skip_space(p);
	}

	if (current_char(p, '\0')) {
		p->err = EXPL_ERR_EXPECTED_LIST;
		free_node(node);
		return NULL;
	}

	advance(p);
	return node;
}

static expl_value*
lookup(environment* env, const char* name)
{
	while (env) {
		if (strcmp(env->name, name) == 0)
			return &env->value;
		env = env->next;
	}

	return NULL;
}

static expl_node*
value(parser* p, expl_type type)
{
	expl_node* node = NULL;
	size_t start = 0;

	if (type == EXPL_TYPE_STRING)
		advance(p);

	start = p->pos;

	if (type == EXPL_TYPE_STRING) {
		while (!current_char(p, '"') && !current_char(p, '\0')) {
			if (current_char(p, '\\') && p->src[p->pos + 1])
				advance(p);
			advance(p);
		}

		if (current_char(p, '\0')) {
			p->err = EXPL_ERR_EXPECTED_STRING;
			return NULL;
		}
	}
	else {
		while (p->src[p->pos] &&
		       !strchr("();\"", p->src[p->pos]) &&
		       !isspace((unsigned char)p->src[p->pos]))
			advance(p);
	}

	if (start == p->pos && type == EXPL_TYPE_ATOM) {
		p->err = EXPL_ERR_EXPECTED_ATOM;
		return NULL;
	}

	node = new_node(type);
	if (!node) {
		p->err = EXPL_ERR_OUT_OF_MEMORY;
		return NULL;
	}

	node->value = _strndup(p->src + start, p->pos - start);
	if (!node->value) {
		free_node(node);
		p->err = EXPL_ERR_OUT_OF_MEMORY;
		return NULL;
	}

	if (type == EXPL_TYPE_STRING) {
		unescape(node->value);
		advance(p);
	}

	return node;
}

static expl_value
eval(expl_node* n, environment** env)
{
	char* end = NULL;

	if (!n)
		return NULL;

	switch (n->type) {
	case EXPL_TYPE_STRING:
		return n;

	case EXPL_TYPE_ATOM:
		strtod(n->value, &end);
		if (end != n->value && !*end)
			return n;

		return lookup(*env, n->value);

	case EXPL_TYPE_LIST:
		/* XXX */
		return NULL;

	default:
		return NULL;
	}
}

static expl_node*
eval_let(expl_node* args, environment** env)
{
static int
register_native(environment** env, const char* name, expl_native fn)
{
	function* f = malloc(sizeof(*f));
	expl_value value = { 0 };

	if (!f)
		return 0;

	f->native = fn;
	f->args = NULL;
	f->body = NULL;

	value.type = EXPL_VALUE_FUNCTION;
	value.as.function = f;

	if (!define(env, name, value)) {
		free(f);
		return 0;
	}

	return 1;
}

static void
skip_space(parser* p)
{
	while (isspace((unsigned char)p->src[p->pos]) || current_char(p, ';')) {
		/* comment */
		if (current_char(p, ';')) {
			while (!current_char(p, '\n') && !current_char(p, '\0'))
				advance(p);
		}

		/* whitespace */
		else
			advance(p);
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

static void
unescape(char* src)
{
	char* dst = src;

	while (*src) {
		if (*src == '\\' && src[1]) {
			src++;
			switch (*src) {
			case 'n':  *dst++ = '\n'; break;
			case 't':  *dst++ = '\t'; break;
			case 'r':  *dst++ = '\r'; break;
			case '\\': *dst++ = '\\'; break;
			case '"':  *dst++ = '"';  break;
			default:
				*dst++ = '\\';
				*dst++ = *src;
			}
		}
		else {
			*dst++ = *src;
		}

		src++;
	}

	*dst = '\0';
}

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
free_node(expl_node* node)
{
	if (!node)
		return;

	if (node->value)
		free(node->value);

	free_node(node->child);
	free_node(node->next);

	free(node);
}

static void
free_environment(environment* env)
{
	while (env) {
		environment* next = env->next;

		if (env->value.type == EXPL_VALUE_FUNCTION)
			free(env->value.as.function);

		free(env->name);
		free(env);
		env = next;
	}
}

expl_node*
expl_parse(const char* source)
{
	parser p = { 0 };
	expl_node* head = NULL;
	expl_node** next = &head;

	if (!source)
		return NULL;
	p.src = source;

	skip_space(&p);
	while (!current_char(&p, '\0')) {
		*next = expression(&p);
		if (!*next) {
			free_node(head);
			return NULL;
		}

		next = &(*next)->next;
		skip_space(&p);
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
		case EXPL_TYPE_ATOM:
			printf("ATOM: %s\n", node->value);
			break;
		case EXPL_TYPE_STRING:
			printf("STRING: %s\n", node->value);
			break;
		case EXPL_TYPE_LIST:
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
	expl_node* node = expl_parse("(fn square (x \"hi\") (* x x))");
	expl_print(node, 0);
	free_node(node);
	return EXIT_SUCCESS;
}
#endif

