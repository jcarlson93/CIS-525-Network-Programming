/* Assignment B Question 1 by Nickalas Porsch based off of example 7.
Server for Chat room
	To run type make and then ./dirserver&; ./client;
	To start an individual chat server run ./directory IP:PORT TOPIC;
	Where you provide the IP, PORT, and TOPIC
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include "inet.h"
#include <sys/socket.h>




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

char				filename[MAX];

main(int argc, char **argv)
{


	int                 sockfd, newsockfd, childpid;
	unsigned int	clilen;
	struct sockaddr_in  cli_addr, serv_addr;
	char                s[MAX];
	char				buff[MAX];
	char				request[MAX];
	int					ret_val;

	FILE				*fp;

	if (argc < 3) {
		perror("Only need two arguments IP:PORT TOPIC.\n");
		exit(1);
	}


	//IP
	char * ip = strtok(argv[1], ": ");
	char * port = strtok(NULL, ": ");
	char * topic = argv[2];

	printf("%s\n", ip);
	printf("%s\n", port);
	printf("%s\n", topic);

	strcpy(filename, topic);
	strcat(filename, "users");

	childpid = -1;

	/* Create communication endpoint */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("server: can't open stream socket");
		exit(1);
	}

	/* Bind socket to local address */
	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(ip);
	serv_addr.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("server: can't bind local address");
		exit(1);
	}

	listen(sockfd, 5);

	for (; ; ) {

		if (childpid != 0) {
			/* Accept a new connection request. */
			clilen = sizeof(cli_addr);
			newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
			if (newsockfd < 0) {
				perror("server: accept error");
				exit(1);
			}

			childpid = fork();
		}
		
		if (childpid == 0) {
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
				
				fp = fopen(filename, "r");

				while (c != EOF) {
					c = getc(fp);

					if (c == '\n') {
						count++;
						break;
					}
				}
				fclose(fp);

				fclose(fp);
				/*If no users are in the file then we know that this client is the first one in the chat.*/
				if (count == 0) {
					sprintf(s, "You are the first user in the ");
					strcat(s, topic);
					strcat(s, " chat.\n");

					strcpy(buff, message);
					printf(buff);
					
					


					while ((fp = fopen(filename, "w+")) == NULL) {
						sleep(1);
					}

					fprintf(fp, "%s %d\n", username, newsockfd);
					fclose(fp);
					write(newsockfd, s, MAX);
				}
				else {
					/*Goes through list of users and makes sure the username is not taken.*/

					if (findUser(message) == 0) {
						/*Creates the welcome message for a new user*/
						sprintf(s, message);
						strcat(s, " has joined the ");
						strcat(s, topic);
						strcat(s, "chat.\n");

						strcpy(buff, message);
						addUserToList(buff, newsockfd);
						messageAllUsers(s, newsockfd);
						//messageAllUsers(s, newsockfd);
					}
					else {
						sprintf(s, "Username already taken.\n");
						write(newsockfd, s, MAX);
					}
				}
				/* Send the reply to the client. */
				
				//close(newsockfd);
				break;

				/*Once a user is registered they can send messages*/
			case 'M':
				;
				strcpy(s, username);

				strcat(s, ": ");
				strcat(s, message);
				messageAllUsers(s, newsockfd);
				break;
				/*When the user ends the client we remove the user from the list and end their socket.*/
			case 'Q':
				;
				FILE				*fp;
				char				oldusername[53];
				int					oldsock;
				int found = 0;

				fp = fopen(filename, "r");

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
				messageAllUsers(s, newsockfd);
				//close(newsockfd);
				exit(0);
				break;
			default: strcpy(s, "Invalid request\n");
				write(newsockfd, s, MAX);
				break;
			}

		
	}

			/* Send the reply to the client. */
			//write(newsockfd, s, MAX);
    }
	close(newsockfd);
}


/* Registers a User to the Chat room*/
void addUserToList(char username[53], int sockID) {

	FILE				*fp;
		while ((fp = fopen(filename, "a")) == 0) {
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

	fp = fopen(filename, "r");

	while ((fscanf(fp, "%s %d\n", oldusername, &oldsock)) != EOF) {
		if (strcmp(oldusername, username) == 0) {
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

	fp = fopen(filename, "r");

	while ((fscanf(fp, "%s %d\n", oldusername, &sockID)) != EOF) {
		write(sockID, message, MAX);
	}
}

/*Remove User*/
int removeUser(char username[53]) {
	//Forgot how to mess with linked list so based my code off of this: https://codeforwin.org/2015/09/c-program-to-create-and-traverse-singly-linked-list.html
	User * userList = (User *)malloc(sizeof(User));
	User * pu = userList;

	FILE				*fp;
	char				oldusername[53];
	int					oldsock;
	int found = 0;

	while ((fp = fopen(filename, "r+")) == 0) {
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

	if (strcmp(pu->username, username) == 0) {

		pu = pu->nextUser;
		userList->nextUser = NULL;
		free(userList);
		userList = pu;
	}

	while (pu != NULL) {

		if (strcmp(pu->nextUser->username, username) == 0) {
			User * removal = pu->nextUser;

			pu->nextUser = removal->nextUser;
			removal->nextUser = NULL;
			free(removal);
		}
		pu = pu->nextUser;
	}

	while ((fp = fopen(filename, "w+")) == 0) {
		sleep(1);
	}

	pu = userList;

	while (pu != NULL) {
		fprintf("%s %d\n", pu->username, pu->socketId);

	}
	fclose(fp);

}
