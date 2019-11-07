#
# Makefile for Example
#
LIBS	= 
CFLAGS	= -g -ansi -pedantic

all:	tcp 

#
# Internet stream version (TCP protocol).
#

tcp:	server client hostent

client.o server.o: inet.h

server:	server.o 
	gcc $(CFLAGS) -o myserver server.o $(LIBS)

client:	client.o 
	gcc $(CFLAGS) -o myclient client.o $(LIBS)

hostent: hostent.o
	gcc $(CFLAGS) -o $@ hostent.o $(LIBS)

#
# Clean up the mess we made
#
clean:
	rm *.o \
	myserver myclient hostent
 
