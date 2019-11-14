/* Assignment D by Nickalas Porsch based off of previous assignment c.
Chat client
		To run type make and then ./directory&; Then start an indvidual server  by doing ./server PORT TOPIC and then create a client with ./client
	Where you provide the PORT and TOPIC

	Client does not have a SSL certificate
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "inet.h"
#include <signal.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <pthread.h> /*For help on threads I used this example: https://www.geeksforgeeks.org/multithreading-c-2/*/

#define MAX 10000 /* This definition defines the maximum number of characters that can be read in character arrays */

/* The user struct holds information on the connection to a chat server */
typedef struct User {
	char			username[MAX]; /* Holds the username of the client on the chat server */
	SSL *			ssl; /* Holds the SSL Connection to the chat server*/
} User;


int closeConnection = 0; /* This is a flag that helps to tell if a user does an interrupt before the connection is properly closed. */
int                 chatfd; /* File Descriptor for connection to a chat server */
int					directoryfd; /* File descriptor for connection to the directory */
SSL_CTX *			chatCtx = NULL; /* The context of the SSL Connection of the chat server the client is connected to.*/
User *				user; /* The user struct holds basic information about the connection to the chat server such as user name and SSL connection */
SSL_CTX *			ctxdir = NULL; /* The context of the SSL Connection that connects the directory to the client. */
SSL *				ssldir = NULL; /* The SSL connection that connects the client to the directory */

/* Method definitions please see below main function for their descriptions*/
char * get_message(void);
char * get_username(char *);
char * get_topic(void);
void recieveChatMessage(User * user);
void sendChatMessage(User  * user);
void shutdownClient(int sig_num);
void ShowCerts(SSL* ssl, char* nameOfServer);
void endDirectoryConnection();
void endServerConnection();
SSL_CTX * InitCTX(void);

