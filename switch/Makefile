all: switch

clean:
		rm -rf *.o switch

switch: switch.o ../lib/isomessage.pb.o
		g++ switch.o ../lib/isomessage.pb.o -l protobuf -o switch

switch.o: switch.c
		g++ -c switch.c
