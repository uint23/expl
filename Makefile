CC = cc
AR = ar
CFLAGS = -ansi -pedantic -Wall -Wextra

SRC = expl.c
HDR = expl.h
OUT = expl.o
LIB = libexpl.a
TEST_SRC = test.c
TEST_OUT = expl

all: ${LIB}

${OUT}: ${SRC} ${HDR}
	${CC} ${CFLAGS} -c ${SRC} -o ${OUT}

${LIB}: ${OUT}
	${AR} rcs ${LIB} ${OUT}

test: ${TEST_OUT}
	./${TEST_OUT}

${TEST_OUT}: ${TEST_SRC} ${LIB}
	${CC} ${CFLAGS} -o ${TEST_OUT} ${TEST_SRC} ${LIB}

clean:
	rm -f ${OUT} ${LIB} ${TEST_OUT}

compile_flags:
	printf '%s\n' ${CFLAGS} > compile_flags.txt
