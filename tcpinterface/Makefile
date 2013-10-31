CFLAGS=-ggdb


all: tcpif_visa #tcpif_mast

clean:
	rm -f *.o tcpif_visa tcpif_mast core.*

tcpif_visa: tcpinterface.o ../lib/ipc.o tcp.o visa.o ../parser/libparser.a ../lib/isomessage.pb.o
		g++ -L ../parser tcpinterface.o ../lib/ipc.o tcp.o visa.o ../lib/isomessage.pb.o -lparser -lprotobuf -o tcpif_visa

tcpif_mast: tcpinterface.o ../lib/ipc.o tcp.o mast.o ../parser/libparser.a ../lib/isomessage.pb.o
		g++ -L ../parser tcpinterface.o ../lib/ipc.o tcp.o mast.o ../lib/isomessage.pb.o -lparser -lprotobuf -o tcpif_mast

tcpinterface.o: tcpinterface.c net.h tcp.h ../lib/ipc.h ../lib/isomessage.pb.h
		g++ -c tcpinterface.c ${CFLAGS}

visa.o: visa.c net.h ../lib/isomessage.pb.h
		g++ -c visa.c ${CFLAGS}

mast.o: mast.c net.h ../lib/isomessage.pb.h
		g++ -c mast.c ${CFLAGS}

tcp.o: tcp.c tcp.h ../lib/isomessage.pb.h
		g++ -c tcp.c ${CFLAGS}
