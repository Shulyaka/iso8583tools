CFLAGS=-ggdb

all: isomessage.pb.o ipc.o kvs.o

clean:
		rm -rf *.o isomessage.pb.cc isomessage.pb.h

isomessage.pb.o: isomessage.pb.cc isomessage.pb.h
		g++ -c isomessage.pb.cc ${CFLAGS}

isomessage.pb.cc isomessage.pb.h: isomessage.proto
		protoc --cpp_out=. isomessage.proto

ipc.o: ipc.c ipc.h
		g++ -c ipc.c ${CFLAGS}

kvs.o: kvs.c kvs.h
		g++ -c kvs.c ${CFLAGS}
