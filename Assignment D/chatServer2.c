/* Assignment C by Nickalas Porsch based off of previous assignment b.
Chat Server
	To run type make and then ./directory&; Then start an indvidual server  by doing ./server PORT TOPIC and then create a client with ./client
	Where you provide the PORT and TOPIC

	name of server is stored in the Common Name of the cert  

	There are 3 server PEM files which allow 3 different chatrooms to be created.
	Topic: cats, cows, dogs which each have certificate of catsPEM, cowsPEM, and dogsPEM
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include "inet.h"
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <signal.h>
#include <pthread.h> /*For help on threads I used this example: https://www.geeksforgeeks.org/multithreading-c-2/*/

/* SSL Information was based off of this: https://aticleworld.com/ssl-server-client-using-openssl-in-c/ */
/* IBM also has info on SSL: https://developer.ibm.com/tutorials/l-openssl/ */

#define MAX 10000
/* This is the max number of SSL Connection that can be connected to this server*/
#define CONNECTIONS 100
int closeDirectoryConnection = 0;
SSL_CTX *			ctxdir; /* Creates a context for SSL Connection to directory*/

SSL *				ssldir; /* Creates an ssl connection to directory */
char *				topic;

/* A struct that defines each user name. All the user names are linked together to form a list of users */
typedef struct User {
	char					username[MAX];
	int						socketId;
	SSL *					ssl;
} User;

typedef struct Listener {
	User * userList;
	SSL ** sslConnection;
	int listenfd;
	char * certName;
	struct sockaddr_in client_addr;

} Listener;

typedef struct ClientThread {
	User *	userList;
	SSL **	sslConnection;
	int		sslIndex;

} ClientThread;



/*Method definitions please see below Main for each method's purpose*/
SSL_CTX * InitServerCTX(void);
SSL_CTX * InitCTX(void);
void LoadCerts(SSL_CTX *ctx, char * CertFile, char *KeyFile);
void ShowCerts(SSL * ssl);
void CheckCerts(SSL* ssl, char* nameOfServer);
int findUser(User * userList,char username[53]);
void messageAllUsers(User * userList, char message[255]);
void removeUser(User * userList, char username[53]);
void newConnection(Listener * listenerData);
void clientRequest(ClientThread * clientThreadData);
void endDirectoryConnection();

/* For using select I decided to use this link https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/ */

