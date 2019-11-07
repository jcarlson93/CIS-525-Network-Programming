/*-------------------------------------------------------------*/
/* client.c - sample time/date client.                         */
/*-------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "inet.h"

#define MAX 100

int get_response(void);
int readn(int, char *, int);

main(int argc, char **argv)
{
    int                 sockfd;
    struct sockaddr_in  serv_addr;
    char                s[MAX];      /* array to hold output */
    int                 response;    /* user response        */
    int                 nread;       /* number of characters */

    /* Set up the address of the server to be contacted. */
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
    serv_addr.sin_port        = htons(SERV_TCP_PORT);

    /* Display the menu, read user's response, and send it to the server. */
    while((response = get_response()) != 4) {

        /* Create a socket (an endpoint for communication). */
        if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("client: can't open stream socket");
            exit(1);
        }

        /* Connect to the server. */
        if (connect(sockfd, (struct sockaddr *) &serv_addr,
          sizeof(serv_addr)) < 0) {
            perror("client: can't connect to server");
            exit(1);
        }

        sprintf(s,"%d",response);

        /* Send the user's request to the server. */
        write (sockfd, s, sizeof(char));

        /* Read the server's reply. */
        nread = readn (sockfd, s, MAX);
        if (nread > 0) {
			printf("   %s\n", s);
	} else {
		printf("Nothing read. \n");
	}
        close(sockfd);
    }
    exit(0);  /* Exit if response is 4  */
}

/* Display menu and retrieve user's response */
int get_response()
{
    int choice;

    printf("===========================================\n");
    printf("                   Menu: \n");
    printf("-------------------------------------------\n");
    printf("                1. Current Time\n");
    printf("                2. Process ID of Server\n");
    printf("                3. Random Number between 1 and 30 inclusive\n");
    printf("                4. Quit\n");
    printf("-------------------------------------------\n");
    printf("               Choice (1-4):");
    scanf("%d",&choice);
    printf("===========================================\n");
    return(choice);
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
		ptr   += nread;
	}
	return(nbytes - nleft);		/* return >= 0 */
}

