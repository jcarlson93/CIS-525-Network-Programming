#
# Makefile for Example
#
#LIBS	= -lsocket -lnsl -lbind
LIBS	= -lnsl
CFLAGS	= -g -ansi -pedantic
LDFLAGS = -L/usr/local/ssl/lib
LDLIBS = -lssl -lcrypto

hellomake: chatClient2.c chatServer2.c directoryServer2.c
	gcc chatServer2.c -o server -lssl -lcrypto;
	gcc chatClient2.c -o client -lssl -lcrypto -lpthread;
	gcc directoryServer2.c -o directory -lssl -lcrypto;
debugDirectory: directoryServer2.c
	gcc -g directoryServer2.c -o directory -lssl -lcrypto;
debugClient: chatClient2.c
	gcc -g chatClient2.c -o client -lssl -lcrypto -lpthread;

 debugServer: chatServer2.c
	gcc -g chatServer2.c -o server -lssl -lcrypto;
