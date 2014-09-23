CFLAGS=-ggdb

all: saf

clean:
		rm -rf *.o saf

saf: saf.o response.o ../lib/isomessage.pb.o ../lib/ipc.o ../lib/kvs.o
		g++ saf.o response.o ../lib/isomessage.pb.o ../lib/ipc.o ../lib/kvs.o -l protobuf -l hiredis -o saf

saf.o: saf.c ../lib/ipc.h ../lib/isomessage.pb.h response.h
		g++ -c saf.c ${CFLAGS}

response.o: response.c ../lib/ipc.h response.h
		g++ -c response.c ${CFLAGS}

%.o: %.c
