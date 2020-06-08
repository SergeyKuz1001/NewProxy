EXE=proxy
CC_FLAGS=-O3 -Wall -Wpedantic -Wextra
SOURCES=proxy_main.c proxy_funcs.c proxy_private_funcs.c proxy_trace.c user_funcs.c
all: user_funcs.c
	$(CC) $(CC_FLAGS) $(SOURCES) -o $(EXE)
user_funcs.c:
	cp examples/user_funcs_id.c $@
clean:
	$(RM) $(EXE)
