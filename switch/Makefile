all: switch

clean:
		rm -rf *.o switch

switch: switch.o request.o ../lib/isomessage.pb.o ../lib/ipc.o
		g++ switch.o request.o ../lib/isomessage.pb.o ../lib/ipc.o -l protobuf -o switch

switch.o: switch.c ../lib/ipc.h request.h
		g++ -c switch.c

request.o: request.c ../lib/ipc.h request.h
		g++ -c request.c