void main(int argc, char **argv)
{
	int                 sockfd, newsockfd, directoryfd, childpid, nread, maxsocketdesc, selection, maxfds ;
	SSL*				ssl; /* Used for listener socket*/

	SSL **				sslConnection; /*List of all SSL Connections*/
	struct sockaddr_in	dir_addr; /* Used for connection to the directory */
	unsigned int	clilen;
	struct sockaddr_in  cli_addr, serv_addr; /* Used for handling client connections */
	char                s[MAX];
	char				buff[MAX];
	char				request[MAX]; /* Incoming message from a connection */
	User *				userList; /* List of all users connected to the server*/
	fd_set				readfds; /* List of all file descriptors */
	SSL_CTX *			ctx; /* Used for establishing context for listener connection*/

	Listener *			listenerData;
	pthread_t			listenerThread;


	listenerData = (Listener *)malloc(sizeof(Listener));

	sslConnection = NULL;

	if (argc < 3) {
		perror("Need two arguments PORT TOPIC.\n");
		exit(1);
	}


	/* Setting up */
	char * portAsString = argv[1];
	topic = argv[2];
	char * dirPortAsString;
	int dirPort = 0;

	if (argc == 4) {
		dirPortAsString = argv[3];
		/* How to convert an string to int*/
		for (int i = 0; i < strlen(dirPortAsString); i++) {
			dirPort = dirPort * 10 + (dirPortAsString[i] - '0');
		}
	}


	int port = 0;


	/* How to convert an string to int*/
	for (int i = 0; i < strlen(portAsString); i++) {
		port = port * 10 + (portAsString[i] - '0');
	}

	/* Initializing OpenSSL */
	SSL_library_init();
	ctx = InitServerCTX();

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

	if (connect(directoryfd, (struct sockaddr *) &dir_addr, sizeof(dir_addr)) < 0) {
		perror("Connection error to directory.\n");
		exit(1);
	}

	printf("Connecting to Directory");
	/* Creating SSL Connection to Directory*/
	SSL_library_init();

	ctxdir = InitCTX();
	ssldir = SSL_new(ctxdir);
	SSL_set_fd(ssldir, directoryfd);


	if (SSL_connect(ssldir) <= 0) {
		ERR_print_errors_fp(stdout);
		exit(1);
	}


	CheckCerts(ssldir, "directory");
	printf("Checking directory Certs. \n");

	/* Checks to see if we are connected to the directory*/
	nread = SSL_read(ssldir, s, MAX);
	if (nread > 0) {
		printf("%s", s);

	}
	printf("\n--------------------------------------------\n");
	
	/* Send of server data to directory */
	sprintf(s, "S,%s,%d", topic, port);

	SSL_write(ssldir, s, MAX);

	nread = SSL_read(ssldir, s, MAX);
	if (nread > 0) {
		printf("%s", s);

	}
	printf("\n--------------------------------------------\n");


	/* Start the interrupt so the connnections can be closed if the user needs to leave unexpectedly. */
	signal(SIGINT, endDirectoryConnection);

	/* This finds the correct cert for the chatroom*/
	char certName[MAX];
	strcpy(certName, "./");
	strcat(certName, topic);
	strcat(certName, "PEM");



	/* Making sure everything is set to NULL/zeroing variables */
	userList = calloc(CONNECTIONS, sizeof(User));
	sslConnection = calloc(CONNECTIONS, sizeof(SSL*));

	for (int i = 0; i < CONNECTIONS; i++) {
		userList[i].socketId = -1;
		userList[i].ssl = NULL;
		strcpy(userList[i].username, "");
		sslConnection[i] = NULL;
	}

	printf("Creating listening socket\n");
	/* Create communication endpoint (master socket)*/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("server: can't open stream socket");
		exit(1);
	}


	/* Bind socket to local address */
	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("server: can't bind local address");
		exit(1);
	}

	if (listen(sockfd, 5) < 0) {
		perror("Too many connections to listen too");
		exit(1);
	}

	printf("Creating SSL Connection");
	ctx = InitServerCTX();
	LoadCerts(ctx, certName, certName);
	ssl = SSL_new(ctx);

	/* Adding listener to list of SSL Connections*/
	SSL_set_fd(ssl, sockfd);
	sslConnection[0] = ssl;
	ShowCerts(sslConnection[0]);


	listenerData->certName = certName;
	listenerData->listenfd = sockfd;
	listenerData->sslConnection = sslConnection;
	listenerData->userList = userList;
	listenerData->client_addr = cli_addr;

	/* Creating threads for listener*/
	/* The threadInfo object will hold all the information that is needed for the listener to perform its duties.*/
	if ((pthread_create(&listenerThread, NULL, newConnection, listenerData) != 0)) {
		perror("Creating recieve Message Thread failed!\n");
		exit(1);
	}

	printf("Before for loop");
	while (closeDirectoryConnection == 0) {

	}

	/* Set up the address of the server to be contacted. */

	strcpy(buff, "Q,");
	strcat(buff, topic);
	strcat(buff, ",");
	strcat(buff, topic);
	SSL_write(ssldir, buff, MAX);
	close(directoryfd);
	SSL_CTX_free(ctxdir);
	close(sockfd);
	SSL_CTX_free(ssl);
	return 0;
}

void endDirectoryConnection() {
	char s[MAX];

	strcpy(s, "Q,");
	strcat(s, topic);
	strcat(s, ",");
	strcat(s, topic);
	SSL_write(ssldir, s, MAX);

	/* The client closes connection to the directory */
	close(SSL_get_fd(ssldir));
	SSL_free(ssldir);
	SSL_CTX_free(ctxdir);
	exit(0);
}

/* This method is used to load the server's certificate (The CertFile and KeyFile are the same file)*/
void LoadCerts(SSL_CTX *ctx, char * CertFile, char *KeyFile) {

	if (SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0) {
		perror("Cannot load certificate.");
		exit(1);
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0) {
		perror("Cannot gather private key from Key file");
		exit(1);
	}

	if (!SSL_CTX_check_private_key(ctx)) {
		perror("Private Key does not match public cert.");
		exit(1);
	}

}

