/* Assignment C by Nickalas Porsch based off of previous assignment c.
Chat directory
		To run type make and then ./directory&; Then start an indvidual server  by doing ./server PORT TOPIC and then create a client with ./client
	Where you provide the PORT and TOPIC

	Client does not have a SSL certificate
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
#include <pthread.h> /*For help on threads I used this example: https://www.geeksforgeeks.org/multithreading-c-2/*/

#define MAX 10000
#define CONNECTIONS 100

int close(int);
unsigned int sleep(unsigned int);
pid_t fork(void);
ssize_t write(int, const void*, size_t);
ssize_t read(int, void*, size_t);
void processClient(int);

/* A struct that defines each server. All the servers are linked together to form a list of servers */
typedef struct Server {
	char					topic[MAX];
	int						port, sockfd;
	SSL *					ssl;
} Server;


typedef struct Listener {
	Server * serverList;
	SSL ** sslConnection;
	int listenfd;
	struct sockaddr_in cli_addr;

} Listener;

typedef struct ClientThread {
	Server *	serverList;
	SSL **	sslConnection;
	int		sslIndex;

} ClientThread;


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
	printf("Created a context.\n");
	return ctx;
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
	printf("Loaded Certs.\n");
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

/* Method declaration for the method to add servers to the directory's server list */
void addServerToList(Server * serverList, char topic[53], int sockfd, int maxfds);

/* Method declaration for the method to help the directory find servers*/
int findServer(Server * serverList, char topic[53]);

/* Method declaration for removing an SSL Connection*/
void removeSSLConnection(int fd, SSL ** sslList);

void newConnection(Listener * listenerData);

void clientRequest(ClientThread * clientThreadData);


