CFLAGS=-ggdb

.PHONY: clean all tests

all: libparser.a testparse tests

clean:
	rm -f *.o libparser.a testparse core.* imessage* omessage*

testparse: libparser.a testparse.o
		g++ testparse.o -L . -l parser -o testparse

libparser.a: field.o fldformat.o parser.o builder.o frmiterator.o
		ar rcs libparser.a field.o fldformat.o parser.o builder.o frmiterator.o

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

frmiterator.o: frmiterator.cpp parser.h
		g++ -c frmiterator.cpp ${CFLAGS}

tests: testparse
		for i in `ls tests`; do ./testparse tests/$$i >/dev/null || (echo "Test $$i failed with status $$?"; exit 1) || exit 1 ; done
