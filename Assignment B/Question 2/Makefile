#
# Makefile for Example
#
#LIBS	= -lsocket -lnsl -lbind
LIBS	= -lnsl
CFLAGS	= -g -ansi -pedantic

hellomake: chatClient2.c chatServer2.c
	gcc chatServer2.c -o server;
	gcc chatClient2.c -o client;
	gcc directoryServer2.c -o directory;
	test -f serverNames && rm serverNames;
	touch serverNames;
	rm *users;
 
