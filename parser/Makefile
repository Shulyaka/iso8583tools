CFLAGS=-ggdb


all: libparser.a testparse

clean:
	rm -f *.o libparser.a testparse core.* imessage* omessage*

testparse: libparser.a testparse.o
		g++ testparse.o -L . -l parser -o testparse

libparser.a: field.o fldformat.o parser.o builder.o
		ar rcs libparser.a field.o fldformat.o parser.o builder.o

testparse.o: testparse.cpp parser.h
		g++ -c testparse.cpp ${CFLAGS}

parser.o: parser.cpp parser.h
		g++ -c parser.cpp ${CFLAGS}

builder.o: builder.cpp parser.h
		g++ -c builder.cpp ${CFLAGS}

field.o: field.cpp parser.h
		g++ -c field.cpp ${CFLAGS}

fldformat.o: fldformat.cpp parser.h
		g++ -c fldformat.cpp ${CFLAGS}
