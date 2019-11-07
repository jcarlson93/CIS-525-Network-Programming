/*-------------------------------------------------------------*/
/* server.c - sample time/date server.                         */
/* Base code provided by example net1 from notes.              */
/* Project by Nickalas Porsch                                  */
/*-------------------------------------------------------------*/

/*To run do gcc server.c -o server; gcc client.c -o client and then ./server &; ./client*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include "inet.h"
#include <unistd.h>

#define MAX 100

main(int argc, char **argv)
{
    int                 sockfd, newsockfd, clilen, childpid, pid, randomValue;
    struct sockaddr_in  cli_addr, serv_addr;
    struct tm           *timeptr;  /* pointer to time structure */
    time_t              clock;     /* clock value (in secs)     */
    char                s[MAX];
    char                request;

    /* Create communication endpoint */
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("server: can't open datagram socket");
        exit(1);
    }

    printf("Created Communication with endpoint\n");

    /* Bind socket to local address */
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET; // AF_INET means used IPv4
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //host to network long (addresses) we don't care what address we are using for the server
    serv_addr.sin_port        = htons(SERV_UDP_PORT); //host to network short (ports)

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("server: can't bind local address"); //perror decodes to a human readable error.
        exit(1);
    }

    for ( ; ; ) { //same as while(true) and never actually stops

        /* Read the request from the client. */
	clilen = sizeof(cli_addr);
        recvfrom(sockfd, (char *) &request, sizeof(request), 0,
                  (struct sockaddr *)&cli_addr, &clilen);
printf("Received: %c \n", request);

        /* Generate an appropriate reply. */
        clock = time(0);
        timeptr = localtime(&clock);

        switch(request) {
			case '0': 
				strcpy(s, "Goodbye!");
				break;
            case '1':   strftime(s, MAX, "%T", timeptr); //formats time
	            break;

	        case '2': 
				pid = getpid();
				sprintf(s, "%d", pid); //Sets s to the current PID
				break;

			//Forgot how to do ranges with random and got help from https://www.geeksforgeeks.org/generating-random-number-range-c/ 	 
			case '3': 
				randomValue = (rand() % (30)) + 1 ;
				sprintf(s, "%d", randomValue); //Returns a random number
                break;

            default: strcpy(s,"Invalid request\n");
                     break;
        }

        /* Send the reply to the client. */
        sendto(sockfd, s, strlen(s)+1, 0,
                (struct sockaddr *) &cli_addr, clilen);
		if (strcmp(s, "Goodbye!") == 0) { /*Checks to see if Goodbye! was sent meaning the server should be shutdown.*/
			exit(0);
		}
    }
}