/* Main Function */
int main()
{

	int listenerfd, clilen,  nread, port; /* The first 3 file descriptors are used to connect to listen for new information, connect to the driectory server, and connect to the chat server respectively*/
	struct sockaddr_in	cli_addr, dir_addr, serv_addr; /* All the socket addresses of the client, directory, and chat server */
	char                s[MAX] = { '\0' }; /*String that is used to send information between the server and client*/
	char				topic[MAX] = "l"; /*The topic of the chat server. It is initally set to l so the client will view all the servers connnected to the directory */
	char				message[MAX] = { '\0' }; /* Any message that is read in from the server that the user is connnected to*/
	char *				chatServer[MAX] = { '\0' }; /*This string will hold the server information for the chat Server*/
	SSL_CTX *			ctxchat; /* Context for sslchat*/
	pthread_t			recieveMessage; /* Thread for handling recieving messages */
	pthread_t			sendMessage; /* Thread for handling sending messages */

	/*Initailizing reader integer and the File Descriptor Set, readfds */
	nread = 0; 

	/* Setting up connection to Directory Server */
	if ((directoryfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("server: can't open stream socket");
		exit(1);
	}


	/* Sets up the address of the server to be contacted. */
	memset((char *)&dir_addr, 0, sizeof(dir_addr));
	dir_addr.sin_family = AF_INET;
	dir_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
	dir_addr.sin_port = htons(SERV_TCP_PORT);

	/* Connects to the server*/
	if (connect(directoryfd, (struct sockaddr *) &dir_addr, sizeof(dir_addr)) < 0) {
		perror("Connection error to directory.\n");
		exit(1);
	}


	/* Creating SSL Connection to Directory*/
	SSL_library_init();

	/* Creates a new ssl connection to the directory server and adds the file descriptor the set of file descriptors*/
	ctxdir = InitCTX();
	ssldir = SSL_new(ctxdir);
	SSL_set_fd(ssldir, directoryfd);


	/* Connects to the directory using SSL */
	if (SSL_connect(ssldir) <= 0) {
		ERR_print_errors_fp(stdout);
		exit(1);
	}
	

	/*Show the certificates from the directory to see if it is legit */
	ShowCerts(ssldir, "directory");
	printf("Showing Certs. \n");
	


	/* Read in infromation from the directory server */
	nread = SSL_read(ssldir, s, MAX);
	
	if (nread > 0) {
		printf("%s", s);

	}
	
	printf("\n--------------------------------------------\n");


	/* Start the interrupt so the connnections can be closed if the user needs to leave unexpectedly. */
	signal(SIGINT, endDirectoryConnection);
	
	/*This while loop keeps the user listing servers until he or she enters a server name.*/
	while (strcmp(topic, "l") == 0) {
		
		sprintf(topic, get_topic());
		
		/* If the client enter l then the directory server will list all current chat servers */
		if (strcmp(topic, "l") == 0) {
			strcpy(s, "L,");
			strcat(s, "NONE");
			strcat(s, ",");
			strcat(s, topic); /* This is building the message from the user to say the username and to say a username is being sent */
			printf("%s\n", s);
			SSL_write(ssldir, s, MAX);
			

			printf("===========================================\n");
			printf("Here are all known servers:\n");
			printf("-------------------------------------------\n");


			/* Read the server's response. */
			memset(s, 0, MAX);
			nread = SSL_read(ssldir, s, MAX);
			if (nread > 0) {
				printf("%s", s);

			}
			printf("--------------------------------------------\n");

		}
		else {

			/* The user has entered a server name now we check to see if it exists */
			memset(s, 0, MAX);
			sprintf(s, "N,NONE,%s", topic);

			SSL_write(ssldir, s, MAX);

			/* Read the server's response. */
			memset(s, 0, MAX);
			nread = SSL_read(ssldir, s, MAX);
			if (nread > 0) {
				printf("\n");

				printf("%s", s);
				printf("\n");
				/* If the server does not exist we place them back in the loop. */
				if (strcmp(s, "Server not Found.") == 0) {
					strcpy(topic, "l");
				}
				else {
					/* If a server has been found we are given the port number.*/
					strcpy(chatServer, strtok(s, ":"));
					strcpy(chatServer, strtok(NULL, ":"));
					sscanf(chatServer, "%d", &port);
				}

			}


		}

	}

	/* The client closes connection to the directory */
	close(directoryfd);
	SSL_free(ssldir);
	SSL_CTX_free(ctxdir);

	/* We create the chat server context*/
	chatCtx = InitCTX();
	int usernameSet = 0;

	user = (User *)malloc(sizeof(User));
	user->ssl = NULL;
	memset(user->username, 0, MAX);

	/* Create a socket (an endpoint for communication). */
	if ((chatfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("client: can't open stream socket");
		exit(1);
	}

	/* Set up the address of the server to be contacted. */
	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
	serv_addr.sin_port = htons(port);

	printf("Connecting.\n");

	/* Connect to the server. */
	if (connect(chatfd, (struct sockaddr *) &serv_addr,
		sizeof(serv_addr)) < 0) {
		perror("client: can't connect to server");
		exit(1);
	}
	printf("Connected.\n");
	
	/*Create the SSL Connection to the chat server*/
	user->ssl = SSL_new(chatCtx);
	SSL_set_fd(user->ssl, chatfd);

	if (SSL_connect(user->ssl) <= 0)			/* perform the connection */
		ERR_print_errors_fp(stderr);

	/* Start the interrupt so the connnections can be closed if the user needs to leave unexpectedly. */
	signal(SIGINT, endServerConnection);

	/* Gather Username from user */
	while (usernameSet == 0) {

		strcpy(user->username, get_username(topic));
		strcpy(s, "U,");
		strcat(s, user->username);
		strcat(s, ",");
		strcat(s, user->username); /* This is building the message from the user to say the username and to say a username is being sent */


		/*Show the certs of the server to make sure they are legit*/
		ShowCerts(user->ssl, topic);

		/*See how much is written by the chat server*/
		int numWrote = SSL_write(user->ssl, s, MAX);


		memset(s, 0, MAX);

		/* Read the server's response. and find out if the username is taken */
		nread = SSL_read(user->ssl, s, MAX);
		if (nread > 0) {
			
			printf(s);
			
			if (strcmp(s, "Username already taken.\n") == 0) {
				usernameSet = 0;	
			}
			else {
				usernameSet = 1;
			}

		}
	}

	/* We can only pass one parameter to a thread method so to solve that we must use a struct. I followed this example: http://www.cse.cuhk.edu.hk/~ericlo/teaching/os/lab/9-PThread/Pass.html*/
	/* This thread is used for sending messages to the chat server */
	if ((pthread_create(&sendMessage, NULL, sendChatMessage, user) != 0)) { 
		perror("Creating Send Message Thread failed!\n");
		exit(1);
	}

	/* This thread is used for recieving messages from the chat server */
	if ((pthread_create(&recieveMessage, NULL, recieveChatMessage, user) != 0)) {
		perror("Creating recieve Message Thread failed!\n");
		exit(1);
	}

	while (1) { /* Since the threads are taking care of everything else the parent waits */

	}

	return 0;
}

/* This method is used by a thread to handle messages sent from the client to the chat server */

void sendChatMessage(User  * user) {
	char			s[MAX] = { '\0' }; /* Used for creating messages to send to the server and recieving messages from the server */
	char *			message[MAX] = { '\0' }; /* The message the user will send */

	for (;;) {
		/* This thread will focus on gathering input from the user to send messages*/
		strcpy(message, get_message());
		strcpy(s, "M,");
		strcat(s, user->username);
		strcat(s, ",");
		strcat(s, message); /* This is building the message from the user to say the username and to say a username is being sent */


				
		/* Send the message to server */
		SSL_write(user->ssl, s, sizeof(char) * 257);
		/*257 is 255 (The max size of the message) + the size of "M," which is 2 * sizeof(char)which is 2 * sizeof(char) */

		/*Free the memory of the message sent*/
		if (message != NULL) {
			memset(message, 0, MAX);
		}

		memset(s, 0, MAX);
	}
}

/* This method is used by a thread to handle messages recieved from the chat server */
void recieveChatMessage(User * user) {
	int				nread = 0;
	char			s[MAX];

	memset(s, 0, MAX);


	/* Read the server's response. */
	for (;;) {
		nread = SSL_read(user->ssl, s, MAX);
		if (nread > 0) {
			printf("\n\n==================================\n");
			printf(s);
			printf("==================================\n");
		}
	}
}

/* When a user sends a SIGTERM when connnected to the directory this method will properly handle closing the connection */
void endDirectoryConnection() {
	char s[MAX]; /* Used for creating messages to send to the server and recieving messages from the server */

	strcpy(s, "C,NONE,NONE");

	SSL_write(ssldir, s, MAX);

	SSL_read(ssldir, s, MAX);

	/* The directory will send Good Bye! and the connection will be closed. */
	printf("\n\n==================================\n");
	printf(s);
	printf("==================================\n");

	/* The client closes connection to the directory */
	close(directoryfd);
	SSL_free(ssldir);
	SSL_CTX_free(ctxdir);
	exit(0);
}

/* When a user sends a SIGTERM when connnected to the server this method will properly handle closing the connection */
void endServerConnection() {
	char s[MAX]; /* Used for sending information to the server. This will tell all other users on the chat server this user is leaving */
	char serverOutput [MAX]; /*Reads in output from the server */

	strcpy(s, "C,NONE,Good Bye!\n");

	SSL_write(user->ssl, s, MAX);

	SSL_read(user->ssl, serverOutput, MAX); /*Should read Good Bye! */

	/* For some reason I did notice a bug. when a client does not have a username on the server the server will return Good Bye! correctly when using serverOutput.*/
	/* But if the client has a username on the server the Server will properly close the connection, but return garabage values. */
	printf("\n\n==================================\n");
	printf("Good Bye!\n"); 
	printf("==================================\n");

	/* The client closes connection to the directory */
	close(SSL_get_fd(user->ssl));
	exit(0);
}

/*Signal interrupt for SIGINT*/
void shutdownClient(int sig_num) {
	closeConnection = 1;
}

/*This method is used to initialize the context for SSL connections.*/
SSL_CTX * InitCTX(void) {
	SSL_METHOD * method;
	SSL_CTX *ctx;


	/*Change method to fit this example: https://wiki.openssl.org/index.php/Simple_TLS_Server */
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	method = SSLv23_method();
	ctx = SSL_CTX_new(method);

	if (ctx == NULL) {
		perror("Cannot create context");
		exit(1);
	}

	return ctx;
}

/* This method sends the cert to the client for them to make sure they are connecting to the right server*/
void ShowCerts(SSL* ssl, char* nameOfServer) {
	X509 * cert;
	char *line; //This is what will read through the cert

	cert = SSL_get_peer_certificate(ssl);

	/* Loads the certificate of the chat server */
	if (cert != NULL) {
		line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
		printf("Topic: %s\n", nameOfServer);
		printf("%s\n", line);

		/* Checks if the client is connecting to a legitimate chat server */
		if (strstr(line, nameOfServer) == NULL) {
			perror("Invalid Server!");
		}
		else {
			printf("%s\n", line);
		}

		free(line);
		X509_free(cert);
	}
	else {
		printf("Info: No client certificates configured.\n");
	}
}

/* gets the username from the client */
char * get_username(char * topic) {

	char buff[MAX]; /* Handles gather input from the user */
	char  * username = (char *)malloc(sizeof(char) * 51); //Character names can only be up to 50 characters (We say 51 to include \n


	printf("===========================================\n");
	printf("           Welcome to the");
	printf(" %s Chat Server     \n", topic);
	printf("-------------------------------------------\n");
	printf("Please enter a Username (Up to 50 characters): ");

	//This link showed me how to only get in a specific amount of information from the user https://www.geeksforgeeks.org/taking-string-input-space-c-3-different-methods/
	scanf("%51s", &buff); //Only moves over the first 50 characters of user input and the new line character.
	snprintf(username, 51, "%s", buff); /* Makes sure only the first 50 characters are read into the username */
	
	char c;
	while ((c = getchar()) != '\n' && c != EOF);

	/* How to convert all letters to lower: https://stackoverflow.com/questions/15708793/c-convert-an-uppercase-letter-to-lowercase*/
	for (int i = 0; i < strlen(username); i++) {
		if (username[i] <= 'Z' && username[i] >= 'A') {
			username[i] = username[i] + 32;
		}
	}

	return(username);
}

/* gets the topic from the client */
char * get_topic() {

	char buff[MAX];
	char  * topic = (char *)malloc(sizeof(char) * 51); //Character names can only be up to 50 characters (We say 51 to include \n

	printf("=================================================\n");
	printf("           Welcome to the Chat Server Directory  \n");
	printf("--------------------------------------------------\n");
	printf("Please enter a topic or type L to list all servers (Up to 50 characters): ");

	//This link showed me how to only get in a specific amount of information from the user https://www.geeksforgeeks.org/taking-string-input-space-c-3-different-methods/
	scanf("%s", &buff);
	snprintf(topic, 50, "%s", buff); //Only moves over the first 254 characters of user input and the new line character.
	
	char c;
	while ((c = getchar()) != '\n' && c != EOF);

	/* How to convert all letters to lower: https://stackoverflow.com/questions/15708793/c-convert-an-uppercase-letter-to-lowercase*/
	for (int i = 0; i < strlen(topic); i++) {
		if (topic[i] <= 'Z' && topic[i] >= 'A') {
			topic[i] = topic[i] + 32;
		}
	}

	return(topic);
}



/* Gets a chat message from the user */

char * get_message()
{
	char buff[MAX];
	char  * message = (char *)malloc(sizeof(char) * 255);

	printf("===========================================\n");
	printf("Enter message (Up to 254 characters): ");
	scanf("%s", &buff);
	snprintf(message, 255, "%s\n", buff); //Only moves over the first 254 characters of user input and the new line character.

	char c;
	while ((c = getchar()) != '\n' && c != EOF);

	printf("===========================================\n");
	return(message);
}

/*
 * Read up to "n" bytes from a descriptor.
 * Use in place of read() when fd is a stream socket.
 */

int
readn(fd, ptr, nbytes)
register int	fd;
register char	*ptr;
register int	nbytes;
{
	int	nleft, nread;

	nleft = nbytes;
	while (nleft > 0) {
		nread = read(fd, ptr, nleft);
		if (nread < 0)
			return(nread);		/* error, return < 0 */
		else if (nread == 0)
			break;			/* EOF */

		nleft -= nread;
		ptr += nread;
	}
	return(nbytes - nleft);		/* return >= 0 */
}
