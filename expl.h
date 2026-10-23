#ifndef EXPL_H
#define EXPL_H

typedef enum {
	EXPL_ATOM,
	EXPL_STRING,
	EXPL_LIST,

	EXPL_TYPE_COUNT
} expl_type;

typedef struct expl_node expl_node;

struct expl_node {
	char*      value;
	expl_type  type;
	expl_node* child;
	expl_node* next;
};

#endif /* EXPL_H */

