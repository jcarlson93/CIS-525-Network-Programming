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

#define MAX 100000

/*Method Definitions*/
char * get_page(void);
int readn(int, char *, int);

/*Main function that allows a user to provide a website and port and then a webpage. The program will then print the webpage or error code to the user.*/
main(int argc, char **argv)
{
	struct hostent		* host_entry; /*Used to convert the URL to the correct IP Address*/
	int                 sockfd;		/* Socket file descriptor that connects to server*/
	struct sockaddr_in  serv_addr;  /* Represents the server address to the server*/
	char                s[MAX];      /* array to hold output */
	int                 nread;       /* number of characters */
	char *				url;		 /* URL provided by the user*/
	char *				ipBuff;      /* IP address written in dot notation */
	char *				page;        /* The page the user would like to view*/
	int					port = 80;   /* The port to connect to the website */
	char *				portAsString; /* The Port parameter given as a string parameter */
	char				getRequest[MAX]; /*The initial GET Request */
	char 				httpCode[MAX];  /* The success code given back from the server */
	char *				webPage; /* The webpage represented in HTML */
	char *				errorCode; /* If the webpage is not returned correctly an error code will be given*/

	if (argc < 2 || argc > 3) {
		perror("Need at least one argument URL and optional parameter PORT.\n");
		exit(1);
	}

	/* Getting URL from client */
	url = argv[1];
	
	if (argc == 3) {
		portAsString = argv[2];
		/* How to convert an string to int*/
		for (int i = 0; i < strlen(portAsString); i++) {
			port = port * 10 + (portAsString[i] - '0');
		}
	}

	/*How to convert url to ip https://www.geeksforgeeks.org/c-program-display-hostname-ip-address/*/
	host_entry = gethostbyname(url);

	ipBuff = inet_ntoa(*((struct in_addr *) host_entry->h_addr_list[0]));
	printf("Trying %s...\n", ipBuff);
	printf("Connected to %s.\n", url);
	
	page = get_page();
	
	/*How to create a GET Request https://aticleworld.com/http-get-and-post-methods-example-in-c/ */
	/*This helped with HTTP 1.0 GET Request*/

	sprintf(getRequest, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", page, url);

	/* Set up the address of the server to be contacted. */
	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ipBuff);
	serv_addr.sin_port = htons(port);

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

	write(sockfd, getRequest, MAX);

	//	/* Read the server's reply. */
		nread = readn(sockfd, s, MAX);
		if (nread > 0) {
			strncpy(httpCode, s, MAX);
			if ((strstr(httpCode, "HTTP/1.0 200 OK") != NULL) || (strstr(httpCode, "HTTP/1.1 200 OK"))) {

				/*Does string manipulation to return the webpage starting with DOCTYPE */
				webPage = strtok(s, "<");
				webPage = strtok(NULL,"\0");
				strcpy(s, "<");
				strcat(s, webPage);
				
				printf("%s\n", s);
			}
			else {
				/* Does string manipulation on the returned page to get the error code.*/
				errorCode = strtok(s, "<");
				errorCode = strtok(NULL, "<");
				errorCode = strtok(NULL, "<");
				errorCode = strtok(NULL, "<");
				errorCode = strtok(NULL, "<");
				errorCode = strtok(errorCode, ">");
				errorCode = strtok(NULL, "\0");
				printf("%s\n",errorCode);
			}
		}
		else {
			printf("Nothing read. \n");
		}
	close(sockfd);
	exit(0); 
}

/* Asks the user for the page they would like to see */
char * get_page()
{
	char  * page = (char *)malloc(sizeof(char) * MAX); //Character names can only be up to 50 characters (We say 51 to include \n

	printf("Which page? ");
	scanf("%s", page);

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

