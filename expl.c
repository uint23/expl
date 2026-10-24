#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "expl.h"

#define current_char(p, c) ((p)->src[(p)->pos] == (c))
#define advance(p) ((p)->pos++)
#define pos(p) ((p)->pos)
#define peek(p) ((p)->src[(p)->pos + 1])

typedef enum {
	EXPRESSION_TYPE_ATOM,
	EXPRESSION_TYPE_STRING,
	EXPRESSION_TYPE_LIST,

	EXPRESSION_TYPE_COUNT
} expression_type;

typedef struct environment environment;
typedef struct function function;
typedef struct parser parser;

struct expl {
	environment* env;
	expression*  programs;
};

struct environment {
	char*        name;
	expl_value   value;
	environment* next;
};

struct function {
	expl_native native;
	expression*  args;
	expression*  body;
	void*       data;
};

struct expression {
	char*      value;
	expression_type  type;
	expression* child;
	expression* next;
};

struct parser {
	const char* src;
	size_t      pos;
	expl_error  err;
};

static int define(environment** env, const char* name, expl_value value);
static expression* parse_expression(parser* p);
static expression* list(parser* p);
static expl_value* lookup(environment* env, const char* name);
expression* parse(const char* source);
static expression* value(parser* p, expression_type type);

static expl_value eval(expression* n, environment** env);
static expl_value eval_call(expression* args, environment** env);
static expl_value eval_fn(expression* args, environment** env);
static expl_value eval_user(function* f, int argc, expl_value* argv, environment** env);
static int register_native(environment** env, const char* name, expl_native fn, void* data);

static void skip_space(parser* p);
static char* _strndup(const char* src, size_t len);
static void unescape(char* src);

static expression* new_node(expression_type type);
static void free_node(expression* node);
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

static expression*
parse_expression(parser* p)
{
	skip_space(p);
	switch (p->src[p->pos]) {
	case '"':
		return value(p, EXPRESSION_TYPE_STRING);
	case '(':
		return list(p);
	case ')':
	case '\0':
		p->err = EXPL_ERR_UNEXPECTED_TOKEN;
		return NULL;
	default:
		return value(p, EXPRESSION_TYPE_ATOM);
	}
}

