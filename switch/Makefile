all: switch

clean:
		rm -rf *.o switch

switch: switch.o reqresp.o ../lib/isomessage.pb.o ../lib/ipc.o ../lib/kvs.o
		g++ switch.o reqresp.o ../lib/isomessage.pb.o ../lib/ipc.o ../lib/kvs.o -l protobuf -l hiredis -o switch

switch.o: switch.c ../lib/ipc.h ../lib/isomessage.pb.h reqresp.h
		g++ -c switch.c

reqresp.o: reqresp.c ../lib/ipc.h ../lib/kvs.h reqresp.h
		g++ -c reqresp.c

%.o: %.c
