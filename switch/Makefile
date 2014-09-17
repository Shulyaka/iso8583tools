all: switch

clean:
		rm -rf *.o switch

switch: switch.o request.o response.o ../lib/isomessage.pb.o ../lib/ipc.o ../lib/kvs.o
		g++ switch.o request.o response.o ../lib/isomessage.pb.o ../lib/ipc.o ../lib/kvs.o -l protobuf -l hiredis -o switch

switch.o: switch.c ../lib/ipc.h ../lib/isomessage.pb.h request.h
		g++ -c switch.c

request.o: request.c ../lib/ipc.h ../lib/kvs.h request.h
		g++ -c request.c

response.o: response.c ../lib/ipc.h ../lib/kvs.h response.h
		g++ -c response.c
