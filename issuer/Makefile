all: issuer

clean:
		rm -rf *.o issuer

issuer: issuer.o request.o ../lib/isomessage.pb.o ../lib/ipc.o
		g++ issuer.o request.o ../lib/isomessage.pb.o ../lib/ipc.o -l protobuf -o issuer

issuer.o: issuer.c ../lib/ipc.h ../lib/isomessage.pb.h request.h
		g++ -c issuer.c

request.o: request.c ../lib/ipc.h request.h
		g++ -c request.c

