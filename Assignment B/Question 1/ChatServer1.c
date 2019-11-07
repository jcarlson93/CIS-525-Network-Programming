/* Assignment B Question 1 by Nickalas Porsch based off of example 7.
Server for Chat room
	To run type make and then ./server&; ./client
*/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include "inet.h"
#include <sys/socket.h>
#include<stdlib.h> 
#include <unistd.h> 
#include <sys/wait.h>

#define MAX 10000

/*These function definitions help to make sure C goes to the correct functions*/
int close(int);
unsigned int sleep(unsigned int);
pid_t fork(void);
ssize_t write(int, const void*, size_t);
ssize_t read(int, void*, size_t);
void processClient(int);


/*A struct that defines each user name. All the user names are linked together to form a list of users*/
typedef struct User {
	char					username[MAX];
	int						socketId;
	struct User *			nextUser;
} User;


/*A struct that defines each message sent. All the messages are linked together to form a list of messages*/
typedef struct Message {
	char			 message[MAX];
	struct Message * nextMessage;
} Message;

void addUserToList(char username[53], int socketID);
int findUser(char username[53]);
void messageAllUsers(char message[255], int newsockfd);
int removeUser(int removeSock);

main(int argc, char **argv)
{
	int                 sockfd, newsockfd, childpid;
	unsigned int	clilen;
	struct sockaddr_in  cli_addr, serv_addr;
	char                s[MAX];
	char				buff[MAX];
	char				request[MAX];
	int					ret_val;
	int					pipefd[2]; // To allows information from child to pass to parent using a pipe: https://www.geeksforgeeks.org/c-program-demonstrate-fork-and-pipe/
	
	FILE				*fp;

	if (pipe(pipefd) == -1) {
		perror("Pipe Failed");
		exit(1);
	}


	childpid = -1;

	/* Create communication endpoint */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("server: can't open stream socket");
		exit(1);
	}

	/* Bind socket to local address */
	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(SERV_TCP_PORT);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("server: can't bind local address");
		exit(1);
	}

	listen(sockfd, 5);

	for (; ; ) {

		if (childpid != 0) {

			close(pipefd[1]);
			char childMessage[MAX];
			read(pipefd[0], childMessage, sizeof(childMessage));
			messageAllUsers(childMessage, newsockfd);
			close(pipefd[0]);
			printf("%s", childMessage);

			/* Accept a new connection request. */
			clilen = sizeof(cli_addr);
			newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
			if (newsockfd < 0) {
				perror("server: accept error");
				exit(1);
			}

			childpid = fork();
		}	
		else if (childpid == 0) {

			/* Read the request from the client. */
			read(newsockfd, &request, sizeof(char) * 257);
			/* Forgot how to use string tokenizer followed this link: https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/ */
			char * typeOfMessage = strtok(request, ",");
			char * token = strtok(NULL, ",");
			char * username = token;
			token = strtok(NULL, ",");
			char * message = token;
			/* Generate an appropriate reply. */

			switch (typeOfMessage[0]) {

				/*This case registers the username of the client.*/
			case 'U':
				;
				/*This sourced helped with counting line numbers https://www.sanfoundry.com/c-program-number-lines-text-file/*/
				int count = 0;
				char c = ' ';
				
				fp = fopen("users", "r");

				while (c != EOF) {
					c = getc(fp);

					if (c == '\n') {
						count++;
						break;
					}
				}
				fclose(fp);

				/*If no users are in the file then we know that this client is the first one in the chat.*/
				if (count == 0) {

					sprintf(s, "You are the first user in the chat.\n");

					strcpy(buff, message);

					while ((fp = fopen("users", "w+")) == NULL) {
						sleep(1);
					}

					fprintf(fp, "%s %d\n", username, newsockfd);
					fclose(fp);

					//close(pipe[0]);

					//write(pipe[1], s, strlen(s) + 1);
					//close(pipe[1]);
					write(newsockfd, s, MAX);
				}
				else {
					/*Goes through list of users and makes sure the username is not taken.*/

					if (findUser(message) == 0) {
						/*Creates the welcome message for a new user*/
						
						sprintf(s, message);
						strcat(s, " has joined the chat.\n");
						strcpy(buff, message);
						addUserToList(buff, newsockfd);

						close(pipefd[0]);
						write(pipefd[1], s, strlen(s) + 1);
						close(pipefd[1]);
						//messageAllUsers(s, newsockfd);
					}
					else {
						sprintf(s, "Username already taken.\n");
						write(newsockfd, s, MAX);
					}
				}

				printf(s);
				/* Send the reply to the client. */
				
				//close(newsockfd);
				break;

				/*Once a user is registered they can send messages*/
			case 'M':
				;
				strcpy(s, username);

				strcat(s, ": ");
				strcat(s, message);

				close(pipefd[0]);

				write(pipefd[1], s, strlen(s) + 1);
				close(pipefd[1]);
				break;
				/*When the user ends the client we remove the user from the list and end their socket.*/
			case 'Q':
				;
				FILE				*fp;
				char				oldusername[53];
				int					oldsock;
				int found = 0;

				fp = fopen("users", "r");

				while ((fscanf(fp, "%s %d\n", oldusername, &oldsock)) != EOF) {
					if (oldsock == newsockfd) {
						break;
					}
				}

				fclose(fp);

				strcat(s, oldusername);
				strcat(s, ": ");
				strcat(s, message);
				removeUser(message);
				close(pipefd[0]);

				write(pipefd[1], s, strlen(s) + 1);
				close(pipefd[1]);

				close(newsockfd);
				exit(0);
				break;
			default: strcpy(s, "Invalid request\n");
				close(pipefd[0]);

				write(pipefd[1], s, strlen(s) + 1);
				close(pipefd[1]);
				break;
			}

		
		}
			/* Send the reply to the client. */
			//write(newsockfd, s, MAX);
    }
}


