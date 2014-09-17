CFLAGS=-ggdb


all: tcpif_visa #tcpif_mast

clean:
	rm -f *.o tcpif_* *.pb.cc *.pb.h core.*

tcpif_%: tcpinterface.o ../lib/ipc.o tcp.o %.o ../parser/libparser.a %.pb.o ../lib/isomessage.pb.o
		g++ -L ../parser tcpinterface.o ../lib/ipc.o tcp.o $*.o ../lib/isomessage.pb.o $*.pb.o -lparser -lprotobuf -o $@

tcpinterface.o: tcpinterface.c net.h tcp.h ../lib/ipc.h ../lib/isomessage.pb.h
		g++ -c tcpinterface.c ${CFLAGS}

tcp.o: tcp.c tcp.h ../lib/isomessage.pb.h
		g++ -c tcp.c ${CFLAGS}

%.o: %.c
%.o: %.c net.h ../lib/isomessage.pb.h %.pb.h
		g++ ${CFLAGS} -c $<

../lib/isomessage.pb.h ../lib/isomessage.pb.o:
		echo "Please go to ../lib and run 'make'"
		exit 1

%.pb.o: %.pb.cc %.pb.h
		g++ -c $<

%.pb.cc %.pb.h: %.proto
		protoc --cpp_out=. $<
