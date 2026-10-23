#ifndef EXPL_H
#define EXPL_H

typedef enum {
	EXPL_ATOM,
	EXPL_STRING,
	EXPL_LIST,

	EXPL_TYPE_COUNT
} expl_type;

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

#endif /* EXPL_H */

