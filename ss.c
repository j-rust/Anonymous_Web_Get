//
// Created by Jonathan Rust on 10/20/15.
//

#include "ss.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>

#define DEBUG 1

void server(unsigned short port){
	if(DEBUG) {
		printf("Calling client()\n");
		client();
	}
	int status, sockfd, clientfd, listen_success;
	struct sockaddr_in server;
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	char ipstr[INET6_ADDRSTRLEN];

	memset(&server, 0, sizeof(server));
	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sockfd == -1){
		perror("Error establishing socket\n");
		return;
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(port);

	int bind_status = bind(sockfd, (struct sockaddr *) &server, sizeof(server));
	if (bind_status < 0) {
		perror("Error binding to socket\n");
		return;
	}


	listen_success = listen(sockfd, 10);
	if(listen_success == -1){
		perror("Unable to listen to socket\n");
		return;
	}

	struct sockaddr_in sockin;
	socklen_t socklen = sizeof(sockin);
	getsockname(sockfd, (struct sockaddr *) &sockin, &socklen);


	printf("%s %d\n", getCurrentIP(), ntohs(sockin.sin_port));

	addr_size = sizeof(their_addr);
	int byte_count;
	clientfd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

}

void client(){

	if(DEBUG) printf("Calling getNextSteppingStone()\n");
	char* next_ss_info = getNextSteppingStone();
	if(DEBUG){
		exit(0);
	}
	if(next_ss_info == NULL){
		// call wget and send the file down the chain
	}

	int sockfd, status, connect_status;
	struct sockaddr_in serv_addr;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	char* ip = NULL;
	char* port = NULL;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;


	if ((status = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}

	sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	connect_status = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
	if(connect_status < 0){
		perror("Error trying to connect\n");
		return;
	}

}

char* getNextSteppingStone(){
	FILE *ifp = fopen("chainlist.txt", "r");

	if (ifp == NULL) {
		perror("Can't open file chainlist.txt");
		exit(-1);
	}
	int lines = 0;
	char* line_holder = calloc(100, sizeof(char));

	/* Need to get a line count first to properly call rand() */
	while (fgets(line_holder, 100, ifp)) {

	}
	if(lines == 1) return NULL;

	if(DEBUG) printf("Calling removeCurrentHost()\n");
	removeCurrentHost();
	lines--;

	time_t t;
	int next_host; /* line containg info for the next stepping stone */

	srand((unsigned) time(&t));
	/* Generate 100 random numbers to increase randomness -> probably not necessary */
	int i = 0;
	for (i; i < 100; i++) {
		next_host = rand() % (lines - 1);
	}



	return NULL;

}

void removeCurrentHost(){

	char* ip = getCurrentIP();
	FILE *ifp = fopen("chainlist.txt", "r");
	FILE *ofp = fopen("chainlist.txt.new", "w");
	char* line = calloc(100, sizeof(char));
	char* compIP;

	while (fgets(line, 100, ifp)) {
		if(DEBUG) printf("Calling parseIP()\n");
		compIP = parseIP(line);
		if(DEBUG) printf("DEBUG - Parsed IP address and got %s\n", compIP);
		/* Only write to the new file if it's not the current IP address */
		printf("Compared %s to %s and got %d\n", compIP, ip, strcmp(ip, compIP));
		if(strcmp(ip, compIP) != 0) {
			fprintf(ofp, line);
			fprintf(ofp, "\n");
		}
	}
	fclose(ifp);
	fclose(ofp);
	rename("chainlist.txt.new", "chainlist.txt");

}

char* getCurrentIP(){
	char Buf [ 200 ] ;
	struct hostent * Host = (struct hostent * ) malloc ( sizeof ( struct hostent ));
	gethostname ( Buf , 200 );
	Host = ( struct hostent * ) gethostbyname ( Buf );
	printf("%s\n", inet_ntoa(*((struct in_addr *)Host->h_addr)));

	return inet_ntoa(*((struct in_addr *)Host->h_addr));
}

char* parseIP(char* line){
	char* tmp;
	tmp = strtok(line, "\t");
	return tmp;
}




int main(int argc, char** argv){
	if(argc > 1) server(atoi(argv[1]));
	else server(0);
	return 0;
}

