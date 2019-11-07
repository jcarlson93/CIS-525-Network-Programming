/* Assignment B Question 2 by Nickalas Porsch based off of example 7.
Client for Chat room
	To run type make and then ./directory&; ./client;
	To start an individual chat server run ./server IP:PORT TOPIC;
	Where you provide the IP, PORT, and TOPIC
*/
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "inet.h"
#include <signal.h>

#define MAX 10000

char * get_message(void);
char * get_username(void);
char * get_topic(void);
int readn(int, char *, int);

int needToShutdown = 0;
int                 sockfd;

/*Signal interrupt for SIGINT*/
void sigintHandler(int sig_num) {
	char                s[MAX];
	strcpy(s, "Q,");
	strcat(s, "NONE");
	strcat(s, ",");
	strcat(s, "Good Bye!");
	write(sockfd, s, sizeof(char) * 257);
	exit(0);
}



/*Main Function*/
main(int argc, char **argv)
{

	struct sockaddr_in  serv_addr;
	char                s[MAX];      /* array to hold output */
	int                 response;    /* user response        */
	int                 nread;       /* number of characters */
	int					usernameSet = 0;  /* makes sure the client has a username for the server that is unique */
	char *				username;
	char *				message;
	char *				topic;
	int					retval;
	int					foundServer = 0; /*Makes sure the client is connected to a server*/

/* Set up the address of the server to be contacted. */
	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
	serv_addr.sin_port = htons(SERV_TCP_PORT);

	topic = get_topic();
	strcpy(s, "N,");
	strcat(s, topic);
	strcat(s, ",");
	strcat(s, "NONE"); /* This is building the message from the user to say the username and to say a username is being sent */

	/* Create a socket (an endpoint for communication). */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("client: can't open stream socket");
		exit(1);
	}

	/* Connect to the server. */
	if (connect(sockfd, (struct sockaddr *) &serv_addr,
		sizeof(serv_addr)) < 0) {
		perror("client: can't connect to server");
		exit(1);
	}

	/* Send the username to server */
	write(sockfd, s, sizeof(char) * 53); //53 is 51 (The max size of the username) + the size of "U," which is 2 * sizeof(char)

	memset(s, 0, MAX);

	/* Read the server's response. */
	nread = readn(sockfd, s, MAX);
	if (nread > 0) {
		printf(s);
		free(topic);
	}

	/* Gather Username from user */
	while (usernameSet == 0) {

		username = get_username();
		strcpy(s, "U,");
		strcat(s, username);
		strcat(s, ",");
		strcat(s, username); /* This is building the message from the user to say the username and to say a username is being sent */

		/* Create a socket (an endpoint for communication). */
		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("client: can't open stream socket");
			exit(1);
		}

		/* Connect to the server. */
		if (connect(sockfd, (struct sockaddr *) &serv_addr,
			sizeof(serv_addr)) < 0) {
			perror("client: can't connect to server");
			exit(1);
		}

		/* Send the username to server */
		write(sockfd, s, sizeof(char) * 53); //53 is 51 (The max size of the username) + the size of "U," which is 2 * sizeof(char)

		memset(s, 0, MAX);

		/* Read the server's response. */
		nread = readn(sockfd, s, MAX);
		if (nread > 0) {
			printf(s);
			if (strcmp(s, "Username already taken.\n") == 0) {
				free(username);
				close(sockfd);
			}
			else {
				usernameSet = 1;
				signal(SIGINT, sigintHandler);
			}
		}
	}

	/*
  * Fork off child processes.
  */
	retval = fork();
	if (retval == -1) {
		perror("Fork failed");
		exit(1);
	}

	for (;;) {

		while (1) {
			if (retval == 0) { //Child will check for server responses
				//if (needToShutdown == 1) {
				//	exit(0);
				//}

				/* Read the server's response. */
				nread = readn(sockfd, s, MAX);
				if (nread > 0) {
					printf("\n");
					printf(s);
					printf("\n");

				}

			}
			else {
				message = get_message();
				strcpy(s, "M,");
				strcat(s, username);
				strcat(s, ",");
				strcat(s, message); /* This is building the message from the user to say the username and to say a username is being sent */

				/* Send the message to server */
				write(sockfd, s, sizeof(char) * 257); //257 is 255 (The max size of the message) + the size of "M," which is 2 * sizeof(char)which is 2 * sizeof(char)
				/*Free the memory of the message sent*/
				if (message != NULL) {
					free(message);
				}

				memset(s, 0, MAX);

			}



		}
	}
	close(sockfd);
	exit(0);
}

/* gets the username from the client */
char * get_username() {

	char buff[MAX];
	char  * username = (char *)malloc(sizeof(char) * 51); //Character names can only be up to 50 characters (We say 51 to include \n


	printf("===========================================\n");
	printf("           Welcome to the Chat Server     \n");
	printf("-------------------------------------------\n");
	printf("Please enter a Username (Up to 50 characters): ");

	//This link showed me how to only get in a specific amount of information from the user https://www.geeksforgeeks.org/taking-string-input-space-c-3-different-methods/
	fgets(username, sizeof(username), stdin);
	fflush(stdin);
	username = strtok(username, "\n");
	return(username);
}

/* gets the topic from the client */
char * get_topic() {

	char buff[MAX];
	char  * topic = (char *)malloc(sizeof(char) * 51); //Character names can only be up to 50 characters (We say 51 to include \n


	printf("=================================================\n");
	printf("           Welcome to the Chat Server Directory  \n");
	printf("--------------------------------------------------\n");
	printf("Please enter a topic (Up to 50 characters): ");

	//This link showed me how to only get in a specific amount of information from the user https://www.geeksforgeeks.org/taking-string-input-space-c-3-different-methods/
	fgets(topic, sizeof(topic), stdin);
	fflush(stdin);
	topic = strtok(topic, "\n");
	return(topic);
}

/* Gets a chat message from the user */

char * get_message()
{
	char  * message = (char *)malloc(sizeof(char) * 255);

	printf("===========================================\n");
	printf("Enter message (Up to 254 characters): ");
	fgets(message, sizeof(message), stdin); //Only moves over the first 254 characters of user input and the new line character.
	message = strtok(message, "\n");
	fflush(stdin);
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