static expression*
list(parser* p)
{
	expression* node = NULL;
	expression** next = NULL;

	advance(p);

	node = new_node(EXPRESSION_TYPE_LIST);
	if (!node) {
		p->err = EXPL_ERR_OUT_OF_MEMORY;
		return NULL;
	}

	next = &node->child;

	skip_space(p);
	while (!current_char(p, ')') && !current_char(p, '\0')) {
		*next = parse_expression(p);
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

expression*
parse(const char* source)
{
	parser p = { 0 };
	expression* head = NULL;
	expression** next = &head;

	if (!source)
		return NULL;
	p.src = source;

	skip_space(&p);
	while (!current_char(&p, '\0')) {
		*next = parse_expression(&p);
		if (!*next) {
			free_node(head);
			return NULL;
		}

		next = &(*next)->next;
		skip_space(&p);
	}

	return head;
}

static expression*
value(parser* p, expression_type type)
{
	expression* node = NULL;
	size_t start = 0;

	if (type == EXPRESSION_TYPE_STRING)
		advance(p);

	start = p->pos;

	if (type == EXPRESSION_TYPE_STRING) {
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

	if (start == p->pos && type == EXPRESSION_TYPE_ATOM) {
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

	if (type == EXPRESSION_TYPE_STRING) {
		unescape(node->value);
		advance(p);
	}

	return node;
}

static expl_value
eval(expression* n, environment** env)
{
	expl_value result = { 0 };
	expl_value* v;
	char* end;

	if (!n)
		return result;

	switch (n->type) {
	case EXPRESSION_TYPE_STRING:
		result.type = EXPL_VALUE_STRING;
		result.as.string = n->value;
		return result;

	case EXPRESSION_TYPE_ATOM:
		result.as.number = strtod(n->value, &end);
		if (end != n->value && !*end) {
			result.type = EXPL_VALUE_NUMBER;
			return result;
		}

		v = lookup(*env, n->value);
		if (v)
			return *v;
		return result;

	case EXPRESSION_TYPE_LIST:
		return eval_call(n, env);

	default:
		return result;
	}
}

static expl_value
eval_call(expression* n, environment** env)
{
	expl_value result = { 0 };
	expl_value* fn = NULL;
	expl_value* argv = NULL;
	expression* arg = NULL;
	function* f = NULL;
	int argc = 0;
	int i = 0;

	if (!n->child || n->child->type != EXPRESSION_TYPE_ATOM)
		return result;
	
	/* defined function */
	if (!strcmp(n->child->value, "fn"))
		return eval_fn(n->child->next, env);	

	fn = lookup(*env, n->child->value);
	if (!fn || fn->type != EXPL_VALUE_FUNCTION)
		return result;

	for (arg = n->child->next; arg; arg = arg->next)
		argc++;

	/* allocate argv */
	if (argc) {
		argv = malloc(argc * sizeof(*argv));
		if (!argv)
			return result;
	}

	/* evaluate arguments */
	for (arg = n->child->next; arg; arg = arg->next)
		argv[i++] = eval(arg, env);

	/* call */
	f = fn->as.function;
	if (f->native)
		result = f->native(argc, argv, f->data);
	else
		result = eval_user(f, argc, argv, env);

	free(argv);
	return result;
}

static expl_value
eval_fn(expression* args, environment** env)
{
	expl_value result = { 0 };
	expression* name = NULL;
	function* f = NULL;

	name = args;
	if (!name || name->type != EXPRESSION_TYPE_ATOM ||
	    !name->next || !name->next->next ||
	    name->next->type != EXPRESSION_TYPE_LIST)
		return result;

	f = malloc(sizeof(*f));
	if (!f)
		return result;

	f->native = NULL;
	f->args = name->next->child;
	f->body = name->next->next;

	result.type = EXPL_VALUE_FUNCTION;
	result.as.function = f;

	if (!define(env, name->value, result)) {
		free(f);
		result.type = EXPL_VALUE_NIL;
	}

	return result;
}

static expl_value
eval_user(function* f, int argc, expl_value* argv, environment** env)
{
	environment* local = *env;
	expression* param = f->args;
	environment* next = NULL;
	expression* body = NULL;
	expl_value result = { 0 };
	int i;

	for (i = 0; i < argc && param; i++) {
		if (!define(&local, param->value, argv[i]))
			break;
		param = param->next;
	}

	if (i == argc && !param) {
		for (body = f->body; body; body = body->next)
			result = eval(body, &local);
	}

	while (local != *env) {
		next = local->next;
		free(local->name);
		free(local);
		local = next;
	}

	return result;
}

static int
register_native(environment** env, const char* name, expl_native fn, void* data)
{
	function* f = malloc(sizeof(*f));
	expl_value value = { 0 };

	if (!f)
		return 0;

	f->native = fn;
	f->args = NULL;
	f->body = NULL;
	f->data = data;

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

static expression*
new_node(expression_type type)
{
	expression* node = malloc(sizeof(*node));
	if (!node)
		return NULL;

	node->type = type;
	node->value = NULL;
	node->child = NULL;
	node->next = NULL;

	return node;
}

static void
free_node(expression* node)
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

expl* expl_init(void)
{
	return calloc(1, sizeof(expl));
}

void expl_close(expl* ctx)
{
	if (!ctx)
		return;

	free_environment(ctx->env);
	free_node(ctx->programs);
	free(ctx);
}

int
expl_register(expl* ctx, const char* name, expl_native fn, void* data)
{
	if (!ctx || !name || !fn)
		return 0;

	return register_native(&ctx->env, name, fn, data);
}

expl_value
expl_run(expl* ctx, const char* source)
{
	expl_value result = { 0 };
	expression* program = NULL;
	expression* n = NULL;

	if (!ctx || !source)
		return result;

	program = parse(source);
	if (!program)
		return result;

	for (n = program; n; n = n->next)
		result = eval(n, &ctx->env);

	if (ctx->programs) {
		n = ctx->programs;
		while (n->next)
			n = n->next;
		n->next = program;
	}
	else
		ctx->programs = program;

	return result;
}

#if 1
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
#endif

