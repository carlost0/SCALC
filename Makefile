.PHONY: clean

scalc: scalc.c
	clang -std=c90 -pedantic -Wall -Wextra scalc.c -o scalc  -O2
static: scalc.c
	clang -std=c90 -pedantic -Wall -Wextra scalc.c -o scalc -static -O2
debug: scalc.c
	clang -std=c90 -pedantic -Wall -Wextra scalc.c -o scalc -fsanitize=address -ggdb
clean:
	rm scalc
