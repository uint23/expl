#ifndef EXPL_H
#define EXPL_H

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

typedef struct expression expression;
typedef struct expl_value expl_value;
typedef struct expl expl;

typedef expl_value (*expl_native)(int argc, const expl_value* argv, void* data);

struct expl_value {
	expl_value_type type;
	union {
		double number;
		char*  string;
		void*  function;
	} as;
};

/* create a new context and initialize it
   returns context on success, NULL on failure */
expl* expl_init(void);

/* free given context and resources */
void expl_close(expl* ctx);

/* register a native function in the given context
   returns 1 on success, 0 on failure */
int expl_register(expl* ctx, const char* name, expl_native fn, void* data);

/* run the source code in the given context
   returns the result of expression evaluated, return->type is EXPL_VALUE_NIL on
   failure */
expl_value expl_run(expl* ctx, const char* source);

#endif /* EXPL_H */

