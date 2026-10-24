#ifndef EXPL_H
#define EXPL_H

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

expl* expl_init(void);
void expl_close(expl* ctx);
int expl_register(expl* ctx, const char* name, expl_native fn, void* data);
expl_value expl_run(expl* ctx, const char* source);

#endif /* EXPL_H */

