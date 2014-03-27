CFLAGS=-ggdb


all: libparser.a testparse

clean:
	rm -f *.o libparser.a testparse message_out

testparse: libparser.a testparse.o
		g++ testparse.o -L . -l parser -o testparse

libparser.a: parser.o frmload.o tools.o builder.o
		ar rcs libparser.a parser.o frmload.o tools.o builder.o

testparse.o: testparse.c parser.h
		g++ -c testparse.c ${CFLAGS}

parser.o: parser.c parser.h
		g++ -c parser.c ${CFLAGS}

builder.o: builder.c parser.h
		g++ -c builder.c ${CFLAGS}

frmload.o: frmload.c parser.h
		g++ -c frmload.c ${CFLAGS}

tools.o: tools.c parser.h
		g++ -c tools.c ${CFLAGS}
