all: debit

clean:
		rm -rf *.o debit

debit: debit.o request.o ../lib/isomessage.pb.o ../lib/ipc.o
		g++ debit.o request.o ../lib/isomessage.pb.o ../lib/ipc.o -l protobuf -o debit

debit.o: debit.c ../lib/ipc.h ../lib/isomessage.pb.h request.h
		g++ -c debit.c

request.o: request.c ../lib/ipc.h request.h
		g++ -c request.c

