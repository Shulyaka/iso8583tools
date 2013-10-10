all: libparser.a

clean:
	rm -f *.o libparser.a test message_out

test: libparser.a test.o
		gcc test.o -L . -l parser -o test

libparser.a: parser.o frmload.o tools.o builder.o
		ar rcs libparser.a parser.o frmload.o tools.o builder.o

test.o: test.c parser.h
		gcc -c test.c -ggdb

parser.o: parser.c parser.h
		gcc -c parser.c -ggdb

builder.o: builder.c parser.h
		gcc -c builder.c -ggdb

frmload.o: frmload.c parser.h
		gcc -c frmload.c -ggdb

tools.o: tools.c parser.c
		gcc -c tools.c -ggdb