/* This method sends the cert to the client for them to make sure they are connecting to the right server*/
void ShowCerts(SSL * ssl) {
	X509 * cert;
	char *line;

	cert = SSL_get_peer_certificate(ssl);
	if (cert != NULL) {
		line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
		printf("Subject: %s\n", line);
		free(line);
		X509_free(cert);
	}
	else {
		printf("No Cert!!!\n");
	}
}

/* Check the directory server's certs */
void CheckCerts(SSL* ssl, char* nameOfServer) {
	X509 * cert;
	char *line; //This is what will read through the cert

	cert = SSL_get_peer_certificate(ssl);

	if (cert != NULL) {
		line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
		printf("Topic: %s\n", nameOfServer);
		printf("%s\n", line);
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


/*This method is used to initialize the context for SSL connections.*/
SSL_CTX * InitServerCTX(void) {
	SSL_METHOD * method;
	SSL_CTX *ctx;

	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	method = SSLv23_server_method();
	ctx = SSL_CTX_new(method);

	if (ctx == NULL) {
		perror("Cannot create context");
		exit(1);
	}

	return ctx;
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

void newConnection(Listener * listenerData) {
	unsigned int		clilen;
	int					newsockfd;
	SSL_CTX *			newctx; /* Used for incoming SSL Connections */
	SSL *				newssl; /* Used for incoming SSL Connections */
	pthread_t			connectionThread;
	ClientThread *		clientThreadData;

	

	for (;;)
	{
		clientThreadData = (ClientThread *)malloc(sizeof(ClientThread));
		/* Accept a new connection request. */
		printf("ACCEPTING");
		clilen = sizeof(listenerData->client_addr);
		newsockfd = accept(listenerData->listenfd, (struct sockaddr *) &(listenerData->client_addr), &clilen);


		/* Once a new accept has happened we create a new SSL connection using the ctx context*/
		newctx = InitServerCTX();
		LoadCerts(newctx, listenerData->certName, listenerData->certName);

		newssl = SSL_new(newctx);
		SSL_set_fd(newssl, newsockfd);

		/* try to accept using ssl */
		if (SSL_accept(newssl) <= 0) {
			ERR_print_errors_fp(stdout);
		}
		
		ShowCerts(newssl);
		printf("AFTER CERT");

		/* Find an open SSL Connection in the list to add the new connection */
		for (int index = 0; index < CONNECTIONS; index++) {
			if (listenerData->sslConnection[index] == NULL) {
				listenerData->sslConnection[index] = newssl;
				printf("ADDING NEW CONNECTION\n");
				clientThreadData->sslConnection = listenerData->sslConnection;
				clientThreadData->sslIndex = index;
				clientThreadData->userList = listenerData->userList;
				

				if ((pthread_create(&connectionThread, NULL, clientRequest, clientThreadData) != 0)) {
					perror("Creating recieve Message Thread failed!\n");
					exit(1);
				}

				break;
			}
		}


	}
}

void clientRequest(ClientThread * clientThreadData) {
	char		request[MAX];
	char		s[MAX];
	char		buff[MAX];
	printf("\nENTERED CLIENT THREAD\n");
	for (;;) {
		SSL_read(clientThreadData->sslConnection[clientThreadData->sslIndex], request, MAX);

		printf("Reading Request\n");
		printf("%s\n\n", request);
		/* Forgot how to use string tokenizer followed this link: https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/ */
		char * typeOfMessage = strtok(request, ",");


		char * token = strtok(NULL, ",");
		char * username = token;

		token = strtok(NULL, ",");
		char * message = token;

		/* Generate an appropriate reply. */

		int currentfd = SSL_get_fd(clientThreadData->sslConnection[clientThreadData->sslIndex]);

		switch (typeOfMessage[0]) {

			/*This case registers the username of the client.*/
		case 'U':
			;
			int firstUser = 1;
			for (int index = 0; index < CONNECTIONS; index++) {
				if (clientThreadData->userList[index].socketId != -1) {
					firstUser = 0;
					break;
				}
			}
			/*If no users are in the file then we know that this client is the first one in the chat.*/
			if (firstUser == 1) {

				sprintf(s, "You are the first user in the chat.\n");
				strcpy(clientThreadData->userList[0].username, username);
				clientThreadData->userList[0].socketId = currentfd;
				clientThreadData->userList[0].ssl = clientThreadData->sslConnection[clientThreadData->sslIndex];
				printf("FIRST USER\n");
				printf("%s", s);

				if (SSL_write(clientThreadData->userList[0].ssl, s, MAX) <= 0) {
					ERR_print_errors_fp(stdout);
				}
				printf("Written.\n");

			}
			else {
				/*Goes through list of users and makes sure the username is not taken.*/

				if (findUser(clientThreadData->userList, username) == 0) {
					/*Creates the welcome message for a new user*/
					sprintf(s, message);
					strcat(s, " has joined the chat.\n");

					/* Adds the new user to the userList*/
					for (int index = 0; index < CONNECTIONS; index++) {
						if (clientThreadData->userList[index].socketId == -1) {
							clientThreadData->userList[index].ssl = clientThreadData->sslConnection[clientThreadData->sslIndex];
							strcpy(clientThreadData->userList[index].username, username);
							clientThreadData->userList[index].socketId = currentfd;
							break;
						}
					}


					strcpy(buff, message);
					messageAllUsers(clientThreadData->userList, s);

					printf("NEWER USER\n");
				}
				else {
					sprintf(s, "Username already taken.\n");
					SSL_write(clientThreadData->sslConnection[clientThreadData->sslIndex], s, MAX);

				}
			}

			/* Send the reply to the client. */

			break;

			/*Once a user is registered they can send messages*/
		case 'M':
			;
			strcpy(s, username);

			strcat(s, ": ");
			strcat(s, message);
			printf("GOT A MESSAGE");
			printf("%s", s);
			messageAllUsers(clientThreadData->userList, s);
			break;
			/*When the user ends the client we remove the user from the list and end their socket.*/
		case 'C':
			;
			printf("SOMEONE WANTS TO QUIT\n\n");
			int found = 0;
			/* Go through the userlist and remove the user*/
			for (int index = 0; index < CONNECTIONS; index++) {
				if (clientThreadData->userList[index].socketId == clientThreadData->sslIndex) {
					strcpy(s, clientThreadData->userList[index].username);
					printf("REMOVING USER\n");
					strcat(s, ": ");
					strcat(s, message);
					printf("%s", s);
					messageAllUsers(clientThreadData->userList, s);
					close(currentfd);
					SSL_CTX_free(SSL_get_SSL_CTX(clientThreadData->userList[index].ssl));
					strcpy(clientThreadData->userList[index].username, "");
					clientThreadData->userList[index].socketId = -1;

				}
			}
			
			SSL_free(clientThreadData->sslConnection[clientThreadData->sslIndex]);
			/* Check to see if we have no users*/
			int emptyServer = 1;
			for (int index = 0; index < CONNECTIONS; index++) {
				if (clientThreadData->userList[index].socketId > 0) {
					emptyServer = 0;
					break;
				}
			}

			/*Close down the Server and remove it from directory*/
			if (emptyServer == 0) {
				closeDirectoryConnection = 1;
			}


			pthread_exit(0);
			break;
		default: strcpy(s, "Invalid request\n");
			SSL_write(clientThreadData->userList[clientThreadData->sslIndex].ssl, s, MAX);
			break;
		}
	}
}


/*Send a chat message to all users on the chat server*/
void messageAllUsers(User * userList, char message[255]) {

	for (int i = 0; i < CONNECTIONS; i++) {
		if (userList[i].socketId > 0) {
			SSL_write(userList[i].ssl, message, MAX);
		}
	}

}

/*Remove User*/
void removeUser(User * userList, char username[53]) {
	for (int i = 0; i < CONNECTIONS; i++) {
		if (strcmp(userList[i].username, username) == 0) {
			strcpy(userList[i].username, "");
			userList[i].socketId = -1;
			userList[i].ssl = NULL;
			break;
		}
	}	
	
}

/* Check to see if a user exists in the userlist if given their username.*/
int findUser(User * userList, char username[53]) {
	int found = 0;
	printf("Searching for user.\n");
	for (int i = 0; i < CONNECTIONS; i++) {
		if (strcmp(userList[i].username, username) == 0) {
			found = 1;
			printf("FOUND\n");
			break;
		}
	}
	return found;
}