int main()
{
	int                 listenerfd, newsockfd, clilen, childpid, selection, maxfds, port, read_blocked, accept_blocked;
	struct sockaddr_in  cli_addr, serv_addr, chat_addr;
	char                s[MAX] = { '\0' };
	char				request[MAX] = { '\0' };
	Server *			serverList;
	SSL **				sslConnection;
	SSL_CTX *			ctx;
	SSL_CTX *			newctx;
	SSL *				ssl;
	SSL *				newssl;
	Listener *			listenerData;
	pthread_t			listenerThread;

	/* Initializing OpenSSL */
	SSL_library_init();

	/*Initializes the lists that will hold server information and ssl connections respectively*/
	serverList = calloc(CONNECTIONS, sizeof(Server));
	sslConnection = calloc(CONNECTIONS, sizeof(SSL *));

	for (int i = 0; i < CONNECTIONS; i++) {
		serverList[i].port = -1;
		serverList[i].sockfd = -1;
		serverList[i].ssl = NULL;
		strcpy(serverList[i].topic, "\0");
	}


	/* Create communication endpoint (master socket)*/
	if ((listenerfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("server: can't open stream socket");
		exit(1);
	}

	/* Bind socket to local address */
	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(SERV_TCP_PORT);

	if (bind(listenerfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("server: can't bind local address");
		exit(1);
	}

	if (listen(listenerfd, 5) < 0) {
		perror("Too many connections to listen too");
		exit(1);
	}

	/* Creates a new context for the listener SSL Connection*/
	ctx = InitServerCTX();

	/* Loads the directory's certificate for SSL connection into the listener's context*/
	LoadCerts(ctx, "./directoryPEM", "./directoryPEM");

	/* Creates the listener's ssl connection*/
	ssl = SSL_new(ctx);
	/* Adding listener to list of SSL Connections*/
	sslConnection[0] = ssl;
	ShowCerts(sslConnection[0]);
	printf("SHOWN CERTS");
	listenerData = (Listener *) malloc(sizeof(Listener));
	
	listenerData->listenfd = listenerfd;
	listenerData->serverList = serverList;
	listenerData->sslConnection = sslConnection;
	listenerData->cli_addr = cli_addr;

	if ((pthread_create(&listenerThread, NULL, newConnection, listenerData) != 0)) {
		perror("Creating Listener thread failed!\n");
		exit(1);
	}

	while (1) {

	}


}

void newConnection(Listener * listenerData) {
	unsigned int		clilen;
	int					newsockfd;
	SSL_CTX *			newctx; /* Used for incoming SSL Connections */
	SSL *				newssl; /* Used for incoming SSL Connections */
	pthread_t			connectionThread;
	ClientThread *		clientThreadData;
	char				s[MAX];

	for (;;) {
		clientThreadData = (ClientThread *)malloc(sizeof(ClientThread));
		/* The listener recieves a connection and creates a new file descriptor*/
		printf("Accepting new connnection.\n");

		
		clilen = sizeof(listenerData->cli_addr);

		/* fd of new connection */
		newsockfd = accept(listenerData->listenfd, (struct sockaddr *) &listenerData->cli_addr, &clilen);
		if (newsockfd < 0) {
			perror("Failed to do a normal accept.\n");
			exit(1);
		}

		/* Loads the directory's certificate into the new SSL Connection*/
		newctx = InitServerCTX();
		LoadCerts(newctx, "./directoryPEM", "./directoryPEM");
		newssl = SSL_new(newctx);

		if (newssl == NULL) {
			ERR_print_errors_fp(stdout);
			exit(1);
		}

		/* Adding new fd to list of SSL Connections*/
		SSL_set_fd(newssl, newsockfd);

		/* Find a place in the SSL Connections array that is empty*/
		for (int index = 0; index < CONNECTIONS; index++) {
			printf("ENTERING FOR LOOP.\n");
			if (listenerData->sslConnection[index] == NULL) {

				/* Add the new SSL Connection to the array*/
				listenerData->sslConnection[index] = newssl;
				ShowCerts(listenerData->sslConnection[index]);

				if (SSL_accept(listenerData->sslConnection[index]) <= 0) {
					ERR_print_errors_fp(stdout);
					exit(1);
				}

				/* Connection is now connected to the directory */
				strcpy(s, "Connected to Directory Server!");
				printf("Accepted connection on index: %d\n", index);

				clientThreadData->sslConnection = listenerData->sslConnection;
				clientThreadData->sslIndex = index;
				clientThreadData->serverList = listenerData->serverList;

				SSL_write(newssl, s, MAX);

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
	int			port;

	for (;;) {
		/* Checks to see if the current file descriptor needs to be processed*/
		printf("Reading Request\n");

		/* Read in the request */
		if (SSL_read(clientThreadData->sslConnection[clientThreadData->sslIndex], request, MAX) <= 0) {
			printf("Unable to read\n");
		}


		printf("%s\n", request);

		/* Forgot how to use string tokenizer followed this link: https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/ */
		char * typeOfMessage = strtok(request, ",");
		char * token = strtok(NULL, ",");
		char * username = token;
		token = strtok(NULL, ",");
		char * topic = token;

		/* Generate an appropriate reply. */

		switch (typeOfMessage[0]) {

			/*This case finds the server info for the user.*/
		case 'N':
			;

			int isfound = findServer(clientThreadData->serverList, topic);

			if (isfound == 1) {
				/*Joins the already created server.*/
				sprintf(s, "Joining Server.\n");
				printf("Connecting to server");

				for (int index = 0; index < CONNECTIONS; index++) {
					if (strcmp(clientThreadData->serverList[index].topic, topic) == 0) {
						port = clientThreadData->serverList[index].port;
						break;
					}
				}

				sprintf(s, "Server found at port:%d", port);

				/* Send server information to client */
				SSL_write(clientThreadData->sslConnection[clientThreadData->sslIndex], s, MAX);
				removeSSLConnection(clientThreadData->sslIndex, clientThreadData->sslConnection);

			}
			else {

				/* A server could not be found and tell user*/
				sprintf(s, "Server not Found.");
				SSL_write(clientThreadData->sslConnection[clientThreadData->sslIndex], s, MAX);
			}

			break;

			/* This case is so the user can see all servers connnected to the directory*/
		case 'L':
			;
			memset(s, 0, MAX);

			/* Go through the list of servers and send them to the user */
			for (int i = 0; i < CONNECTIONS; i++) {
				if (strcmp(clientThreadData->serverList[i].topic, "") != 0) {
					strcat(s, clientThreadData->serverList[i].topic);
					strcat(s, "\n");
				}
				printf("GOING THROUGH INDEX: %d", &index);
			}

			SSL_write(clientThreadData->sslConnection[clientThreadData->sslIndex], s, MAX);

			break;

			/* This case is if a user needs to be removed from the directory (Tends to cause segfaults) */
		case 'C':
			;
			strcpy(s, "Good Bye!");
			SSL_write(clientThreadData->sslConnection[clientThreadData->sslIndex], s, MAX);
			removeSSLConnection(clientThreadData->sslIndex, clientThreadData->sslConnection);

			break;

			/* This case s if a server is created and connects to the directory*/
		case 'S':
			;

			char * nameofServer = username;

			port = 0;
			memset(s, 0, MAX);

			/* Find an empty spot in the ServerList to add the server */

			for (int index = 0; index < CONNECTIONS; index++) {
				if (strcmp(clientThreadData->serverList[index].topic, "") == 0) {
					strcpy(clientThreadData->serverList[index].topic, nameofServer);
					for (int digit = 0; digit < strlen(topic); digit++) {
						port = port * 10 + (topic[digit] - '0');
					}

					clientThreadData->serverList[index].port = port;

					/*Send over a confirmation to the server that the connection to the directory has been made */
					strcpy(s, "Created Server on Directory");
					SSL_write(clientThreadData->sslConnection[clientThreadData->sslIndex], s, MAX);
					printf("Server created at index: %d and on port %d\n\n\n", index, clientThreadData->serverList[index].port);
					break;

				}
			}

			break;

			/* This case is if a server needs to disconnect and needs to be removed from Server List */
		case 'Q':
			;

			/* This function removes the current socket being processed from the list of SSL Connections */
			removeSSLConnection(clientThreadData->sslIndex, clientThreadData->sslConnection);

			/* The below look removes the server from the server list */
			int index = 0;
			while (index < 100) {

				if (strncmp(topic, clientThreadData->serverList[index].topic, strlen(clientThreadData->serverList[index].topic)) == 0) {
					clientThreadData->serverList[index].port = -1;
					clientThreadData->serverList[index].sockfd = -1;
					clientThreadData->serverList[index].ssl = NULL;
					strcpy(clientThreadData->serverList[clientThreadData->sslIndex].topic, "");
					bzero(clientThreadData->serverList[index].topic, strlen(clientThreadData->serverList[index].topic));
					break;
				}

				index++;
			}

			break;

			/* This case is if someone enters a type of message that cannot be processed (This will not occur by the client or server) */
		default: strcpy(s, "Invalid request\n");
			SSL_write(clientThreadData->sslConnection[clientThreadData->sslIndex], s, MAX);

			break;

		}
	}
}

/* Returns if the server exists returns either NULL or the server info*/
int findServer(Server * serverList, char topic[53]) {
	int					found = 0;
	char				serverTopic[MAX];
	int					port, sockd; /*These are you to hold on to the chat server's ip*/
	int					* serverInfo = (int *)malloc(sizeof(int) * 5);


	printf("FINDING SERVER");
	printf("%s", topic);
	for (int i = 0; i < CONNECTIONS; i++) {
		if(serverList[i].port != -1)
			printf("%s", serverList[i].topic);
		if (strcmp(serverList[i].topic, topic) == 0) {
			printf("FOUND");
			found = 1;
			break;
		}
	}

	return found;
}

/* This method removes an SSL connection from the list of SSL Connections */

void removeSSLConnection(int index, SSL ** sslConnection) {

	printf("REMOVING SSL Connection");

	/*Shutsdown the SSL Connection */
	SSL_shutdown(sslConnection[index]);
	
	/* Frees the context of the SSL Connection */
	SSL_CTX_free(SSL_get_SSL_CTX(sslConnection[index]));
	
	/* Frees the actual SSL Connection*/
	free(sslConnection[index]);
	sslConnection[index] = NULL;

}