#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/time.h>

#define SRV_PORT 34025
//server query string
#define A2S_INFO "\xFF\xFF\xFF\xFF\x54Source Engine Query" 
#define A2S_INFO_LENGTH 25
//query from client string
#define A2S_QUERY "\xFF\xFF\xFF\xFF\x54" 
#define A2S_QUERY_LENGTH 5
#define SERVER_ANSWER "\xFF\xFF\xFF\xFF\x49" 
#define SERVER_TEMPLATE_LEN 5

#define MAXLEN 1024
#define TIMEOUT 2
#define MINLEN 64

struct sockaddr_in loc_addr; 
struct sockaddr_in srv_addr;
struct sockaddr_in cli_addr;

int cli_sock; 
int srv_sock;

int Socket(int domain, int type, int proto) {
    int sock = socket(domain, type, proto);
    if ( sock <= 0 ) {
        perror("Socket error");
        exit(EXIT_FAILURE);
    }
    return sock;
}

void GetParam(int argc, char **argv) {
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
	srv_addr.sin_family = AF_INET;
} 

long mtime()
{	
	struct timeval tv;
	gettimeofday(&tv, NULL);
	long mt = (long)tv.tv_sec;
	return mt;
}

int main(int argc, char *argv[])
{
	GetParam(argc, argv);

	cli_sock = Socket(AF_INET,SOCK_DGRAM,0);
	srv_sock = Socket(AF_INET,SOCK_DGRAM,0);
	
	loc_addr.sin_family = AF_INET;
	loc_addr.sin_port = htons(SRV_PORT);
	loc_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//Program socket ip addr (bind addr) 
	if ( bind(cli_sock, (const struct sockaddr *)&loc_addr,sizeof(loc_addr)) < 0 ) 
	{ 
		perror("Bind failed"); 
		exit(EXIT_FAILURE); 
	}

	struct pollfd fds[2];
	fds[0].fd = cli_sock;	
	fds[0].events = POLLIN;
	fds[1].fd = srv_sock; 
	fds[1].events = POLLIN;
	
	struct srv_info {
		int data_len;
		unsigned char info[MAXLEN];
	} a2s;

	int len, n, ret; 
	unsigned char buffer[MAXLEN]; 	
	long cur_time              = 0;
	long next_request_time     = 0;
 	long next_request_period = 2e6;

	while(1)
	{	
		ret = poll(fds, 2, TIMEOUT * 1000);
		if ( -1 == ret ){
			perror("poll error");
        	return 1;
		}
		
		cur_time = mtime();
		if (!ret || next_request_time < cur_time)
		{ 
			n = sendto(srv_sock, A2S_INFO, A2S_INFO_LENGTH, 
					MSG_DONTWAIT, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
			next_request_time = cur_time + next_request_period;
			printf("%ld\n", cur_time);
		}
		
		if ( fds[0].revents & POLLIN )
		{ 	
			n = recvfrom(cli_sock, (unsigned char *)buffer, MAXLEN,  
        	        MSG_WAITALL, ( struct sockaddr *) &cli_addr, &len);
			ret = memcmp(A2S_QUERY, buffer, A2S_QUERY_LENGTH);
			if ( 0 == ret ) 
			{
				if (a2s.data_len){
					n = sendto(cli_sock,(unsigned char *) a2s.info, a2s.data_len, 
						MSG_DONTWAIT,(struct sockaddr *) &cli_addr, sizeof(cli_addr));
				}
			}
		}

		if ( fds[1].revents & POLLIN )
		{ 	 
			n = recvfrom(srv_sock, (unsigned char *) buffer, MAXLEN, 
        	        MSG_WAITALL, ( struct sockaddr *) &cli_addr, &len);
			//Check server answer(first 5 bytes)	
			ret = memcmp(SERVER_ANSWER, buffer, SERVER_TEMPLATE_LEN);
			if ( 0 == ret ) {
				a2s.data_len = n;
				printf("%s\n", "correct server answer"); 
				memcpy(a2s.info, buffer, n);
			}
		}
		
	}
	
	close(cli_sock);
	close(srv_sock);
//printf("receive byte:%d, from client ip: %s:%d\n ", 
//		n, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
/*
typedef struct {
	char version;
	char hostname[256];
	char map[32];
	char game_directory[32];
	char game_description[256];
	short app_id;
	char num_players ;
	char max_players;
	char num_of_bots;
	char dedicated;
	char os;
	char password;
	char secure;
	char game_version[32];
} A2S_INFO_REPLY;
*/

	return 0;
}
