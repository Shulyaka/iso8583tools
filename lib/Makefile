all: isomessage.pb.o ipc.o

clean:
		rm -rf *.o isomessage.pb.cc isomessage.pb.h

isomessage.pb.o: isomessage.pb.cc
		g++ -c isomessage.pb.cc

isomessage.pb.cc: isomessage.proto
		protoc --cpp_out=. isomessage.proto

ipc.o: ipc.c ipc.h
		g++ -c ipc.c
