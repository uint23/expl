#ifndef EXPL_H
#define EXPL_H

#ifndef EXPL_SPACES
#define EXPL_SPACES "  "
#endif

typedef enum {
	EXPL_TYPE_ATOM,
	EXPL_TYPE_STRING,
	EXPL_TYPE_LIST,

	EXPL_TYPE_COUNT
} expl_type;

typedef enum {
	EXPL_VALUE_NIL,
	EXPL_VALUE_INTGER,
	EXPL_VALUE_FLOAT,
	EXPL_VALUE_STRING,
	EXPL_VALUE_FUNCTION,
	EXPL_VALUE_NATIVE,

	EXPL_VALUE_COUNT
} expl_value;

typedef enum {
	EXPL_ERR_NONE,
	EXPL_ERR_EXPECTED_ATOM,
	EXPL_ERR_EXPECTED_STRING,
	EXPL_ERR_EXPECTED_LIST,
	EXPL_ERR_UNEXPECTED_TOKEN,
	EXPL_ERR_OUT_OF_MEMORY,

	EXPL_ERR_COUNT
} expl_error;

typedef struct expl_node expl_node;

struct expl_node {
	char*      value;
	expl_type  type;
	expl_node* child;
	expl_node* next;
};

void expl_print(const expl_node* node, int depth);
expl_node* expl_parse(const char* source);

#endif /* EXPL_H */

