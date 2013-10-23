all: libparser.a

clean:
	rm -f *.o libparser.a test message_out

test: libparser.a test.o
		g++ test.o -L . -l parser -o test

libparser.a: parser.o frmload.o tools.o builder.o
		ar rcs libparser.a parser.o frmload.o tools.o builder.o

test.o: test.c parser.h
		g++ -c test.c -ggdb

parser.o: parser.c parser.h
		g++ -c parser.c -ggdb

builder.o: builder.c parser.h
		g++ -c builder.c -ggdb

frmload.o: frmload.c parser.h
		g++ -c frmload.c -ggdb

tools.o: tools.c parser.h
		g++ -c tools.c -ggdb
