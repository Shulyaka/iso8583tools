all: tcpif_visa tcpif_mast

clean:
	rm -f *.o tcpif_visa tcpif_mast

tcpif_visa: tcpinterface.o visa.o ../parser/libparser.a
		gcc tcpinterface.o visa.o -L ../parser -l parser -o tcpif_visa

tcpif_mast: tcpinterface.o mast.o ../parser/libparser.a
		gcc tcpinterface.o visa.o -L ../parser -l parser -o tcpif_mast

tcpinterface.o: tcpinterface.c net.h
		gcc -c tcpinterface.c -ggdb

visa.o: visa.c net.h
		gcc -c visa.c -ggdb

mast.o: mast.c net.h
		gcc -c mast.c -ggdb
