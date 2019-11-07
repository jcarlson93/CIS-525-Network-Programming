/*
 * Definitions for TCP and UDP client/server programs.
 */

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define	SERV_UDP_PORT	9763
//I changed this to localhost to do testing on my own laptop before trying it on cougar
#define	SERV_HOST_ADDR	"127.0.0.1"//"129.130.10.43" /* Change this to be your host addr */
