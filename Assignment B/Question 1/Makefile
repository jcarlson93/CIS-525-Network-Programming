#
# Makefile for Example
#
#LIBS	= -lsocket -lnsl -lbind
LIBS	= -lnsl
CFLAGS	= -g -ansi -pedantic

hellomake: chatClient1.c chatServer1.c
	gcc chatServer1.c -o server;
	gcc chatClient1.c -o client;
	test -f users && rm users;
	touch users;
 