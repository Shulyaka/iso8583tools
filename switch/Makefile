all: isomessage.pb.o

clean:
		rm -rf isomessage.pb.o isomessage.pb.cc isomessage.pb.h

isomessage.pb.o: isomessage.pb.cc
		g++ -c isomessage.pb.cc

isomessage.pb.cc: isomessage.proto
		protoc --cpp_out=. isomessage.proto
