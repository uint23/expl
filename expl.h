#ifndef EXPL_H
#define EXPL_H

#ifndef EXPL_SPACES
#define EXPL_SPACES "  "
#endif /* EXPL_SPACES */

typedef enum {
	EXPL_TYPE_ATOM,
	EXPL_TYPE_STRING,
	EXPL_TYPE_LIST,

	EXPL_TYPE_COUNT
} expl_type;

typedef enum {
	EXPL_VALUE_NIL,
	EXPL_VALUE_NUMBER,
	EXPL_VALUE_STRING,
	EXPL_VALUE_FUNCTION,

	EXPL_VALUE_COUNT
} expl_value_type;

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
typedef struct expl_value expl_value;
typedef struct expl expl;

typedef expl_value (*expl_native)(int argc, const expl_value* argv);

struct expl_node {
	char*      value;
	expl_type  type;
	expl_node* child;
	expl_node* next;
};

struct expl_value {
	expl_value_type type;
	union {
		double number;
		char*  string;
		void*  function;
	} as;
};

expl* expl_init(void);
void expl_close(expl* ctx);
int expl_register(expl* ctx, const char* name, expl_native fn);
expl_value expl_run(expl* ctx, const char* source);
void expl_print(const expl_node* node, int depth);

#endif /* EXPL_H */

