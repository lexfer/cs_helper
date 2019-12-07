#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/select.h>

#define MAXLEN 128


int socket(int domain, int type, int protocol);

//client query
const unsigned char  cli_query[] = {0xff, 0xff, 0xff, 0xff, 0x54};
const int cmp_query_size = sizeof(cli_query); //compare size
//server query
const unsigned char  srv_query[] = 
				{ 
				0xff,0xff,0xff,0xff,0x54,0x53,0x6f,0x75,0x72,
				0x63,0x65,0x20,0x45,0x6e,0x67,0x69,0x6e,0x65,
				0x20,0x51,0x75,0x65,0x72,0x79,0x00
				};
int sock;
unsigned char buffer[MAXLEN]; 	
struct sockaddr_in addr, srv_addr, cli_addr;
int main(int argc, char *argv[])
{
    if (argc != 3) {
    	fprintf(stderr, "%s <server-address> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
   	}
	if (inet_aton(argv[1], &srv_addr.sin_addr) == 0) {
        fprintf(stderr, "Invalid address\n");
		exit(EXIT_FAILURE);
    }
	if (atoi(argv[2]) < 1 || atoi(argv[2]) > 65535) {
        fprintf(stderr, "Invalid port\n");
		exit(EXIT_FAILURE);
    }
	srv_addr.sin_port = htons(atoi(argv[2]));
    // Creating socket file descriptor 
    if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("Socket creation failed"); 
        exit(EXIT_FAILURE); 
    }
	//Program socket ip addr (bind addr) 
    addr.sin_family = AF_INET;
    addr.sin_port = htons(34025);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
 	 // Bind the socket with the server address 
    if ( bind(sock, (const struct sockaddr *)&addr,  
            sizeof(addr)) < 0 ) 
    { 
        perror("Bind failed"); 
        exit(EXIT_FAILURE); 
    }
	
	while(1)
	{

		int len, n; 
	    n = recvfrom(sock, (unsigned char *)buffer, MAXLEN,  
        	        MSG_WAITALL, ( struct sockaddr *) &cli_addr, &len);
 		if (cli_addr.sin_addr.s_addr == srv_addr.sin_addr.s_addr){
			printf("%s", "Get server response\n");
		} 

    	if (memcmp(cli_query, buffer, cmp_query_size) == 0){
			  printf("receive byte:%d, from client ip: %s:%d\n ", 
					n, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
		}
	}
	
	close(sock);
    return 0;
}
