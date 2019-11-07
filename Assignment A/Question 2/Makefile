#
# Makefile for Example
#
# LIBS	= -lsocket -lnsl
LIBS	=
CFLAGS	= -g

all:	udp

#
# Internet datagram version (UDP protocol).
#

udp:	UDPserver UDPclient

client.o server.o: inet.h

UDPserver: server.o 
	gcc $(CFLAGS) -o UDPserver server.o $(LIBS)

UDPclient: client.o 
	gcc $(CFLAGS) -o UDPclient client.o $(LIBS)

#
# Clean up the mess we made
#
clean:
	rm *.o \
	UDPserver UDPclient
 
