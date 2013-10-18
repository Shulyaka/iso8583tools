all: switch

clean:
		rm -rf *.o switch

switch: switch.o ../lib/isomessage.pb.o ../lib/ipc.o
		g++ switch.o ../lib/isomessage.pb.o ../lib/ipc.o -l protobuf -o switch

switch.o: switch.c ../lib/ipc.h
		g++ -c switch.c
