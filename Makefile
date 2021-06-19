CC = gcc209
default: main
main: ish
ish: ish.o dynarray.o process.o token.o
	$(CC) -o $@ $^
ish.o: ish.c
	$(CC) -c $<
dynarray.o: dynarray.c dynarray.h
	$(CC) -c $<
process.o: process.c process.h
	$(CC) -c $<
token.o: token.c token.h
	$(CC) -c $<
clean:
	rm -f *.o *.i *.s
