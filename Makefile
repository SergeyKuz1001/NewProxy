# Copyright [2020] [Sergey Kuzivanov]

TARGET=newproxy
SOURCES:=$(wildcard proxy_*.c)
OBJS:=$(patsubst %c,%o,${SOURCES})
LIB=lib_${TARGET}.a
CC_FLAGS=-O3 -Wall -Wextra -Wpedantic
EXE=proxy
.PHONY: clean# all
all: ${EXE} Makefile
clean:
	${RM} ${OBJS} ${LIB} ${EXE}
${LIB}: ${OBJS}
	${AR} rcs $@ $^
${EXE}: user_funcs.c ${LIB}
	${CC} ${CC_FLAGS} -I . -o $@ $^
user_funcs.c:
	cp examples/user_funcs_id.c $@
