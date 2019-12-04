#
# Makefile for Example
#
#LIBS	= -lsocket -lnsl -lbind
LIBS	= -lnsl
CFLAGS	= -g -ansi -pedantic
LDFLAGS = -L/usr/local/ssl/lib
LDLIBS = -lssl -lcrypto

hellomake: httpClient.c
	gcc httpClient.c -o client;
debugClient: httpClient.c
	gcc -g httpClient.c -o client;
