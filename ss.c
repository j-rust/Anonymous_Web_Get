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
//	if(DEBUG) {
//		printf("Calling client()\n");
//		client();
//	}
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
	clientfd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
	printf("Accepted an incoming connection\n");

	char *rec_buffer = calloc(500, sizeof(char));
	uint32_t total_length;
	uint32_t total_bytes_received = 500;
	char *data;
	int recv_status;

	/* First packet received is the url */
	recv_status = recv(clientfd, rec_buffer, 500, 0);
	unsigned int msg_length;
	memcpy(&msg_length, rec_buffer + 4, 2);
	char *url = calloc(msg_length, sizeof(char));
	memcpy(url, rec_buffer + 6, msg_length);
	memset(rec_buffer, 0, strlen(rec_buffer));


	FILE *ofp = fopen("chainlist.txt", "w");

	while (total_bytes_received < total_length) {
		msg_length = 0;
		recv_status = recv(clientfd, rec_buffer, 500, 0);
		if (recv_status < 0) { perror("Error: receive failed\n"); }

		memcpy(&total_length, rec_buffer + 0, 4);
		memcpy(&msg_length, rec_buffer + 4, 2);
		total_length = htonl(total_length);
		msg_length = htons(msg_length);

		printf("File Length: %zu\n", total_length);
		printf("Message Length: %u\n", msg_length);

		total_bytes_received += msg_length;
		data = calloc(msg_length, sizeof(char));
		memcpy(data, rec_buffer + 6, msg_length);
		printf("Message is: %s\n", data);
		fprintf(ofp, data);

		free(data);
		memset(rec_buffer, 0, strlen(rec_buffer));
	}
	fclose(ofp);

	char* next_ss = getNextSteppingStone();
	if (next_ss == NULL) {
		// call wget and send back
	} else {
		client(next_ss, url);
	}

}

void client(char* next_ss_info, char* url){

	char *mutable_info = calloc(strlen(next_ss_info), sizeof(char));
	memcpy(mutable_info, next_ss_info, strlen(next_ss_info));

	int sockfd, status, connect_status;
	struct sockaddr_in serv_addr;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	char* ip = parseIP(mutable_info);
	if(DEBUG) printf("Parsed IP address: %s\n", ip);
	memset(mutable_info, 0, strlen(next_ss_info));
	memcpy(mutable_info, next_ss_info, strlen(next_ss_info));
	char *port = parsePort(mutable_info);
	if(DEBUG) printf("Parsed port: %s\n", port);

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
	char* line_holder = calloc(100, sizeof(char));
	char c = NULL;
	int num_ss;

	c = fgetc(ifp);
	num_ss = c - '0';
	fclose(ifp);
	if(num_ss == 1) return NULL;

	num_ss--;
	removeCurrentHost(num_ss);

	return getRandomSS("chainlist.txt");

}

void removeCurrentHost(int num_ss){

	char* ip = getCurrentIP();
	FILE *ifp = fopen("chainlist.txt", "r");
	FILE *ofp = fopen("chainlist.txt.new", "w");
	char* line = calloc(30, sizeof(char));
	char *backup_line = calloc(30, sizeof(char));
	char* compIP;


	fgets(line, 30, ifp);
	if(DEBUG) printf("Read first line and got %s", line);

	fprintf(ofp, "%d\n", num_ss);

	while (fgets(line, 30, ifp)) {
		memcpy(backup_line, line, 30);
		compIP = parseIP(line);
		/* Only write to the new file if it's not the current IP address */
		printf("Compared %s to %s and got %d\n", compIP, ip, strcmp(ip, compIP));
		if (strcmp(ip, compIP) != 0) {
			fprintf(ofp, "%s", backup_line);
		}
		memset(line, 0, 30);
		memset(backup_line, 0, 30);
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

	return inet_ntoa(*((struct in_addr *)Host->h_addr));
}

char* parseIP(char* line){
	char* tmp;
	tmp = strtok(line, "\t");
	return tmp;
}

char* parsePort(char *line){
	char* tmp;
	if(DEBUG) printf("Received line %s\n", line);
	tmp = strtok(line, "\t");
	if(DEBUG) printf("First port token: %s\n", tmp);
	tmp = strtok(NULL, "\t");
	return tmp;
}

int generateRandomNumber(int num_ss)
{
	time_t t;
	int next_ss; /* line containg info for the next stepping stone */
	srand((unsigned) time(&t));
	/* Generate 100 random numbers to increase randomness -> probably not necessary */
	int i = 0;
	for (i; i < 100; i++) {
		next_ss = rand() % (num_ss);
	}
	return next_ss + 1;
}

char* getRandomSS()
{
	FILE *ifp;
	ifp = fopen("chainlist.txt", "r");
	char* contentsOfLine = calloc(100, sizeof(char));
	int numberOfLinesInFile = 0;
	int lineCounter = 0;
	char ch;
	int positionOfCharInLine = 0;
	int randomNumber = 0;

	/*Get the characters on the first line of the file which will be the number of lines in the file*/
	while((ch = fgetc(ifp) ) != '\n' )
	{
		if(ch != '\n')
		{
			contentsOfLine[positionOfCharInLine] = ch;
		}
		else
		{
			break;
		}
		positionOfCharInLine++;
	}

	numberOfLinesInFile = atoi(contentsOfLine);
	randomNumber = (generateRandomNumber(numberOfLinesInFile)) + 1;
	positionOfCharInLine = 0;

	if(randomNumber == numberOfLinesInFile) randomNumber = randomNumber - 1;


	do
	{
		if(ch == '\n' && lineCounter == randomNumber)
		{
			return contentsOfLine;
		}
		else if(ch == '\n')
		{
			lineCounter++;
			memset(contentsOfLine, 0, sizeof(contentsOfLine));
			positionOfCharInLine = 0;
		}
		else
		{
			contentsOfLine[positionOfCharInLine] = ch;
			positionOfCharInLine++;
		}
	}while((ch = fgetc(ifp) ) != EOF );

}



int main(int argc, char** argv){
	if(argc > 1) server(atoi(argv[1]));
	else server(0);
	return 0;
}

