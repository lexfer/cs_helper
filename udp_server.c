#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/time.h>
#include <getopt.h>
#include <ctype.h>

#define LOCAL_PORT 34025
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
int check_port(char *optarg) {
	int pos = 0; 
	int port;
	while ( pos < strlen(optarg) )	{
		if ( 0 == isdigit(optarg[pos]) ) {
			fprintf(stderr, "Invalid port: %s\n", optarg);
			exit(EXIT_FAILURE);
		}
		pos++;
	}
	port = atoi(optarg);
	if ( port < 1 || port > 65535 ) {
					fprintf(stderr, "Invalid port: %s\n", optarg);
					exit(EXIT_FAILURE);
				}
	return port; 	
}
void Usage(void) {
	fprintf(stderr, "Usage:  --bindip local_ip --bindport local_port --server server_ip --port server_ip\n");
}

void GetParam(int argc, char **argv) {
	int longIndex = 0, opt; 	
	static const struct option longOpts[] = {
    	{ "server",    required_argument, NULL, '1' },
    	{ "port",      required_argument, NULL, '2' },
    	{ "localip",   required_argument, NULL, '3' },
    	{ "localport", required_argument, NULL, '4' },
    	{ "help",      no_argument,       NULL, 'h' },
	   	{ NULL,        no_argument,       NULL,  0  }
	};
	while ( (opt = getopt_long( argc, argv, "", longOpts, &longIndex ) ) != EOF ) {
		switch (opt) {
			case 'h': {
				Usage();
				break;
			}
			case '1': {
				if ( 0 == inet_aton(optarg, &srv_addr.sin_addr) ) {
					fprintf(stderr, "Invalid address, %s\n", optarg);
					exit(EXIT_FAILURE);
				}	
				break;
			}
			case '2': {
					srv_addr.sin_port = htons(check_port(optarg));
				break;
			}
			case '3': {
				if ( 0 == inet_aton(optarg, &loc_addr.sin_addr) ) {
					fprintf(stderr, "Invalid address, %s\n", optarg);
					exit(EXIT_FAILURE);
				}	
				break;
			}
			case '4': {
				loc_addr.sin_port = htons(check_port(optarg));
				break;
			}	
			case '?': {
				Usage();
				exit(EXIT_FAILURE);
				break;
			}
			default: {
				printf("unknown option\n");
				exit(0);
				break;
			}
		}
	}
}
/*
	
	if (inet_aton(argv[2], &srv_addr.sin_addr) == 0) {
		fprintf(stderr, "Invalid address\n");
		exit(EXIT_FAILURE);
    }
	if (atoi(argv[2]) < 1 || atoi(argv[2]) > 65535) {
		fprintf(stderr, "Invalid port\n");
		exit(EXIT_FAILURE);
	}
	srv_addr.sin_port = htons(atoi(argv[2]));
	srv_addr.sin_family = AF_INET;
*/

long mtime()
{	
	struct timeval tv;
	gettimeofday(&tv, NULL);
	long mt = (long)tv.tv_sec;

	return mt;
}

int main(int argc, char *argv[]) {	

	loc_addr.sin_family      = AF_INET;
	loc_addr.sin_port        = htons(LOCAL_PORT);
	loc_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	srv_addr.sin_family = AF_INET;
	
	GetParam(argc, argv);

	cli_sock = Socket(AF_INET,SOCK_DGRAM,0);
	srv_sock = Socket(AF_INET,SOCK_DGRAM,0);
	
		//Program socket ip addr (bind addr) 
	if ( bind(cli_sock, (const struct sockaddr *)&loc_addr,sizeof(loc_addr)) < 0 ) 
	{ 
		perror("Bind failed"); 
		exit(EXIT_FAILURE); 
	}

	struct pollfd fds[2];
	fds[0].fd     = cli_sock;	
	fds[0].events = POLLIN;
	fds[1].fd     = srv_sock; 
	fds[1].events = POLLIN;
	
	struct srv_answer {
		int data_len;
		unsigned char info[MAXLEN];
	} s2a = { 0 };

	int len, n, ret; 
	unsigned char buffer[MAXLEN]; 	
	long cur_time            = 0;
	long next_request_time   = 0;
 	long next_request_period = TIMEOUT;

	while(1)
	{	
		ret = poll(fds, 2, TIMEOUT * 1000);
		if ( -1 == ret ){
			perror("poll error");
        	return 1;
		}
		
		cur_time = mtime();
		if (!ret || next_request_time <= cur_time)
		{ 
			n = sendto( srv_sock, A2S_INFO, A2S_INFO_LENGTH, MSG_DONTWAIT, 
						( struct sockaddr *) &srv_addr, sizeof(srv_addr) );
			next_request_time = cur_time + next_request_period;
			printf("%ld\n", cur_time);
		}
		
		if ( fds[0].revents & POLLIN )
		{ 	
			n = recvfrom( cli_sock, (unsigned char *)buffer, MAXLEN,  
						  MSG_WAITALL, ( struct sockaddr *) &cli_addr, &len );
		
			ret = memcmp( A2S_QUERY, buffer, A2S_QUERY_LENGTH );
			if ( 0 == ret ) 
			{
				if (s2a.data_len) {
					n = sendto( cli_sock, (unsigned char *) s2a.info, s2a.data_len, 
                         		MSG_DONTWAIT, (struct sockaddr *) &cli_addr, sizeof(cli_addr) );
				}
			}
		}

		if ( fds[1].revents & POLLIN )
		{ 	 
			n = recvfrom( srv_sock, (unsigned char *) buffer, MAXLEN, MSG_WAITALL, 
					      ( struct sockaddr *) &cli_addr, &len );
			//Check server answer(first 5 bytes)	
			ret = memcmp( SERVER_ANSWER, buffer, SERVER_TEMPLATE_LEN );
			if ( 0 == ret ) {
				s2a.data_len = n;
				printf("%s\n", "correct server answer"); 
				memcpy(s2a.info, buffer, n);
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