/* Registers a User to the Chat room*/
void addUserToList(char username[53], int sockID) {

	FILE				*fp;
		while ((fp = fopen("users", "a")) == 0) {
			sleep(1);
		}

		fprintf(fp, "%s %d\n", username, sockID);
		fclose(fp);
}


/* Returns if the user exists 1 for yes 0 for no*/
int findUser(char username[53]) {
	FILE				*fp;
	char				oldusername[53];
	int					oldsock;
	int found = 0;

	fp = fopen("users", "r");

	while ((fscanf(fp, "%s %d\n", oldusername, &oldsock)) != EOF) {
		if (strcmp(oldusername, username) == 0) {
			printf("%s %s\n", oldusername, username);
			found = 1;
			break;
		}
	}

	fclose(fp);
	return found;
}

/*Send a chat message to all users on the chat server*/
void messageAllUsers(char message[255], int newsockfd) {
	FILE				*fp;
	char				oldusername[53];
	int					sockID;
	int found = 0;

	fp = fopen("users", "r");

	while ((fscanf(fp, "%s\n%d\n", oldusername, &sockID)) != EOF) {
		if ((write(sockID, message, MAX) < 0)) {
			printf("NOT ABLE TO WRITE");
		}
		printf("READING");
	}
}

/*Remove User*/
int removeUser(int removeSock) {
	//Forgot how to mess with linked list so based my code off of this: https://codeforwin.org/2015/09/c-program-to-create-and-traverse-singly-linked-list.html
	User * userList = (User *)malloc(sizeof(User));
	User * pu = userList;

	FILE				*fp;
	char				oldusername[53];
	int					oldsock;
	int found = 0;

	while ((fp = fopen("users", "r+")) == 0) {
		sleep(1);
	}

	while ((fscanf(fp, "%s %d\n", oldusername, &oldsock)) != EOF) {
		strcpy(pu->username, oldusername);
		pu->socketId = oldsock;
		pu->nextUser = (User *)malloc(sizeof(User));
		pu = pu->nextUser;

	}

	fclose(fp);

	pu = userList;

	if (removeSock == pu->socketId) {

		pu = pu->nextUser;
		userList->nextUser = NULL;
		free(userList);
		userList = pu;
	}

	while (pu != NULL) {

		if (removeSock == pu->nextUser->socketId) {
			User * removal = pu->nextUser;

			pu->nextUser = removal->nextUser;
			removal->nextUser = NULL;
			free(removal);
		}
		pu = pu->nextUser;
	}

	while ((fp = fopen("users", "w+")) == 0) {
		sleep(1);
	}

	pu = userList;

	while (pu != NULL) {
		fprintf("%s %d\n", pu->username, pu->socketId);
		pu = pu->nextUser;
	}

	fclose(fp);
}