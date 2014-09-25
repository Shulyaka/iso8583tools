CFLAGS=-ggdb

all: saf

clean:
		rm -rf *.o saf

saf: saf.o reqresp.o ../lib/isomessage.pb.o ../lib/ipc.o ../lib/kvs.o
		g++ saf.o reqresp.o ../lib/isomessage.pb.o ../lib/ipc.o ../lib/kvs.o -l protobuf -l hiredis -o saf

saf.o: saf.c ../lib/ipc.h ../lib/isomessage.pb.h reqresp.h
		g++ -c saf.c ${CFLAGS}

reqresp.o: reqresp.c ../lib/ipc.h reqresp.h
		g++ -c reqresp.c ${CFLAGS}

%.o: %.c
