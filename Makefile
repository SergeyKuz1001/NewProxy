#  Copyright 2020 Sergey Kuzivanov
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

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
