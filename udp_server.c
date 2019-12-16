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
unsigned char buffer[MAXLEN]; 	
struct sockaddr_in addr, srv_addr, cli_addr;
int sock, sock_query;

int Socket(int domain, int type, int proto) {
    int desk = socket(domain, type, proto);
    if (desk <= 0) {
        perror("Socket error");
        exit(EXIT_FAILURE);
    }
    return desk;
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

int SendToServer()
{
	int n;
	n = sendto(sock_query, A2S_INFO, A2S_INFO_LENGTH, 
					MSG_DONTWAIT, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
	return n;
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

	sock = Socket(AF_INET,SOCK_DGRAM,0);
	sock_query = Socket(AF_INET,SOCK_DGRAM,0);
	
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

	int len, n, ret, ans_len = 0; 
	unsigned char srv_buffer[MAXLEN]; 	
	unsigned char answer[MAXLEN]; 	
	long cur_time 					= 0;
	long next_request_time 		= 0;
 	long next_request_period = 2e6;

	while(1)
	{	
		ret = poll(fds, 2, TIMEOUT * 1000);
		if ( -1 == ret ){
			perror("poll error");
        	return 1;
		}
		cur_time = mtime();
		if(next_request_time < cur_time){
			SendToServer();
			next_request_time = cur_time + next_request_period;
			printf("%ld\n", cur_time);
		}
		/*TIMEOUT (query server if not receive any data)
		This block work when client not send query*/
		if (!ret)
			SendToServer();
		if ( fds[0].revents & POLLIN ){ 	//client query
			n = recvfrom(sock, (unsigned char *)buffer, MAXLEN,  
        	        MSG_WAITALL, ( struct sockaddr *) &cli_addr, &len);
			if (memcmp(A2S_QUERY, buffer, A2S_QUERY_LENGTH) == 0)
			{
			  //printf("receive byte:%d, from client ip: %s:%d\n ", 
			//		n, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
				if (ans_len){
					n = sendto(sock,(unsigned char *) answer, ans_len, 
						MSG_DONTWAIT,(struct sockaddr *) &cli_addr, sizeof(cli_addr));
				}
			}
		}
		if ( fds[1].revents & POLLIN ){ 	//server answer, get new data 
			n = recvfrom(sock_query, (unsigned char *) srv_buffer, MAXLEN, 
        	        MSG_WAITALL, ( struct sockaddr *) &cli_addr, &len);
			//Check server answer(first 5 bytes)	
			ret = memcmp(SERVER_ANSWER, srv_buffer, SERVER_TEMPLATE_LEN);
			if (0 == ret){
				ans_len = n;
				printf("%s\n", "correct server answer"); 
				memcpy(answer, srv_buffer, ans_len);
			}
		}
		
	}
	
	close(sock);
	close(sock_query);
   
	return 0;
}
