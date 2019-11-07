/*-------------------------------------------------------------*/
/* server.c - sample iterative time/date server.               */
/*-------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include "inet.h"

#define MAX 100

main(int argc, char **argv)
{
    int                 sockfd, newsockfd, childpid, pid, randomValue;
    unsigned int	clilen;
    struct sockaddr_in  cli_addr, serv_addr;
    struct tm           *timeptr;  /* pointer to time structure */
    time_t              clock;     /* clock value (in secs)     */
    char                s[MAX];
    char                request;

    /* Create communication endpoint */
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("server: can't open stream socket");
        exit(1);
    }

    /* Bind socket to local address */
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port        = htons(SERV_TCP_PORT);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("server: can't bind local address");
        exit(1);
    }

    listen(sockfd, 5);

    for ( ; ; ) {

        /* Accept a new connection request. */
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("server: accept error");
            exit(1);
        }

        /* Read the request from the client. */
        read(newsockfd, &request, sizeof(char));

        /* Generate an appropriate reply. */
        clock = time(0);
        timeptr = localtime(&clock);

        switch(request) {

            case '1': strftime(s, MAX, "%T", timeptr);
	              break;

	    case '2': 
			pid = getpid();
			sprintf(s, "%d", pid); //Sets s to the current PID
	              break;

	    case '3':
			randomValue = (rand() % (30)) + 1;
			sprintf(s, "%d", randomValue); //Returns a random number
                      break;

            default: strcpy(s,"Invalid request\n");
                     break;
        }

        /* Send the reply to the client. */
        write(newsockfd, s, MAX);
	close(newsockfd);     
    }
}
