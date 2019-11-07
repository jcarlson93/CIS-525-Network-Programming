/* Assignment C by Nickalas Porsch based off of previous assignment c.
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

#define MAX 10000

/* Method headers please see below main function */
char * get_message(void);
char * get_username(char *);
char * get_topic(void);
int readn(int, char *, int);

/* needToCloseConnection is used by the sigint handler to figure out which connection needs to be closed.*/
int needToCloseConnection = 0;

/*The File descriptor of the chat server that the client connects to*/
int                 chatfd;

/* The context of the SSL Connection of the chat server the client is connected to.*/
SSL_CTX *			chatCtx = NULL;

/* The SSL Connection that connects the client to the chat server. */
SSL *				chatSsl = NULL;

/* The context of the SSL Connection that connects the directory to the client. */
SSL_CTX *			ctxdir = NULL;

/* The SSL connection that connects the client to the directory */
SSL *				ssldir = NULL;

/*Signal interrupt for SIGINT*/
void sigintHandler(int sig_num) {
	char                s[MAX];

	memset(s, 0, MAX);

	/* Closes the SSL Connection to the directory*/
	if (needToCloseConnection == 1) {
		SSL_shutdown(ssldir);
		exit(0);

	}
	else if(needToCloseConnection == 2){
		/* Closes the connection to the chat server */
		memset(s, 0, MAX);
		strcpy(s, "Q,");
		strcat(s, "NONE");
		strcat(s, ",");
		strcat(s, "Good Bye!");
		SSL_write(chatSsl, s, MAX);

		int nread = 0;

		nread = SSL_read(chatSsl, s, MAX);
		if (nread > 0) {
			printf(s);

		}

		close(chatfd);
		SSL_CTX_free(chatCtx);
		exit(0);
	}
	else {
		/* If no connection is created the client will exit. */
		exit(0);
	}
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
		printf("%s\n",line);

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


int main()
{

	int listenerfd, directoryfd, chatfd, clilen, selection, maxfds, nread, port, retval; /* The first 3 file descriptors are used to connect to listen for new information, connect to the driectory server, and connect to the chat server respectively*/
	struct sockaddr_in	cli_addr, dir_addr, serv_addr; /* All the socket addresses of the client, directory, and chat server */
	char                s[MAX] = { '\0' }; /*String that is used to send information between the server and client*/
	char				username[MAX] = { '\0' }; /* The username of the client on the chat server */
	char				topic[MAX] = "l"; /*The topic of the chat server. It is initally set to l so the client will view all the servers connnected to the directory */
	char				message[MAX] = { '\0' }; /* Any message that is read in from the server that the user is connnected to*/
	char *				chatServer[MAX] = { '\0' }; /*This string will hold the server information for the chat Server*/
	fd_set				readfds; /* File descriptor set that holds all file descriptors that need to be procesed*/

	SSL *				sslchat; /* SSL Connection that connects to the chat server*/

	SSL_CTX *			ctxchat; /* Context for sslchat*/

	/*Initailizing reader integer and the File Descriptor Set, readfds */
	nread = 0; 

	FD_ZERO(&readfds);


	int n, len;
	/* Setting up connection to Directory Server */
	if ((directoryfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("server: can't open stream socket");
		exit(1);
	}


	/* Set up the address of the server to be contacted. */
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
	
	/* If the user needs to close unexpectedly the connection to the directory server will be disconnnected. (Only works sometimes) */
	needToCloseConnection = 1;

	/*Show the certificates from the directory to see if it is legit */
	ShowCerts(ssldir, "directory");
	printf("Showing Certs. \n");
	
	/* Add the directory file descriptor to the file descriptor set*/
	FD_SET(directoryfd, &readfds);


	/* Read in infromation from the directory server */
	nread = SSL_read(ssldir, s, MAX);
	if (nread > 0) {
		printf("%s", s);

	}
	printf("\n--------------------------------------------\n");


	/* Start the interrupt so the connnections can be closed if the user needs to leave unexpectedly. */
	signal(SIGINT, sigintHandler);
	
	/*This while loop keeps the user listing servers until he or she enters a server name.*/
	while (strcmp(topic, "l") == 0) {
		sprintf(topic, get_topic());
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
			/* The user has entered a server name now we check to see if it exists*/
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

	/*The client is not connected to a connection so we set this variable to 0*/
	needToCloseConnection = 0;

	//port = 5925; //These two line are used to help debug the chatServer
	//strcpy(topic, "cats");

	/* We create the chat server context*/
	chatCtx = InitCTX();
	int usernameSet = 0;

	/* Gather Username from user */
	while (usernameSet == 0) {

		strcpy(username, get_username(topic));
		strcpy(s, "U,");
		strcat(s, username);
		strcat(s, ",");
		strcat(s, username); /* This is building the message from the user to say the username and to say a username is being sent */
	
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
		chatSsl = SSL_new(chatCtx);
		SSL_set_fd(chatSsl, chatfd);

		if (SSL_connect(chatSsl) <= 0)			/* perform the connection */
			perror("Unable to connect with SSL."); //ERR_print_errors_fp(stderr);

		/*Show the certs of the server to make sure they are legit*/
		ShowCerts(chatSsl, topic);

		/*The client is now connected to the chatServer so we set this variable to 2 for the interrupt*/
		needToCloseConnection = 2;

		/*See how much is written by the chat server*/
		int numWrote = SSL_write(chatSsl, s, MAX);


		memset(s, 0, MAX);

		/* Read the server's response. and find out if the username is taken */
		nread = SSL_read(chatSsl, s, MAX);
		if (nread > 0) {
			printf(s);
			if (strcmp(s, "Username already taken.\n") == 0) {
				memset(username, 0, MAX);
				
			}
			else {
				usernameSet = 1;
				
			}
		}
	}

	/*
  * We fork here because the user has a username and will need a child process to handle gather messages from other users.
  */
	retval = fork();
	if (retval == -1) {
		perror("Fork failed");
		exit(1);
	}

	for (;;) {

		while (1) {
			
			if (retval == 0) { //Child will check for server responses
		

				/* Read the server's response. */
				nread = SSL_read(chatSsl, s, MAX);

				if (nread > 0) {
					printf("\n\n==================================\n");
					printf(s);
					printf("==================================\n");
				}

			}
			else {
				/* The parent will focus on gathering input from the user to send messages*/
				strcpy(message, get_message());
				strcpy(s, "M,");
				strcat(s, username);
				strcat(s, ",");
				strcat(s, message); /* This is building the message from the user to say the username and to say a username is being sent */

				/* Send the message to server */
				SSL_write(chatSsl, s, sizeof(char) * 257);
				/*257 is 255 (The max size of the message) + the size of "M," which is 2 * sizeof(char)which is 2 * sizeof(char) */
				
				/*Free the memory of the message sent*/
				if (message != NULL) {
					memset(message, 0, MAX);
				}

				memset(s, 0, MAX);

			}



		}
	}

	/* End connection to the chat Server and clean up. (This shouldn't happen)*/
	close(chatfd);
	SSL_free(sslchat);
	SSL_CTX_free(ctxchat);
	return 0;
}


/* gets the username from the client */
char * get_username(char * topic) {

	char buff[MAX];
	char  * username = (char *)malloc(sizeof(char) * 51); //Character names can only be up to 50 characters (We say 51 to include \n


	printf("===========================================\n");
	printf("           Welcome to the");
	printf(" %s Chat Server     \n", topic);
	printf("-------------------------------------------\n");
	printf("Please enter a Username (Up to 50 characters): ");

	//This link showed me how to only get in a specific amount of information from the user https://www.geeksforgeeks.org/taking-string-input-space-c-3-different-methods/
	scanf("%51s", &buff); //Only moves over the first 50 characters of user input and the new line character.
	snprintf(username, 51, "%s", buff);


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
	fgets(topic, sizeof(topic), stdin);
	fflush(stdin);
	topic = strtok(topic, "\n");

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
