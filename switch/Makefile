all: isomessage.pb.o switch_loadbalancer

clean:
		rm -rf *.o isomessage.pb.cc isomessage.pb.h switch_loadbalancer

isomessage.pb.o: isomessage.pb.cc
		g++ -c isomessage.pb.cc

isomessage.pb.cc: isomessage.proto
		protoc --cpp_out=. isomessage.proto

switch_loadbalancer: switch_loadbalancer.o isomessage.pb.o
		g++ switch_loadbalancer.o isomessage.pb.o -l protobuf -o switch_loadbalancer

switch_loadbalancer.o: switch_loadbalancer.c
		g++ -c switch_loadbalancer.c
