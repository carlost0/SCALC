CC = cc
CFLAGS = -std=c90 -pedantic -Wall -Wextra
.PHONY: clean

scalc: scalc.c
	$(CC) $(CFLAGS) -O2 scalc.c -o scalc
static: scalc.c
	$(CC) $(CFLAGS) -O2 -static scalc.c -o scalc
debug: scalc.c
	clang -fsanitize=address $(CFLAGS) -ggdb scalc.c -o scalc
clean:
	rm scalc
