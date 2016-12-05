CFLAGS=-ggdb -Wall


all: tcpif_visa #tcpif_mast

clean:
	rm -f *.o tcpif_* *.pb.cc *.pb.h core.*

tcpif_%: tcpinterface.o ../lib/ipc.o tcp.o %.o ../parser/libparser.a %.pb.o ../lib/isomessage.pb.o
		g++ -L ../parser tcpinterface.o ../lib/ipc.o tcp.o $*.o ../lib/isomessage.pb.o $*.pb.o -lparser -lprotobuf -o $@

tcpinterface.o: tcpinterface.cpp net.h tcp.h ../lib/ipc.h ../lib/isomessage.pb.h
		g++ -c tcpinterface.cpp ${CFLAGS}

tcp.o: tcp.cpp tcp.h ../lib/isomessage.pb.h
		g++ -c tcp.cpp ${CFLAGS}

%.o: %.cpp
%.o: %.cpp net.h ../lib/isomessage.pb.h %.pb.h
		g++ ${CFLAGS} -c $<

../lib/isomessage.pb.h ../lib/isomessage.pb.o:
		${MAKE} -C ../lib

%.pb.o: %.pb.cc %.pb.h
		g++ -c $<

%.pb.cc %.pb.h: %.proto
		protoc --cpp_out=. $<

../lib/ipc.h ../lib/ipc.o:
		${MAKE} -C ../lib

../parser/libparser.a:
		${MAKE} -C ../parser
