CC = gcc209
default: main
main: ish
ish: ish.c dynarray.c
	$(CC) -o $@ $^
ish_test: ish_test.c dynarray.c token.c
clean:
	rm -f *.o *.i *.s
