all: tcpif_visa tcpif_mast

clean:
	rm -f *.o tcpif_visa tcpif_mast

tcpif_visa: tcpinterface.o ipc.o tcp.o visa.o ../parser/libparser.a ../switch/isomessage.pb.o
		g++ -L ../parser tcpinterface.o ipc.o tcp.o visa.o ../switch/isomessage.pb.o -lparser -lprotobuf -o tcpif_visa

tcpif_mast: tcpinterface.o ipc.o tcp.o mast.o ../parser/libparser.a ../switch/isomessage.pb.o
		g++ -L ../parser tcpinterface.o ipc.o tcp.o visa.o ../switch/isomessage.pb.o -lparser -lprotobuf -o tcpif_mast

tcpinterface.o: tcpinterface.c net.h tcp.h ipc.h
		g++ -c tcpinterface.c -ggdb

visa.o: visa.c net.h ../switch/isomessage.pb.h
		g++ -c visa.c -ggdb

mast.o: mast.c net.h ../switch/isomessage.pb.h
		g++ -c mast.c -ggdb

ipc.o: ipc.c ipc.h
		g++ -c ipc.c -ggdb

tcp.o: tcp.c tcp.h
		g++ -c tcp.c -ggdb
