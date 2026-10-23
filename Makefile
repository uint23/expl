CC = cc
CFLAGS = -ansi -pedantic -Wall -Wextra -Werror
SRC = expl.c
OUT = expl

all: ${SRC}
	${CC} ${CFLAGS} -o ${OUT} ${SRC}

compile_flags:
	rm -f compile_flags.txt
	for f in ${CFLAGS}; do echo $$f >> compile_flags.txt; do


