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
#include <poll.h>

#define MAXLEN 1024
#define TIMEOUT 2

int socket(int domain, int type, int protocol);

//client query string
const unsigned char  cli_query[] = {0xff, 0xff, 0xff, 0xff, 0x54};
const int cmp_query_size = sizeof(cli_query); //in client query size
//server query string
const unsigned char  srv_query[] = 
				{ 
				0xff,0xff,0xff,0xff,0x54,0x53,0x6f,0x75,0x72,
				0x63,0x65,0x20,0x45,0x6e,0x67,0x69,0x6e,0x65,
				0x20,0x51,0x75,0x65,0x72,0x79,0x00
				};
const int srv_query_size = sizeof(srv_query); //out server query size

int sock;
int sock_query;
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

	// Creating client bind socket file descriptor 
	if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
		perror("Socket creation failed"); 
		exit(EXIT_FAILURE); 
   }
	// Creating  server socket file descriptor for query  
	if ( (sock_query = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
		perror("Socket creation failed"); 
		exit(EXIT_FAILURE); 
   }
	
	srv_addr.sin_family = AF_INET;
	
	//Program socket ip addr (bind addr) 
	addr.sin_family = AF_INET;
	addr.sin_port = htons(34025);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if ( bind(sock, (const struct sockaddr *)&addr,sizeof(addr)) < 0 ) 
	{ 
		perror("Bind failed"); 
		exit(EXIT_FAILURE); 
	}
	
	struct pollfd fds[2];
	fds[0].fd = sock;	//client socket
	fds[0].events = POLLIN;
	fds[1].fd = sock_query; //server socket
	fds[1].events = POLLIN;

	int len, n, ret, addrlen; 
	unsigned char srv_buffer[MAXLEN]; 	
		
	while(1)
	{
		ret = poll(fds, 2, TIMEOUT * 1000);
		if ( ret == -1 ){
			perror("poll error");
        	return 1;
    	}
		//TIMEOUT (query server if not receive any data)
		if (!ret){
			n = sendto(sock_query, (unsigned char *) srv_query, srv_query_size, 
				MSG_DONTWAIT, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
		}
		if ( fds[0].revents & POLLIN ){ 	//client query
			n = recvfrom(sock, (unsigned char *)buffer, MAXLEN,  
        	        MSG_WAITALL, ( struct sockaddr *) &cli_addr, &len);
			if (memcmp(cli_query, buffer, cmp_query_size) == 0){ //check client query
			  printf("receive byte:%d, from client ip: %s:%d\n ", 
					n, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
				n = sendto(sock,(unsigned char *) srv_buffer, srv_query_size, 
					MSG_DONTWAIT,(struct sockaddr *) &cli_addr, sizeof(cli_addr));
			}
		}
		if ( fds[1].revents & POLLIN ){ 	//server answer, get new data 
			n = recvfrom(sock_query, (unsigned char *) srv_buffer, MAXLEN, 
        	        MSG_WAITALL, ( struct sockaddr *) &cli_addr, &len);			
		}
/*
 		if (cli_addr.sin_addr.s_addr == srv_addr.sin_addr.s_addr){
			printf("%s", "Get server response\n");
		} 
*/
	}
	
	close(sock);
	close(sock_query);
   
	return 0;
}
