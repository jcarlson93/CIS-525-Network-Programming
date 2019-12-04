/*-------------------------------------------------------------*/
/* Assignment E based off client.c from net7                   */
/* Created by Nickalas Porsch                                  */
/*-------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 

#define MAX 1000

char * get_page(void);
int readn(int, char *, int);

main(int argc, char **argv)
{
	struct hostent		*host_entry;
	int                 sockfd;
	struct sockaddr_in  serv_addr;
	char                s[MAX];      /* array to hold output */
	int                 response;    /* user response        */
	int                 nread;       /* number of characters */
	int					x;
	char *				url;
	char *				ipBuff;
	char *				page;
	int					port = 80;
	char *				portAsString;

	if (argc < 2 || argc > 3) {
		perror("Need at least one argument URL and optional parameter PORT.\n");
		exit(1);
	}

	/* Getting URL from client */
	url = argv[1];
	
	if (argc == 2) {
		portAsString = argv[2]
	}
	/*How to convert url to ip https://www.geeksforgeeks.org/c-program-display-hostname-ip-address/*/
	host_entry = gethostbyname(url);

	ipBuff = inet_ntoa(*((struct in_addr *) host_entry->h_addr_list[0]));
	printf("Trying %s...\n", ipBuff);
	printf("Connected to %s.\n", url);
	
	page = get_page();
	
	///* Set up the address of the server to be contacted. */
	//memset((char *)&serv_addr, 0, sizeof(serv_addr));
	//serv_addr.sin_family = AF_INET;
	//serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
	//serv_addr.sin_port = htons(SERV_TCP_PORT);

	///* Display the menu, read user's response, and send it to the server. */
	//while ((response = get_response()) != 4) {

	//	/* Create a socket (an endpoint for communication). */
	//	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	//		perror("client: can't open stream socket");
	//		exit(1);
	//	}

	//	/* Connect to the server. */
	//	if (connect(sockfd, (struct sockaddr *) &serv_addr,
	//		sizeof(serv_addr)) < 0) {
	//		perror("client: can't connect to server");
	//		exit(1);
	//	}

	//	sprintf(s, "%d", response);

	//	/* Send the user's request to the server. */
	//	x = 256;
	//	write(sockfd, &x, sizeof(int));
	//	write(sockfd, s, sizeof(char));

	//	/* Read the server's reply. */
	//	nread = readn(sockfd, s, MAX);
	//	if (nread > 0) {
	//		printf("   %s\n", s);
	//	}
	//	else {
	//		printf("Nothing read. \n");
	//	}
	//	close(sockfd);
	//}
	exit(0);  /* Exit if response is 4  */
}

/* Asks the user for the page they would like to see */
char * get_page()
{
	char  * page = (char *)malloc(sizeof(char) * MAX); //Character names can only be up to 50 characters (We say 51 to include \n

	printf("Which page? ");
	scanf("%s", &page);

	return(page);
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

